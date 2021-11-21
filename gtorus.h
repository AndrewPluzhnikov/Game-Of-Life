#ifndef GTORUS_H_
#define GTORUS_H_

// Torus

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>
#include <math.h>

using std::ofstream;
using std::endl;

class GTorus{
 public:

  // Generate a torus of size n x n.
  explicit GTorus(int n) {

    adjacency_.resize(n * n);
    vertex_names_.resize(n * n);
  
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        const int index = i + n * j;
        // Populate adjacencies
        // Each vertex has 8 neighbors 
        adjacency_[index].insert(i + n * ((j + 1) % n));
        adjacency_[index].insert(i + n * ((n + j - 1) % n));
        adjacency_[index].insert(((i + 1) % n) + n * j);
        adjacency_[index].insert(((n + i - 1) % n) + n * j);
        adjacency_[index].insert(((i + 1) % n) + n * ((j + 1) % n));
        adjacency_[index].insert(((n + i - 1) % n) + n * ((n + j - 1) % n));
        adjacency_[index].insert(((i + 1) % n) + n * ((n + j - 1) % n));
        adjacency_[index].insert(((n + i - 1) % n) + n * ((j + 1) % n));
        // Vertex name.      
        std::stringstream ss;
        ss << i << "_" << j;
        std::string name;
        ss >> name;
        vertex_names_[index] = name;
        name_to_index_[name] = index; 
      }
    }
  }

  // 'filename' contains the specification of the generated graph, 
  // including its state in JSON format 
  void DumpToJSON(const std::string& filename) {

    ofstream outjson;
    outjson.open(filename);
    outjson << "{" << endl;
    outjson << "\"name\" : \"Torus\"," << endl;
    outjson << "\"size\" : " << sqrt(adjacency_.size()) << "," << endl;
    outjson << "\"vertices\" : [" << endl;
    for (int index = 0; index < adjacency_.size(); ++index) {
      outjson << "{ \"name\" : \"" << vertex_names_[index] << "\"";
      bool live = state_.find(index) != state_.end();
      if (live) {
        outjson << ", \"state\" : true ";
      }
       
      outjson << "}";
      if (index < adjacency_.size() - 1)
        outjson << ",";
      outjson << endl;
    }
    outjson << "]," << endl; // end of vertices
    outjson << "\"edges\" : [" << endl;
    for (int index = 0; index < adjacency_.size(); ++index) {
      const std::set<int>& neighbors = adjacency_[index]; 
      int i = 0;
      for (int j : neighbors) {
        outjson << "{ \"s\" : \"" << vertex_names_[index] << "\","
                << " \"t\" : \"" << vertex_names_[j] << "\" }";
        if (!(index == adjacency_.size() - 1 && i == neighbors.size() - 1))
          outjson << ",";
        outjson << endl;
        i++;
      } 
    }
    outjson << "]," << endl; // end of edges
    // empty result; populated at end of simulation
    outjson << "\"result\" : {" << endl;
    outjson << "\"states\" : [" << endl;
    outjson << "[ "; 
    std::string state;
    for (int i = 0; i < adjacency_.size(); ++i) {
      bool live = state_.find(i) != state_.end();
      outjson << live ? "1" : "0";
      if (i < adjacency_.size() - 1)
        outjson << ",";
    }
    outjson << " ]" << endl;
    outjson << "]," << endl;
    outjson << "\"steps\" : " << 0 << "," << endl;
    outjson << "\"finite_path\" : " << 0 << "," << endl;
    outjson << "\"cycle_length\" : " << 0 << endl;
    outjson << "}" << endl; // end of result
    outjson << "}" << endl;
    outjson.close();
  }

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

 private:
  // A representation of the underlying graph.
  std::vector<std::set<int>> adjacency_;
  // Active vertices.
  std::set<int> state_;
  // Original vertex names.
  std::vector<std::string> vertex_names_;
  std::map<std::string, int> name_to_index_;
  // TODO: represent embedding.
};

#endif //GTORUS_H_
