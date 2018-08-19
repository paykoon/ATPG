#include "Circuit.h"
#include "Gate.h"
#include "ATPG.h"
#include "glucose.h"
#include "Testgenebysat.h"
#include "CircuitSimulation.h"
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
using namespace Simulation;


int main(int argc, char **argv){
  if(argc < 3){
    cout << "Miss some input file" << endl;
    return 0;
  }

  double startTime, endTime;
  startTime = clock();
  char *blifFile = argv[1];
  char *patternFile = argv[2];
  char *TSAFile = argv[3];

  circuit *pCircuit = new circuit(blifFile);
  testgenebysat *testBySAT = new testgenebysat(pCircuit);
  simulation *simulate = new simulation(pCircuit);
  atpg *ATPGInit = new atpg(pCircuit, patternFile, simulate, testBySAT, TSAFile);
  endTime = clock();
  cout << "Total execution time: " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds.\n\n" << endl;

  delete pCircuit;
  delete testBySAT;
  delete simulate;
  delete ATPGInit;
  return 1;
}
