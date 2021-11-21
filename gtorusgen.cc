// Torus generator

#include <sstream>
#include <vector>
#include <bits/stdc++.h>

#include "gtorus.h"

int main() 
{
  // Input torus size, init state and file to dump the graph to
  std::string filename;
  std::cout << "Enter output filename (e.g. out.json): ";
  std::getline(std::cin, filename);

  std::vector<std::string> live_vertices;
  std::string state;
  std::cout << "Enter live vertices (e.g. 0_3 1_3): ";
  std::getline(std::cin, state);
  std::stringstream ss(state);
  std::string intermediate;
  while(getline(ss, intermediate, ' ')) {
    live_vertices.push_back(intermediate);
  }

  int torus_size = 3;
  std::cout << "Enter torus size (e.g. 3 for 3x3): ";
  std::cin >> torus_size;
  
  // Create a torus of 'torus_size' and dump to 'filename'
  GTorus gtorus(torus_size); 
  // Activate vertices.
  for(int i = 0; i < live_vertices.size(); i++)
    gtorus.SetLiveVertex(live_vertices[i]);
  gtorus.DumpToJSON(filename);

  return 0;
}
