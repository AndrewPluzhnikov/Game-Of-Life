#ifndef GLIFE_H_
#define GLIFE_H_

// Life on a Graph.

#include <assert.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include"rapidjson/document.h"
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
//#include"rapidjson/writer.h"
#include <rapidjson/prettywriter.h>

using rapidjson::Document;
using rapidjson::IStreamWrapper;
using rapidjson::SizeType;
using rapidjson::Value;
using rapidjson::OStreamWrapper;
using rapidjson::PrettyWriter;

const int live_to_live_threshold = 3;
const int dead_to_live_threshold = 2;

class GLife {
 public:
  explicit GLife(const GLife& other) = default;
  // 'filename' contains the specification of a graph
  // including its state and possibly embedding in JSON format
  explicit GLife(const std::string& filename) {
    // Read and parse 'filename' in JSON format.
    std::ifstream ifs(filename);
    if (!ifs)
      std::cout << "Error: " << filename << " does not exist." << std::endl;

    IStreamWrapper isw(ifs);
    Document doc;
    doc.ParseStream(isw);
    assert(doc.HasMember("vertices"));
    const Value& vertices = doc["vertices"];
    assert(vertices.IsArray());
    assert(doc.HasMember("edges"));
    const Value& arcs = doc["edges"];
    assert(arcs.IsArray());
    // Populate vertex adjacencies.
    const int num_vertices = vertices.Size();
    adjacency_.resize(num_vertices);
    vertex_names_.resize(num_vertices);
    for (int i = 0; i < num_vertices; ++i) {
      assert(vertices[i].HasMember("name"));
      vertex_names_[i] = vertices[i]["name"].GetString();
      name_to_index_[vertex_names_[i]] = i;
      if (vertices[i].HasMember("state"))
        {
          assert(vertices[i]["state"].IsBool());
          if (vertices[i]["state"].GetBool()) // active state
            SetLiveVertex(vertex_names_[i]);
        }
    }
    const int num_arcs = arcs.Size();
    for (int i = 0; i < num_arcs; ++i) {
      const std::string v1 = arcs[i]["s"].GetString();
      const auto it1 = name_to_index_.find(v1);
      assert(it1 != name_to_index_.end());
      const int index1 = it1->second;
      const std::string v2 = arcs[i]["t"].GetString();
      const auto it2 = name_to_index_.find(v2);
      assert(it2 != name_to_index_.end());
      const int index2 = it2->second;
      // We treat all arcs as undirected.
      adjacency_[index1].insert(index2);
      adjacency_[index2].insert(index1);
    }
  }

  // Output a string representing the state of the graph
  std::string GetStateStr() {
    const size_t size = adjacency_.size();
    std::string state(size, '.');
    for (int i = 0; i < size; ++i) {
      if (state_.find(i) != state_.end()) {
        state[i] = '1';
      }
    }
    return state;
  }

  // Set state from a given state string
  // Inverse of GetStateStr();
  void SetState(const std::string& state) {
    assert(state.size() == adjacency_.size());
    state_.clear();
    for (int j = 0; j < state.size(); j++) {
      if (state[j] == '1') {
        state_.insert(j);
      }
    }
  }

  // Classical Conway rules
  static bool NewStateConway(bool current_state, int num_neighbors,
                      int num_live_neighbors) {
    if (num_live_neighbors < 2) return false;
    if (num_live_neighbors == 2) return current_state;
    if (num_live_neighbors == 3) return true;
    return false;
  }

  void SetNewStateFn(std::function<bool(bool, int, int)> fn) {
    new_state_fn_ = std::move(fn);
  }

  // Simulate one step of life on the underlying graph
  void Update() {
    std::unordered_set<int> next;
    for (int i = 0; i < adjacency_.size(); ++i) {
      const std::unordered_set<int>& neighbors = adjacency_[i];
      std::vector<int> live_neighbors(
        std::min(state_.size(), neighbors.size()));
      const int num_live = std::count_if(neighbors.begin(), neighbors.end(),
           [this](const int id) { return state_.find(id) != state_.end(); });
      bool live = state_.find(i) != state_.end();
      live = new_state_fn_(live, neighbors.size(), num_live);
      if (live) {
        next.insert(i);
      }
    }
    state_.swap(next);
  }

  void OutputLiveAnnotations() {
    for (int i = 0; i < adjacency_.size(); ++i) {
      std::cout << vertex_names_[i] << ": ";
      const std::unordered_set<int>& neighbors = adjacency_[i];
      for (int j : neighbors) {
        bool live = state_.find(j) != state_.end();
        std::string annotation = live ? "*" : "";
        std::cout << vertex_names_[j] << annotation << " ";
      }
      std::cout << std::endl;
    }
  }

  // Select two random Edges [a,b] and [c,d] such that
  // [a,b] and [c,d] can be removed and [a,c] and [b,d]
  // can be connected without violation of "each node has 8 neighbors"
  // property.
  std::optional<std::array<int, 4>> SelectRandomEdges() {
    const int a = rand() % adjacency_.size();
    const int c = rand() % adjacency_.size();
    if (a == c) return std::nullopt;
    auto& a_neighbors = adjacency_[a];
    if (a_neighbors.find(c) != a_neighbors.end()) {
      // We picked directly connected nodes. Try again.
      return std::nullopt;
    }
    auto a_iter = a_neighbors.begin();
    std::advance(a_iter, rand() % a_neighbors.size());
    const int b = *a_iter;

    auto& c_neighbors = adjacency_[c];
    if (c_neighbors.find(b) != c_neighbors.end()) {
      return std::nullopt;
    }
    auto c_iter = c_neighbors.begin();
    std::advance(c_iter, rand() % c_neighbors.size());
    const int d = *c_iter;

    auto& b_neighbors = adjacency_[b];
    if (b_neighbors.find(d) != b_neighbors.end()) {
      return std::nullopt;
    }
    return std::array<int, 4>{a, b, c, d};
  }

  void AddEdge(int a, int b) {
    assert(a != b);
    auto &a_neighbors = adjacency_[a];
    auto &b_neighbors = adjacency_[b];

    assert(a_neighbors.find(b) == a_neighbors.end());
    assert(b_neighbors.find(a) == b_neighbors.end());

    a_neighbors.insert(b);
    b_neighbors.insert(a);
  }

  void RemoveEdge(int a, int b) {
    size_t n = adjacency_[a].erase(b);
    assert(n == 1);
    n = adjacency_[b].erase(a);
    assert(n == 1);
  }

  bool ReWireRandomEdges() {
    auto e = SelectRandomEdges();
    if (!e) return false;
    auto &arr = e.value();
    int a = arr[0];
    int b = arr[1];
    int c = arr[2];
    int d = arr[3];
    std::cerr << "Rewire " << a << "<->" << b << 
        " and " << c << "<->" << d << " to " <<
        a << "<->" << c << " and " << b << "<->" << d <<
        std::endl;

    auto& a_neighbors = adjacency_[a];
    auto& b_neighbors = adjacency_[b];
    auto& c_neighbors = adjacency_[c];
    auto& d_neighbors = adjacency_[d];

    // Remove a<->b edge.
    RemoveEdge(a, b);
    assert(a_neighbors.size() == 7);
    assert(b_neighbors.size() == 7);

    // Remove c<->d edge.
    RemoveEdge(c, d);
    assert(c_neighbors.size() == 7);
    assert(d_neighbors.size() == 7);

    // Connect a<->c.
    a_neighbors.insert(c);
    c_neighbors.insert(a);

    // Connect b<->d.
    b_neighbors.insert(d);
    d_neighbors.insert(b);

    return true;
  }

  void AddEdges(int n) {
    while (n > 0) {
      auto e = SelectRandomEdges();
      if (!e) continue;
      auto& arr = e.value();
      AddEdge(arr[0], arr[1]);
      n -= 1;
      if (n <= 0) break;
      AddEdge(arr[2], arr[3]);
      n -= 1;
    }
  }

  void RemoveEdges(int n) {
    while (n > 0) {
      auto e = SelectRandomEdges();
      if (!e) continue;
      auto& arr = e.value();
      RemoveEdge(arr[0], arr[1]);
      n -= 1;
      if (n <= 0) break;
      RemoveEdge(arr[2], arr[3]);
      n -= 1;
    }
  }

  // Rewire n edges at random.
  void ReWire(int n) {
    for (int j = 0; j < n; j++) {
      while (!ReWireRandomEdges()) {
        // Failed for some reason. Try again.
      }
    }
  }

  void DumpToJSON(const std::string& filename) {
    std::ofstream outjson;
    outjson.open(filename);
    outjson << "{" << std::endl;
    outjson << "\"vertices\" : [" << std::endl;
    for (int index = 0; index < adjacency_.size(); ++index) {
      outjson << "{ \"name\" : \"" << vertex_names_[index] << "\"";
      bool live = state_.find(index) != state_.end();
      if (live) {
        outjson << ", \"state\" : true ";
      }
       
      outjson << "}";
      if (index < adjacency_.size() - 1)
        outjson << ",";
      outjson << std::endl;
    }
    outjson << "]," << std::endl; // end of vertices
    outjson << "\"edges\" : [" << std::endl;
    for (int index = 0; index < adjacency_.size(); ++index) {
      const auto& neighbors = adjacency_[index]; 
      int i = 0;
      for (int j : neighbors) {
        outjson << "{ \"s\" : \"" << vertex_names_[index] << "\","
                << " \"t\" : \"" << vertex_names_[j] << "\" }";
        if (!(index == adjacency_.size() - 1 && i == neighbors.size() - 1))
          outjson << ",";
        outjson << std::endl;
        i++;
      } 
    }
    outjson << "]}" << std::endl; // end of edges
    outjson.close();
  }

 private:
  std::function<bool(bool, int, int)> new_state_fn_ = NewStateConway;
  // A representation of the underlying graph.
  std::vector<std::unordered_set<int>> adjacency_;
  // Active vertices.
  std::unordered_set<int> state_;
  // Original vertex names.
  std::vector<std::string> vertex_names_;
  std::unordered_map<std::string, int> name_to_index_;

  // Add vertex 'i' to the live vertices.
  void SetLiveVertex(const std::string& name) {
    const auto it = name_to_index_.find(name);
    if (it == name_to_index_.end()) {
      std::cout << "Error: invalid vertex " << name << std::endl;
      exit(1);
    }

    const int index = it->second;
    state_.insert(index);
  }

};

#endif //GLIFE_H_
