// Driver program

#include <map>
#include "glife.h"

bool IsResult(std::string& filename) {
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
  if (result.HasMember("finite_path") && !result["finite_path"].GetInt() &&
      result.HasMember("cycle_length") && !result["cycle_length"].GetInt())
    return false;
  return true;
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

  assert(states[0].length() == torusSize * torusSize);
  for (int i = 0; i < states.size(); i++)
    {
      Value curr_state(doc_states[0], doc.GetAllocator());
      // store state into curr_state and add it to doc_states array
      for (int j = 0; j < torusSize * torusSize; j++)
        curr_state.GetArray()[j] = states[i][j] == '1' ? 1 : 0;
      doc_states.PushBack(curr_state, doc.GetAllocator());
    }
  // dump file
  std::ofstream ofs(filename);
  OStreamWrapper osw(ofs);
  PrettyWriter<OStreamWrapper> writer(osw);
  doc.Accept(writer);
}

// Annotate results in input json file
void SaveResults(std::string& filename,
                 int steps, int prefix, int cycle) {
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
  // Do not run if result already available in 'filename' 
  if (IsResult(filename)) {
    std::cout << "Result is already available in " << filename << std::endl; 
    return 1;
  }

  // Simulate GOL
  GLife glife(filename);
  // Save intermediate states 
  // 1. to detect cycle
  std::map<std::string, int> states;
  // 2. to dump to file
  std::vector<std::string> doc_states(max_steps);

  for (int i = 0; i < max_steps; ++i) {
    const std::string state = glife.GetStateStr();
    std::cout << i << ": " << state << std::endl;
    const auto it = states.find(state);
    if (it == states.end()) {
      // A new state.
      states[state] = i;
      glife.Update();
      if (i) 
        doc_states[i-1] += state;
    } else { 
      std::cout << "Finite path: " << it->second;
      std::cout << ", Cycle length: " << i - it->second << std::endl; 
      SaveResults(filename, i, it->second, i - it->second);      
      return 1;
    }
  }
  SaveStates(filename, doc_states);
  std::cout << "Finite path: unknown";
  std::cout << ", Cycle length: unknown" << std::endl;

  // No cycle found within max_steps
  SaveResults(filename, max_steps, 0, 0);
  return 0;
}
