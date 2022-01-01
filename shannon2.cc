// Driver program
// Reads in graph definition from a .json file.
// Reads in states from given file
// For each state, runs the GoL until cycle or 10000 steps
// Compute and print shannon entropy.

#include <errno.h>
#include <sys/stat.h>

#include <cmath>
#include <chrono>
#include <unordered_map>
#include <fstream>
#include <iomanip>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "glife.h"

ABSL_FLAG(bool, verbose, false, "Be verbose");
ABSL_FLAG(int, num_rewire, 0, "Number of rewirings to perform");
ABSL_FLAG(int, num_remove, 0, "Number of edges to remove");
ABSL_FLAG(int, num_add, 0, "Number of edges to add");

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
  std::vector<std::string> states;
};

SimResult OneSimulation(GLife& glife)
{
  const int max_steps = 4000;
  SimResult result;

  // Simulate GOL
  // Save intermediate states 
  // 1. to detect cycle
  std::unordered_map<std::string, int> states;

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
      result.states.push_back(std::move(state));
    } else { 
      cycle_begin = it->second;
      // std::cout << "Finite path: " << it->second;
      // std::cout << ", Cycle length: " << i - cycle_begin << " ";
      cycle_end = i;

      std::vector<std::string> cycle(
          result.states.begin() + it->second, result.states.end());
      while (result.states.size() < max_steps) {
        result.states.insert(result.states.end(), cycle.begin(), cycle.end());
      }
      assert(result.states.size() >= max_steps);
      result.states.resize(max_steps);
      break;
    }
  }
  result.max_steps = i;
  double& entropy = result.entropy;
  if (i == max_steps) {
    // No cycle found within max_steps
    // std::cout << "Finite path: unknown";
    // std::cout << ", Cycle length: unknown" << std::endl;
    entropy = ShannonEntropy(result.states.begin(), result.states.end());
    // printf("Shannon entropy: %6.2f\n", shannon_entropy);
  } else {
    assert(cycle_begin != -1);
    entropy = ShannonEntropy(result.states.begin() + cycle_begin,
                             result.states.begin() + cycle_end);
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
  std::string result;
  for (int i = 0; i < argc; i++)
    absl::StrAppend(&result, argv[i], ";");
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

  const std::string outd = "results;" + ConcatArgs(argc, argv) + std::to_string(time(NULL));
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

  std::vector<double> entropies;
  std::vector<int> max_steps;
  std::vector<int> cycle_len;

  auto start = std::chrono::steady_clock::now();
  std::array<int, 1001> histogram = {};
  int count_states = 0;
  std::ifstream ifs(states_filename);
  while (true) {
    std::string state;
    ifs >> state;
    if (ifs.eof()) break;
    
    GLife glife(zygote);
    glife.SetState(state);

    const auto result = OneSimulation(glife);
    entropies.push_back(result.entropy);
    max_steps.push_back(result.max_steps);
    cycle_len.push_back(result.cycle_len);

    count_states += 1;

    int bucket_index = (int)(result.entropy / 0.001);
    if (bucket_index > histogram.size()) {
      std::cerr << "bucket_index " << bucket_index << " too high" << std::endl;
    } else {
      histogram[bucket_index] += 1;
    }
      
    if (verbose && (count_states % 100) == 0) {
      auto end = std::chrono::steady_clock::now();
      std::cerr << std::setw(4) << count_states << " Elapsed time in milliseconds: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
        << " ms" << std::endl;
      start = end;
    }
  }

  SaveTo(outd, "entropy_histogram.csv", absl::StrJoin(histogram, ","));
  SaveTo(outd, "entropy.csv", absl::StrJoin(entropies, ","));
  SaveTo(outd, "max_steps.csv", absl::StrJoin(max_steps, ","));
  SaveTo(outd, "cycle_len.csv", absl::StrJoin(cycle_len, ","));

  return 0;
}
