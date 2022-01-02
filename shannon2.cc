// Driver program
// Reads in graph definition from a .json file.
// Reads in states from given file
// For each state, runs the GoL until cycle or 10000 steps
// Compute and print shannon entropy.

#include <errno.h>
#include <sys/stat.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <future>

#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/strip.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "glife.h"

ABSL_FLAG(bool, verbose, false, "Be verbose");
ABSL_FLAG(int, num_threads, 1, "Number of threads to use");
ABSL_FLAG(int, num_rewire, 0, "Number of rewirings to perform");
ABSL_FLAG(int, num_remove, 0, "Number of edges to remove");
ABSL_FLAG(int, num_add, 0, "Number of edges to add");
ABSL_FLAG(int, max_steps, 4000, "Number of edges to add");
ABSL_FLAG(double, density_threshold, 0, "Use density rule with the given threshold");
ABSL_FLAG(std::vector<std::string>, underpopulation, {},
          "Use underpopulation rule with given thresholds");
ABSL_FLAG(std::vector<std::string>, overpopulation, {},
          "Use overpopulation rule with given thresholds");

// Modified Conway rules.
ABSL_FLAG(std::vector<std::string>, conway, {},
          "Use modified Conway rule with 3 given thresholds");

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
  std::cerr << "Usage: " << argv0 << " graph states" << std::endl;
  exit(1);
}


struct SimResult {
  double entropy = 0.0;  // Shannon entropy.
  int cycle_len = -1;
  int max_steps = 0;
};

SimResult OneSimulation(GLife& glife)
{
  const int max_steps = absl::GetFlag(FLAGS_max_steps);
  SimResult result;
  std::vector<std::string> states_v;

  // Simulate GOL
  // Save intermediate states 
  // 1. to detect cycle
  absl::flat_hash_map<absl::string_view, int> states;

  int cycle_begin = -1;
  int cycle_end = -1;
  int i;
  for (i = 0; i < max_steps; ++i) {
    const std::string state = glife.GetStateStr();
    // std::cout << i << ": " << state << std::endl;
    const auto it = states.find(state);
    const bool found = it != states.end();
    if (!found) {
      // A new state.
      states_v.push_back(std::move(state));
      states.insert({states_v.back(), i});
      glife.Update();
    } else { 
      cycle_begin = it->second;
      // std::cout << "Finite path: " << it->second;
      // std::cout << ", Cycle length: " << i - cycle_begin << " ";
      cycle_end = i;

      std::vector<std::string> cycle(
          states_v.begin() + it->second, states_v.end());
      while (states_v.size() < max_steps) {
        states_v.insert(states_v.end(), cycle.begin(), cycle.end());
      }
      assert(states_v.size() >= max_steps);
      states_v.resize(max_steps);
      break;
    }
  }
  result.max_steps = i;
  double& entropy = result.entropy;
  if (i == max_steps) {
    // No cycle found within max_steps
    // std::cout << "Finite path: unknown";
    // std::cout << ", Cycle length: unknown" << std::endl;
    entropy = ShannonEntropy(states_v.begin(), states_v.end());
    // printf("Shannon entropy: %6.2f\n", shannon_entropy);
  } else {
    assert(cycle_begin != -1);
    entropy = ShannonEntropy(states_v.begin() + cycle_begin,
                             states_v.begin() + cycle_end);
    result.cycle_len = cycle_end - cycle_begin;
    // printf("Shannon entropy: %6.2f\n", shannon_entropy);
  }
  return result;
}

void SaveArgs(const std::string& outd, int argc, char *argv[])
{
  const std::string outf = outd + "/invocation.txt";
  std::ofstream ofs(outf);
  for (int i = 0; i < argc; i++) {
    ofs << argv[i] << std::endl;
  }
}

void SaveTo(const std::string& outd, absl::string_view filename,
            const std::string& contents)
{
  const auto csv = absl::StrCat(outd, "/", filename);
  std::ofstream ofs(csv);
  ofs << contents << std::endl;
}

std::string ConcatArgs(int argc, char *argv[]) 
{
  absl::string_view argv0 = absl::StripPrefix(argv[0], "./");
  std::string result = absl::StrCat(argv0, "___");
  for (int i = 1; i < argc; i++) {
    absl::StrAppend(&result, argv[i], "___");
  }
  return absl::StrReplaceAll(result, {{"/", "_"}});
}

int main(int argc, char *argv[]) 
{
  srand(time(NULL));

  const auto args = absl::ParseCommandLine(argc, argv);

  // Input torus size and file to dump it to
  std::string graph_filename, states_filename;

  if (args.size() == 3) {
    graph_filename = args[1];
    states_filename = args[2];
  } else {
    usage(argv[0]);
  }

  GLife zygote(graph_filename);

  const double density_threshold = absl::GetFlag(FLAGS_density_threshold);
  if (density_threshold > 0) {
    zygote.SetNewStateFn([density_threshold](bool live, int num_neighbors,
                                             int num_live_neighbors) {
      const double density = (double) num_live_neighbors / num_neighbors;
      if (density > density_threshold) {
        return !live;
      }
      return live;
    });
  }
  const auto underpop = absl::GetFlag(FLAGS_underpopulation);
  const auto overpop = absl::GetFlag(FLAGS_overpopulation);
  const auto conway = absl::GetFlag(FLAGS_conway);
  if (!underpop.empty()) {
    assert(density_threshold == 0);
    assert(overpop.empty());
    assert(underpop.size() == 2);
    int i0 = atoi(underpop[0].c_str());
    int i1 = atoi(underpop[1].c_str());
    zygote.SetNewStateFn([i0, i1](bool live, int num_neighbors,
                                  int num_live_neighbors) {
      // Dead vertex becomes alive if it has at least i0 live neighbors.
      // Live vertex remains alive if it has at least i1 live neighbors.
      if (!live && num_live_neighbors >= i0) return true;
      if (live && num_live_neighbors >= i1) return true;
      return false;
    });
  }

  if (!overpop.empty()) {
    assert(density_threshold == 0);
    assert(underpop.empty());
    assert(overpop.size() == 2);
    const int i0 = atoi(overpop[0].c_str());
    const int i1 = atoi(overpop[1].c_str());
    zygote.SetNewStateFn([i0, i1](bool live, int num_neighbors,
                                  int num_live_neighbors) {
      // Dead vertex becomes alive if it has at least i0 live neighbors.
      // Live vertex remains alive if it has at most i1 live neighbors.
      if (!live && num_live_neighbors >= i0) return true;
      if (live && num_live_neighbors <= i1) return true;
      return false;
    });
  }

  if (!conway.empty()) {
    assert(conway.size() == 3);
    const int i0 = atoi(conway[0].c_str());
    const int i1 = atoi(conway[1].c_str());
    const int i2 = atoi(conway[2].c_str());

    zygote.SetNewStateFn([i0, i1, i2](bool live, int num_neighbors,
                                      int num_live_neighbors) {
      // Dead vertex becomes alive if it has exactly i1 live neighbors.
      if (num_live_neighbors == i1) return true;

      // Dead if i0 or fewer or more than i2 neighbors.
      if (num_live_neighbors <= i0 || num_live_neighbors > i2) return false;

      // Otherwise remain in current state.
      return live;
    });

  }

  const auto verbose = absl::GetFlag(FLAGS_verbose);

  const int num_rewire = absl::GetFlag(FLAGS_num_rewire);
  const int num_remove = absl::GetFlag(FLAGS_num_remove);
  const int num_add = absl::GetFlag(FLAGS_num_add);
  if (num_rewire > 0) {
    zygote.ReWire(num_rewire);
  } else if (num_remove > 0) {
    zygote.RemoveEdges(num_remove);
  } else if (num_add > 0) {
    zygote.AddEdges(num_remove);
  }

  const std::string outd = "results___" + ConcatArgs(argc, argv) + std::to_string(time(NULL));
  if (0 != mkdir(outd.c_str(), 0777)) {
    std::cerr << argv[0] << " mkdir(" << outd << "): " << strerror(errno) << std::endl;
    exit(1);
  }
  if (verbose) {
    std::cout << "Results in " << outd << std::endl;
  }
  SaveArgs(outd, argc, argv);

  if (num_rewire > 0 || num_remove > 0 || num_add > 0) {
    std::string out_graph = outd + "/" + graph_filename;
    zygote.DumpToJSON(out_graph);
    if (num_rewire > 0) {
      std::cerr << "Wrote " << out_graph << " with " << num_rewire << " rewirings" << std::endl;
    } else if (num_remove > 0) {
      std::cerr << "Wrote " << out_graph << " with " << num_remove << " removals" << std::endl;
    } else if (num_add > 0) {
      std::cerr << "Wrote " << out_graph << " with " << num_add << " additions" << std::endl;
    }
  }

  const int num_threads = absl::GetFlag(FLAGS_num_threads);
  std::vector<SimResult> results;

  auto start = std::chrono::steady_clock::now();
  int count_states = 0;
  std::ifstream ifs(states_filename);

  bool done = false;
  while (!done) {
    std::vector<std::future<SimResult>> futures;
    for (int j = 0; j < num_threads; j++) {
      std::string state;
      ifs >> state;
      if (ifs.eof()) {
        done = true;
        break;
      }
    
      auto future = std::async(std::launch::async,
                               [state = std::move(state), &zygote]() {
                                 GLife glife(zygote);
                                 glife.SetState(state);
                                 return OneSimulation(glife);
                               });
      futures.push_back(std::move(future));
    }
    for (auto& future : futures) {
      results.push_back(future.get());
    }

    count_states += num_threads;

    if (verbose && (count_states % (100 * num_threads)) == 0) {
      auto end = std::chrono::steady_clock::now();
      std::cerr << std::setw(4) << count_states << " Elapsed time in milliseconds: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
        << " ms" << std::endl;
      start = end;
    }
  }

  std::array<int, 1001> histogram = {};
  for (const auto& result: results) {
    int bucket_index = (int)(result.entropy / 0.001);
    assert(bucket_index < histogram.size());
    histogram[bucket_index] += 1;
  }

  SaveTo(outd, "entropy_histogram.csv", absl::StrJoin(histogram, ","));
  SaveTo(outd, "entropy.csv", absl::StrJoin(results, ",",
                                            [](std::string *out, const SimResult& r) {
                                                absl::StrAppend(out, r.entropy);
                                            }));
  SaveTo(outd, "max_steps.csv", absl::StrJoin(results, ",",
                                              [](std::string *out, const SimResult& r) {
                                                  absl::StrAppend(out, r.max_steps);
                                              }));
  SaveTo(outd, "cycle_len.csv", absl::StrJoin(results, ",",
                                              [](std::string *out, const SimResult& r) {
                                                  absl::StrAppend(out, r.cycle_len);
                                              }));

  return 0;
}
