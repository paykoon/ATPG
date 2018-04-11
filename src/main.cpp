#include "Circuit.h"
#include "Gate.h"
#include "ATPG.h"
#include "glucose.h"
#include "time.h"
#include <vector>
#include <set>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;
using namespace Circuit;
using namespace Glucose;
using namespace ATPG;

int main(int argc, char **argv){
  if(argc < 3){
    cout << "Miss some input file" << endl;
    return 0;
  }


  char *blifFile = argv[1];
  char *patternFile = argv[2];
  // build the circuit
  circuit *pCircuit = new circuit(blifFile);
  atpg *Tester = new atpg(pCircuit, patternFile);

  return 1;
}
