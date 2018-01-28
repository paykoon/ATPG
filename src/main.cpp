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

int testCircuit(char *blifFile) {
  circuit *pCircuit = new circuit(blifFile);
  pCircuit->CNFGenerator();
  delete pCircuit;
}

/*
void testGlucose() {
  glucose *SATsolver = new glucose();
  int test[3] = {1, 2, -3};
  vector<int> SATresult;
  int result = SATsolver->runGlucose(test, SATresult);
  if(result) {
    cout << "------SAT------" << endl;
  }
  else {
    cout << "------UNSAT------" << endl;
  }
  for (int i = 0; i < SATresult.size(); i++) {
    cout << SATresult[i] << ' ' << endl;
  }
  delete SATsolver;
}
*/

int main(int argc, char **argv){
  if(argc < 2){
    cout << "No input file" << endl;
    return 0;
  }

  char *blifFile=argv[1];
  testCircuit(blifFile);
 // testGlucose();

  return 1;
}
