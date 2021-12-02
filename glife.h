#ifndef GLIFE_H_
#define GLIFE_H_

// Life on a Graph.

#include <algorithm>
#include <fstream>
#include <iostream>
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
  explicit GLife(const std::string& filename, double mu) {
    mu_ = mu;
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

  void SetMu(double mu) { mu_ = mu; }

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
      double density = (double)num_live /  neighbors.size();
      if (density > mu_) {
        // flip
        if (!live) {
           next.insert(i);
        }
      } else {
        // keep same
        if (live) {
           next.insert(i);
        }
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

 private:
  double mu_;
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
