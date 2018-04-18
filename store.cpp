
// ****need to add visited****
void findSSAFaultsSameTestVector(int faultID, vector<int> &testVector, set<int> &sameFaults, set<int> &faults, set<int> &visited) {
  int gateID = (faultID >> 3);
  int port = (faultID >> 1) & 3;
  int stuckat = faultID & 1;
  gate *faultGate = theCircuit[gateID];
  // mark the gates that are located in the propagation path
  vector<int> newFaults;
  newFaults.push_back(faultID);
  if(propgateFault(newFaults, testVector) == 0) return;
  // mark previous gate as faulty. it will simplify the function.
  findFaultsSameTest_helper(faultGate, sameFaults);
  visited.insert(sameFaults.begin(), sameFaults.end());
}


// 1. propagate the PI
// 2. find all the faults (in PI) that can be detected by the test pattern
// 3. propgate those faults to PO to eliminate the same faults
void findSSAFaultsSameTestVector(int curFaultID, vector<int> &testVector, set<int> &sameFaults, set<int> &faults, set<int> &visited) {
  resetAllVisitedFaultyOut();
  resetFaultsInCircuit();
  // step 1 propagate the PI
  assignPIs(testVector);
  propagatePI();
  set<int> sameFaultsinPI;
  // step 2 find all the faults (in PI) that can be detected by the test pattern
  sameFaultsinPI.insert(curFaultID);
  for (int i = 0; i < PISize; i++) {
    gate *PIGate = theCircuit[i];
    for (auto *connectedGate : PIGate->fanout) { // check the gate connect to PI
      if (connectedGate->gateType != aig) continue;
      if (connectedGate->fanin1 == PIGate) {
        if (connectedGate->fanin2->outValue == 1) { // side value == 1
          int gateID = connectedGate->gateID;
          int port = 1;
          int stuckat = 0;
          sameFaultsinPI.insert(getFaultID(gateID, port, stuckat));
          stuckat = 1;
          sameFaultsinPI.insert(getFaultID(gateID, port, stuckat));
        }
      } else { // fanout->fanin2 == PIGate
        if (connectedGate->fanin1->outValue == 1) { // side value == 1
          int gateID = connectedGate->gateID;
          int port = 2;
          int stuckat = 0;
          sameFaultsinPI.insert(getFaultID(gateID, port, stuckat));
          stuckat = 1;
          sameFaultsinPI.insert(getFaultID(gateID, port, stuckat));
        }
      }
    }
  }
  // step 3 try to propgate those faults to PO to eliminate the same faults
  // and avoid the replicate checking, we will mark the visited gate.
  // ***noted the faults in "sameFaultsinPI" may not be propagated to output. need to check
  for (auto faultID : sameFaultsinPI) {
    if (visited.find(faultID) != visited.end()) continue;
    int gateID = (faultID >> 3);
    int port = (faultID >> 1) & 3;
    int stuckat = faultID & 1;
    gate *faultGate = theCircuit[gateID];
    // mark the gates that are located in the propagation path
    vector<int> newFaults;
    newFaults.push_back(faultID);
    // if this fault cannot be propagated to output by "testVector", skip
    // mark the propagation path by setting faultyOut to true.
    if (propgateFault(newFaults, testVector) == 0) continue;
    findFaultsSameTest_helper(faultGate, sameFaults);
    visited.insert(sameFaults.begin(), sameFaults.end());
  }
}




void findSSAFaultsSameTestVector(int curFaultID, vector<int> &testVector, set<int> &sameFaults, set<int> &allFaults, set<int> &visited) {
  sameFaults.insert(curFaultID);
  visited.insert(curFaultID);
  for (auto faultID : allFaults) {
    if (visited.find(faultID) != visited.end()) continue;
    vector<int> newFaults;
    newFaults.push_back(faultID);
    if(propgateFault(newFaults, testVector) == 1) {
      sameFaults.insert(faultID);
      visited.insert(faultID);
    }
  }
}


// recursive function to find the faults that can be detected by the same vector
// can work for any fault model.
// Note: need to run "propgateFault" function first to mark the propagation path.
// the first "curGate" should be the faulty gate
// get: set<int> &faultsSameVector
void findFaultsSameTest_helper(gate *curGate, set<int> &faultsSameVector, set<int> &faultList, int iniFaultID) {
  // 1. consider the input
  if (curGate->gateType == PO || curGate->gateType == PI || curGate->gateType == constant || curGate->gateType == bufInv) {
    // faults here will not be blocked.
    // input1
    int gateID = curGate->gateID;
    int port = 1;
    int activate = curGate->fanin1->outValue;
    int stuckat = 1 - activate;
    int faultID = getFaultID(gateID, port, stuckat);
    if (faultList.find(faultID) != faultList.end()) {
      faultsSameVector.insert(faultID);
    }
    port = 3;
    faultID = getFaultID(gateID, port, stuckat);
    if (faultList.find(faultID) != faultList.end()) {
      faultsSameVector.insert(faultID);
    }
    // base case: the gate has no fanout-->PO
    if (curGate->gateType == PO) {
      return;
    }
  } else if (curGate->gateType == aig) {
    // need to consider the fault blocking
    // input1
    int gateID = curGate->gateID;
    int port = 1;
    int activate = curGate->fanin1->outValue;
    int stuckat = 1 - activate;
    int faultID = getFaultID(gateID, port, stuckat);
    // check the side value
    if (curGate->fanin2->outValue == curGate->invIn2 && faultList.find(faultID) != faultList.end()) {
      faultsSameVector.insert(faultID);
    }
    // input2
    port = 2;
    activate = curGate->fanin2->outValue;
    stuckat = 1 - activate;
    faultID = getFaultID(gateID, port, stuckat);
    // check the side value
    if (curGate->fanin1->outValue == curGate->invIn1 && faultList.find(faultID) != faultList.end()) {
      faultsSameVector.insert(faultID);
    }
    // output
    port = 3;
    activate = curGate->outValue;
    stuckat = 1 - activate;
    faultID = getFaultID(gateID, port, stuckat);
    if (faultList.find(faultID) != faultList.end()) {
      faultsSameVector.insert(faultID);
    }
  }
  // TO DO..add other types of gate
  // go to the next level
  for (auto fanout : curGate->fanout) {
    // due to the side value, some gates may not propagate fault.
    // only go the gate that propagate the fault
    if (fanout->faultyOut == true) {
      findFaultsSameTest_helper(fanout, faultsSameVector, faultList, iniFaultID);
    }
  }
}


void generateSSAFTest(set<int> &faults) {
  set<int> visited;
  for (set<int>::iterator iter = faults.begin(); iter != faults.end(); iter++) {
    int faultID = *iter;
    int gateID = faultID >> 3;
    int port = (faultID >> 2) & 3;
    gate *faultGate = theCircuit[gateID];
    int faninType = (port == 1) ? faultGate->fanin1->gateType : faultGate->fanin2->gateType;
    // start from PI
    if (visited.find(faultID) == visited.end() /*&& faninType == PI*/) {
      vector<int> newFaults;
      vector<int> testVector;
      set<int> sameFaults;
      newFaults.push_back(faultID);
      if(generateTestBySAT(newFaults, testVector) == 1) { // SAT
        findSSAFaultsSameTestVector(faultID, testVector, sameFaults, faults, visited);
        faultToTestVector.insert(pair<set<int>, vector<int>>(sameFaults, testVector));
      } else {   // UNSAT
          redundantSSAF.insert(faultID);
          visited.insert(faultID);
      }
    }
  }
  resetAllVisitedFaultyOut();
  resetFaultsInCircuit();

  int totalNum = visited.size();
  int compressedNum = faultToTestVector.size();
  cout << "Total Collapsed Fault: " << collapsedFaultList.size() << endl;
  cout << "Visited faults: " << visited.size() << " Visited faults are compressed to " << faultToTestVector.size() << endl;
  cout << "redundantSSAF number: "<< redundantSSAF.size() << endl;
  for (auto fault : redundantSSAF) {
    cout << fault << " " ;
    vector <int> testVector;
    vector<int> newFaults;
    newFaults.push_back(fault);
    if(generateTestBySAT(newFaults, testVector) == 1) cout << "SAT" << endl;
    else  cout << "UNSAT" << endl;
  }
}




//----------------------------------------------
// just put all faults into SAT-solver.
void redundantFaultTest(set<int> &faultList) {
  set<int> redundant;
  for (auto faultID : faultList) {
    vector <int> testVector;
    vector<int> newFaults;
    newFaults.push_back(faultID);
    if(generateTestBySAT(newFaults, testVector) == 0) {
      redundant.insert(faultID);
    }
  }
  cout << "Real redundantSSAF number: "<< redundant.size() << endl;
  //printFaults(redundant);
}

void propagationTest() {
  for (auto faultID : collapsedFaultList) {
    vector<int> newFaults;
    newFaults.push_back(faultID);
    vector <int> testVector;
    set<int> fault;
    fault.insert(faultID);
    printFaults(fault);
    generateTestBySAT(newFaults, testVector);
    printTestVector(testVector);
    propagateFault(newFaults, testVector);
    cout << endl;
  }
}

void printFaults(set<int> &faults) {
  for (set<int>::iterator iter = faults.begin(); iter != faults.end(); iter++) {
    int faultID = *iter;
    int gateID = faultID >> 3;
    int port = (faultID >> 1) & 3;
    int stuckat = faultID & 1;
    string in1Name = theCircuit[gateID]->in1Name;
    string in2Name = theCircuit[gateID]->in2Name;
    string outName = theCircuit[gateID]->outName;
    cout << "faultID: " << faultID << "   gateID " << gateID << ":  " << in1Name << " " << in2Name << " " << outName << "; ";
    if (port == 1) {
      cout << in1Name << " input1 ";
    } else if (port == 2){
      cout << in2Name << " input2 ";
    } else if (port == 3){
      cout << outName << " output ";
    }
    cout << "stuck at " << stuckat << endl;
  }
}

void printGateType(Type gateType) {
  switch (gateType){
    case constant:
      cout << "constant";
      break;
    case bufInv:
      cout << "bufInv";
      break;
    case aig:
      cout << "aig";
      break;
    case PO:
      cout << "PO";
      break;
    case PI:
      cout << "PI";
      break;
    case OR:
      cout << "OR";
      break;
    case XOR:
      cout << "XOR";
      break;
    case null:
      cout << "null";
      break;
  }
  cout << " ";
}

int printFault(int ID) {
  int faultID = ID;                         cout << "faultID: " << faultID;
  int oriGateID = (faultID >> 3);           cout << ", oriGateID: " << oriGateID;
  int fauGateID = oriGateID + theCircuit.size();  cout << ", fauGateID: " << fauGateID;
  int port = (faultID >> 1) & 3;           cout << ", port: ";
  if (port < 3) {
    cout << "input" << port;
  }
  else {
    cout << "output";
  }
  int stuckat = faultID & 1;                cout << ", stuckat: " << stuckat;
  cout << ", gateType ";
  printGateType(theCircuit[oriGateID]->gateType);
  cout << endl;
}

void printTestVector(vector<int> &testVector) {
  cout << "test Vector: "<< endl;
  for (auto i : testVector) {
    cout << i << " ";
  }
  cout << endl;
}

// -------used to check the result---------
// inject single faults in all places
void testFaultInjectInCNF() {
  vector<int> newFaults;
  for (set<int>::iterator iter = allFaultList.begin(); iter != allFaultList.end(); iter++) {
    int fault = *iter;
    newFaults.clear();
    printFault(fault);
    newFaults.push_back(fault);
    injectFaultsInCNF(newFaults);
    cout << "origianl + faulty circuit" << endl; printCircuit(oriAndFauCir);
    cout << "origianl + faulty cnf" << endl; printCNF(CNFOriAndFauCir);
    cout << endl;
  }
}

void testFaultInjectInCircuit() {
  /*
  vector<int> newFaults;
  for (set<int>::iterator iter = allFaultList.begin(); iter != allFaultList.end(); iter++) {
    int fault = *iter;
    newFaults.clear();
    printFault(fault);
    newFaults.push_back(fault);
    injectFaultsInCircuit(newFaults);
    printCircuit(theCircuit);
    resetFaultsInCircuit();
    printCircuit(theCircuit);
    cout << endl;
  }
  */
}

void printCircuit(vector <gate*> &curCircuit) {
  for (int i = 0; i < curCircuit.size(); i++) {
    cout << i << " ";
    /*
    if (curCircuit[i]->gateType == null) {
      cout << "Big OR gate";
    } else if (curCircuit[i]->gateType == constant) {
      cout << curCircuit[i]->outName << "(" << curCircuit[i]->gateID << ") " <<  curCircuit[i]->outValue;
    } else if (curCircuit[i]->gateType == PI) {
      cout << curCircuit[i]->outName << "(" << curCircuit[i]->gateID << ") ";
    } else {
      cout << curCircuit[i]->fanin1->outName << "(" << curCircuit[i]->fanin1->gateID << ") ";
      if (curCircuit[i]->gateType == aig || curCircuit[i]->gateType == OR || curCircuit[i]->gateType == XOR) {
        cout << curCircuit[i]->fanin2->outName << "(" << curCircuit[i]->fanin2->gateID << ") ";
      }
      cout << curCircuit[i]->outName << "(" << curCircuit[i]->gateID << ") ";
    }
    */
    /*
    if (curCircuit[i]->gateType == null) {
      cout << "Big OR gate";
    } else if (curCircuit[i]->gateType == constant) {
      cout << curCircuit[i]->outName << "(" << curCircuit[i]->gateID << " " << curCircuit[i]->invOut << ") " <<  curCircuit[i]->outValue;
    } else if (curCircuit[i]->gateType == PI) {
      cout << curCircuit[i]->outName << "(" << curCircuit[i]->gateID << ") ";
    } else {
      cout << curCircuit[i]->fanin1->outName << "(" << curCircuit[i]->fanin1->gateID << " " << curCircuit[i]->invIn1 << ") ";
      if (curCircuit[i]->gateType == aig || curCircuit[i]->gateType == OR || curCircuit[i]->gateType == XOR) {
        cout << curCircuit[i]->fanin2->outName << "(" << curCircuit[i]->fanin2->gateID << " " << curCircuit[i]->invIn2 << ") ";
      }
      cout << curCircuit[i]->outName << "(" << curCircuit[i]->gateID << " " << curCircuit[i]->invOut <<  ") ";
    }
    */
    if (curCircuit[i]->gateType == null) {
      cout << "Big OR gate";
    } else if (curCircuit[i]->gateType == constant) {
      cout << curCircuit[i]->outName << "(val" << curCircuit[i]->outValue << " inv" << curCircuit[i]->invOut << ") " <<  curCircuit[i]->outValue;
    } else if (curCircuit[i]->gateType == PI) {
      cout << curCircuit[i]->outName << "(val" << curCircuit[i]->outValue << ") ";
    } else {
      cout << curCircuit[i]->fanin1->outName << "(val" << curCircuit[i]->fanin1->outValue << " inv" << curCircuit[i]->invIn1 << ") ";
      if (curCircuit[i]->gateType == aig || curCircuit[i]->gateType == OR || curCircuit[i]->gateType == XOR) {
        cout << curCircuit[i]->fanin2->outName << "(val" << curCircuit[i]->fanin2->outValue << " inv" << curCircuit[i]->invIn2 << ") ";
      }
      cout << curCircuit[i]->outName << "(val" << curCircuit[i]->outValue << " inv" << curCircuit[i]->invOut <<  ") ";
    }
    // constant, bufInv, aig, PO, PI, OR, XOR
    cout << " "; printGateType(curCircuit[i]->gateType);

    if (curCircuit[i]->faultyOut) {
      cout << " faultyGate  ";
    } else {
      cout << " NormalGate  ";
    }
    if (curCircuit[i]->visited) {
      cout << " visited";
    } else {
      cout << " unvisited";
    }
    cout << " outValue: " << curCircuit[i]->outValue;

    cout << endl;
  }
  cout << endl;
}

void printOutput() {
  for (auto curGate : theCircuit) {
    cout << curGate->gateID << " "<< curGate->outName << " = " << curGate->outValue << endl;
  }
}

void printForTestbench() {
  for (int i = 0; i < PISize; i++) {
    cout << theCircuit[i]->outName << " = " << theCircuit[i]->outValue << ";" << endl;
  }
  for (auto curGate : theCircuit) {
    cout << "$fwrite(w_file,\"\\n" << curGate->gateID << " " << curGate->outName << "= \"," << curGate->outName <<");" << endl;
  }
}

void printCNF(vector<vector<vector<int>>> &CNFFormula) {
  for (int m = 0; m < CNFFormula.size(); m++) {  // gate
    cout << m << ": ";
    for (int i = 0; i < CNFFormula[m].size(); i++) {//clauses
      for (int j = 0; j < CNFFormula[m][i].size(); j++) {//literals
          if (CNFFormula[m][i][j] > 0) {
              cout << CNFFormula[m][i][j] /*- 1*/ << " ";
          } else {
              cout << "-" << (-1)*CNFFormula[m][i][j] /*- 1*/ << " ";
          }
      }
      cout << ", ";
    }
    cout << endl;
  }
}

void printCNFFile(vector<vector<vector<int>>> &CNFFormula, vector<gate*> curCircuit) {
  int cnt = 0;
  for (int m = 0; m < CNFFormula.size(); m++) {  // gate
    cout << m << ": " << endl;
    for (int i = 0; i < CNFFormula[m].size(); i++) {//clauses
      for (int j = 0; j < CNFFormula[m][i].size(); j++) {//literals
          cnt++;
          if (CNFFormula[m][i][j] > 0) {
              cout << CNFFormula[m][i][j] /*- 1*/ << " ";
          } else {
              cout << "-" << (-1)*CNFFormula[m][i][j] /*- 1*/ << " ";
          }
      }
      cout << " 0" << endl;;
    }
  }
  cout << "p cnf " << curCircuit.size() << " " << cnt << endl;
}

void printCircuitCNFBlif(vector <gate*> &curCircuit) {
  ofstream myfile;
  myfile.open ("test.blif");



  myfile << ".model test" << endl;
  myfile << ".inputs ";
  for (auto curGate : curCircuit) {
    if (curGate->gateType == PI) {
      myfile << curGate->outName << " ";
    }
  }
  myfile << endl;

  myfile << ".outputs finalOutput" << endl;
  for (auto curGate : curCircuit) {
    if (curGate->gateType == null)  continue;
    if (curGate->in1Name.compare(curGate->outName) == 0)  continue;
    myfile << ".names ";
    if (curGate->gateType == constant) {
      myfile << curGate->outName << endl;
      myfile << curGate->outValue << endl;
    } else {
      if (curGate->gateType == PI) {
        myfile << curGate->in1Name << " ";
      } else {
        myfile << curGate->fanin1->outName << " ";
        if (curGate->gateType == aig || curGate->gateType == OR || curGate->gateType == XOR) {
          myfile << curGate->fanin2->outName << " ";
        }
      }
      myfile << curGate->outName << endl;

      if (curGate->gateType != XOR) {
        myfile << curGate->invIn1;
        if (curGate->gateType == aig || curGate->gateType == OR || curGate->gateType == XOR) {
          myfile << curGate->invIn2;
        }
        myfile << " " << curGate->invOut << endl;
      } else {
        myfile << "10 1\n01 1\n";
      }
    }
  }

  myfile << ".names ";
  for (auto curGate : curCircuit) {
    if (curGate->gateType == PO) {
      myfile << curGate->outName << " ";
    }
  }
  myfile << "finalOutput" << endl;
  for (auto curGate : curCircuit) {
    if (curGate->gateType == PO) {
      myfile << "0";
    }
  }
  myfile << " 0" << endl;
  myfile << endl;
  myfile << ".end" << endl;


  myfile.close();
}
// --------------------------------------


// CheckGivenPatterns: use to check whether the remaining fautls are redundant faults or not.
// ******to save the time, we dont check it know******
// check the remaining faults undetected by the give test pattern..
// actually they must by redundant
for (auto faultID : faultList) {
  if (visited.find(faultID) == visited.end()) {
    vector<int> newFaults;
    newFaults.push_back(faultID);
    set<int> faultsSameVector;
    faultsSameVector.insert(faultID);
    vector<int> testVector;
    if (generateTestBySAT(newFaults, testVector)) {
      faultToPatterns.insert(pair<set<int>, vector<int>>(faultsSameVector, testVector));
      cout << "Test patterns cannot detect the fault and we generate a new one: " << faultID << endl;
    } else {
      redundantSSAF.insert(faultID);
      visited.insert(faultID);
    }
  }
}
if (visited.size() == faultList.size()) {
  cout << "Test patterns are sufficient" << endl;
  cout << faultToPatterns.size() << "/"  << SSAFPatterns.size() << " test patterns are used."<< endl;
  cout << "redundantSSAF.size(): " << redundantSSAF.size() << endl;
} else {
  cout << "Test patterns are insufficient" << endl;
  cout << "visited.size(): " << visited.size() << endl;
  cout << "redundantSSAF.size(): " << redundantSSAF.size() << endl;
  cout << "collapsedFaultList.size(): " << collapsedFaultList.size() << endl;
}
