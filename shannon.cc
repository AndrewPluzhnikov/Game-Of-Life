// Driver program
// Reads in graph definition from a .json file.
// Reads in states from given file
// For each state, runs the GoL until cycle or 10000 steps
// Compute and print shannon entropy.

#include <cmath>
#include <map>
#include <fstream>

#include "glife.h"

double ShannonEntropy(std::vector<std::string>::iterator begin,
                      std::vector<std::string>::iterator end) {
  assert(begin != end);

  const size_t num_nodes = begin->size();
  std::vector<int> v(num_nodes);
  for (auto it = begin; it != end; ++it) {
    assert(it->size() == num_nodes);
    for (int j = 0; j < num_nodes; j++) {
      v[j] += (*it)[j] == '1' ? 1 : 0;
    }
  }
  double result = 0.0;
  const int cycle_len = end - begin;
  for (const auto count : v) {
    assert(count <= cycle_len);
    double p1 = (double)count / cycle_len;
    assert(p1 <= 1.0);
    assert(0.0 <= p1);
    double p0 = 1.0 - p1;
    assert(p0 <= 1.0);
    assert(0.0 <= p0);

    if (p0 != 0) { result -= p0 * std::log2(p0); }
    if (p1 != 0) { result -= p1 * std::log2(p1); }
  }
  return result / num_nodes;
}

void usage(const char *argv0)
{
  std::cerr << "Usage: " << argv0 << "graph states" << std::endl;
  exit(1);
}

void OneSimulation(GLife& glife)
{
  const int max_steps = 100000;

  // Simulate GOL
  // Save intermediate states 
  // 1. to detect cycle
  std::map<std::string, int> states;
  // 2. to dump to file
  std::vector<std::string> doc_states;

  int cycle_begin = -1;
  int cycle_end = -1;
  int i;
  for (i = 0; i < max_steps; ++i) {
    const std::string state = glife.GetStateStr();
    // std::cout << i << ": " << state << std::endl;
    const auto it = states.find(state);
    if (it == states.end()) {
      // A new state.
      states[state] = i;
      glife.Update();
      doc_states.push_back(std::move(state));
    } else { 
      cycle_begin = it->second;
      std::cout << "Finite path: " << it->second;
      std::cout << ", Cycle length: " << i - cycle_begin << " ";
      cycle_end = i;

      // Make cycle repeat N=10 times.
      const int N = 10;
      std::vector<std::string> cycle(
          doc_states.begin() + it->second, doc_states.end());
      for (int j = 0; j < N; j++) {
        doc_states.insert(doc_states.end(), cycle.begin(), cycle.end());
      }
      break;
    }
  }
  double shannon_entropy = 0.0;
  if (i == max_steps) {
    // No cycle found within max_steps
    std::cout << "Finite path: unknown";
    std::cout << ", Cycle length: unknown" << std::endl;
    shannon_entropy = ShannonEntropy(doc_states.begin(), doc_states.end());
    printf("Shannon entropy: %6.2f\n", shannon_entropy);
  } else {
    assert(cycle_begin != -1);
    shannon_entropy = ShannonEntropy(doc_states.begin() + cycle_begin,
                                     doc_states.begin() + cycle_end);
    printf("Shannon entropy: %6.2f\n", shannon_entropy);
  }
}

int main(int argc, char *argv[]) 
{
  // Input torus size and file to dump it to
  std::string graph_filename, states_filename;

  if (argc == 3) {
    graph_filename = argv[1];
    states_filename = argv[2];
  } else {
    usage(argv[0]);
  }

  std::ifstream ifs(states_filename);
  while (true) {
    std::string state;
    ifs >> state;
    if (ifs.eof()) break;
    
    GLife glife(graph_filename);
    glife.SetState(state);
    OneSimulation(glife);
  }

  return 0;
}