// Generate N unique initial states for MxM torus with %p live cells
//
// Usage: ./genstates N M p
//

#include <iostream>
#include <cmath>
#include <set>
#include <string>
#include <time.h>
#include <unistd.h>

void usage(const char *argv0)
{
  std::cerr << "Usage: " << argv0 << " num-states torus-size percent-live" << std::endl;
  exit(1);
}

std::string gen_state(int size, double p)
{
  std::string state(size, '0');
  int to_go = std::ceil(p * size);
  while (to_go > 0) {
    for (int j = 0; j < size && to_go > 0; j++) {
      const double a = (double)rand() / RAND_MAX;
      if (a < p && state[j] == '0') {
        state[j] = '1';
        to_go -= 1;
      }
    }
  }
  return state;
}

int main(int argc, char *argv[])
{
  srand(time(NULL));
  std::set<std::string> states;

  if (argc < 4) usage(argv[0]);
  int N = atoi(argv[1]);
  int M = atoi(argv[2]);
  double p = atof(argv[3]);

  while (states.size() < N) {
    states.insert(gen_state(M * M, p));
  }
  for (auto& s : states) {
    std::cout << s << std::endl;
  }
}
