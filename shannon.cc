// Driver program
// Reads in graph definition from a .json file.
// Reads in states from given file
// For each state, runs the GoL until cycle or 10000 steps
// Compute and print shannon entropy.

#include <cmath>
#include <chrono>
#include <unordered_map>
#include <fstream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "glife.h"

ABSL_FLAG(bool, verbose, false, "Be verbose");

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

double OneSimulation(GLife& glife)
{
  const int max_steps = 1000;

  const bool verbose = absl::GetFlag(FLAGS_verbose);

  // Simulate GOL
  // Save intermediate states 
  // 1. to detect cycle
  std::unordered_map<std::string, int> states;
  // 2. to dump to file
  std::vector<std::string> doc_states;

  int cycle_begin = -1;
  int cycle_end = -1;
  int i;
  for (i = 0; i < max_steps; ++i) {
    const std::string state = glife.GetStateStr();
    // std::cout << i << ": " << state << std::endl;
    const auto [it, inserted] = states.insert({state, i});
    if (inserted) {
      // A new state.
      glife.Update();
      doc_states.push_back(std::move(state));
    } else { 
      cycle_begin = it->second;
      if (verbose) {
        std::cout << "Finite path: " << it->second;
        std::cout << ", Cycle length: " << i - cycle_begin << " ";
      }
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
    shannon_entropy = ShannonEntropy(doc_states.begin(), doc_states.end());
    if (verbose) {
      std::cout << "Finite path: unknown";
      std::cout << ", Cycle length: unknown" << std::endl;
      printf("Shannon entropy: %6.2f\n", shannon_entropy);
    }
  } else {
    assert(cycle_begin != -1);
    shannon_entropy = ShannonEntropy(doc_states.begin() + cycle_begin,
                                     doc_states.begin() + cycle_end);
    if (verbose) {
      printf("Shannon entropy: %6.2f\n", shannon_entropy);
    }
  }
  return shannon_entropy;
}

int main(int argc, char *argv[]) 
{
  const auto args = absl::ParseCommandLine(argc, argv);

  // Input torus size and file to dump it to
  std::string graph_filename, states_filename;

  if (args.size() == 3) {
    graph_filename = args[1];
    states_filename = args[2];
  } else {
    usage(argv[0]);
  }

  const bool verbose = absl::GetFlag(FLAGS_verbose);

  GLife zygote(graph_filename);

  auto start = std::chrono::steady_clock::now();
  for (double mu = 0.1; mu < 1.0; mu += 0.1) {
    int count_states = 0;
    std::ifstream ifs(states_filename);
    double average_entropy = 0.0;
    while (true) {
      std::string state;
      ifs >> state;
      if (ifs.eof()) break;
    
      GLife glife(zygote);
      glife.SetState(state);
      glife.SetNewStateFn([mu](bool live, int num_neighbors, int num_live_neighbors) {
        const double density = (double)num_live_neighbors / num_neighbors;
        if (density > mu) {
          // flip
          return !live;
        }
        // stay the same
        return live;
      });
      average_entropy += OneSimulation(glife);
      count_states += 1;
      if (verbose && (count_states % 10) == 0) {
        auto end = std::chrono::steady_clock::now();
        std::cout << "Elapsed time in milliseconds: "
          << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
          << " ms" << std::endl;
        start = end;
      }
    }
    average_entropy /= count_states;
    printf("%6.4f %8.4f\n", mu, average_entropy);
  }

  return 0;
}
