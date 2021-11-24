// Driver program

#include <cmath>
#include <map>

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

// Save all states to json file
void SaveStates(std::string& filename,
                const std::vector<std::string>& states) {
  std::ifstream ifs(filename);
  if (!ifs)
    {
      std::cout << "Error: file " << filename << " does not exist." << std::endl;
      exit(1);
    } 
  IStreamWrapper isw(ifs);
  Document doc;
  doc.ParseStream(isw);
  Value& result = doc["result"];
  Value& doc_states = result["states"];
  int torusSize = doc["size"].GetInt();
  auto& allocator = doc.GetAllocator();
  for (int i = 0; i < states.size(); i++)
    {
      Value state(rapidjson::kArrayType);
      // store state into curr_state and add it to doc_states array
      for (int j = 0; j < torusSize * torusSize; j++)
        state.PushBack(states[i][j] == '1' ? 1 : 0, allocator);
      doc_states.PushBack(state, allocator);
    }
  // dump file
  std::ofstream ofs(filename);
  OStreamWrapper osw(ofs);
  PrettyWriter<OStreamWrapper> writer(osw);
  doc.Accept(writer);
}

// Annotate results in input json file
void SaveResults(std::string& filename,
                 int steps, int prefix, int cycle, double shannon_entropy) {
  std::ifstream ifs(filename);
  if (!ifs)
    {
      std::cout << "Error: " << filename << " does not exist." << std::endl;
      exit(1);
    } 
  IStreamWrapper isw(ifs);
  Document doc;
  doc.ParseStream(isw);
  Value& result = doc["result"];
  result["steps"].SetInt(steps);
  result["finite_path"].SetInt(prefix);
  result["cycle_length"].SetInt(cycle);
  if (result.FindMember("shannon_entropy") == result.MemberEnd()) {
    Value v;
    v.SetDouble(shannon_entropy);
    result.AddMember("shannon_entropy", v, doc.GetAllocator());
  } else {
    result["shannon_entropy"].SetDouble(shannon_entropy);
  }
  // dump file
  std::ofstream ofs(filename);
  OStreamWrapper osw(ofs);
  PrettyWriter<OStreamWrapper> writer(osw);
  doc.Accept(writer);
  std::cout << "Result saved in file " << filename << std::endl;
}

int main(int argc, char *argv[]) 
{
  int max_steps;
  // Input torus size and file to dump it to
  std::string filename;

  if (argc == 3) {
    filename = argv[1];
    max_steps = atoi(argv[2]);
  } else {
    std::cout << "Enter input graph filename: ";
    std::getline(std::cin, filename);
    std::cout << "Enter max steps: ";
    std::cin >> max_steps;
  }

  // Simulate GOL
  GLife glife(filename);
  // Save intermediate states 
  // 1. to detect cycle
  std::map<std::string, int> states;
  // 2. to dump to file
  std::vector<std::string> doc_states;

  int cycle_begin = -1;
  int i;
  for (i = 0; i < max_steps; ++i) {
    const std::string state = glife.GetStateStr();
    std::cout << i << ": " << state << std::endl;
    const auto it = states.find(state);
    if (it == states.end()) {
      // A new state.
      states[state] = i;
      glife.Update();
      doc_states.push_back(std::move(state));
    } else { 
      cycle_begin = it->second;
      std::cout << "Finite path: " << it->second;
      std::cout << ", Cycle length: " << i - cycle_begin << std::endl; 
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
    SaveResults(filename, max_steps, 0, 0, shannon_entropy);
  } else {
    assert(cycle_begin != -1);
    shannon_entropy = ShannonEntropy(doc_states.begin() + cycle_begin,
                                     doc_states.end());
    printf("Shannon entropy: %6.2f\n", shannon_entropy);
    SaveResults(filename, i, cycle_begin, i - cycle_begin, shannon_entropy);      
  }
  SaveStates(filename, doc_states);
  return 0;
}
