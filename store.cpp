
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



// ----------------------previous propagation fault----------------------------
// try to find all path of all faults
void findSSAFPath(vector<int> &newFaults) {
  for (auto faultID : newFaults) {
    int gateID = (faultID >> 3);
    if (theCircuit[gateID]->different == true) {
      findSSAFPathDFS(theCircuit[gateID]);
    }
  }
}
// function: find the SSA faults' path; also try to find the faults that may block it.
// base case : reach PO. return true(has check visited in previous level)
// recursion rule: select the visited fanout and go into next level.
// if none of fanout return true, we will return false.
bool findSSAFPathDFS(gate *curGate) {
  // base case
  if (curGate->gateType == PO) {
      curGate->faultyOut = true;
      return true;
  }
  bool result = false;
  for (auto fanout : curGate->fanout) {
    // if one of its fanout can propagate the value, its faultyOut == true.
    if (fanout->different == true && findSSAFPathDFS(fanout) == true) {
      curGate->faultyOut = true;
      result = true;
    }
  }
  return result;
}

// given the faultID and test vector, mark the gate in the propgation path as "faultyOut = true"
// first propagate PI in faulty circuit, then do the same thing in original circuit.
// so the outValue remains in the cirucit is the value to activate and propagte faults
// return 1 if faults can be tested by the test pattern, else return 0;
int propagateFault(vector<int> &newFaults, vector<int> &testVector) {
  resetAllVisitedFaultyOut();
  resetFaultsInCircuit();
  // inject faults and propagate the value
  injectFaultsInCircuit(newFaults);
  assignPIs(testVector);
  propagatePI();

  resetAllVisitedFaultyOut();
  resetFaultsInCircuit();
  // progate the value in original circuit
  assignPIs(testVector);
  propagatePI();
  /*
  findSSAFPath(newFaults);
  for (int i = 0; i < POSize; i++) {
    gate *POGate = theCircuit[PISize + gateSize + i];
    // if the fautls can be detected in one of the PO, then it's detected.
    if (POGate->faultyOut == true) {
      return 1;
    }
  }
  */
  for (int i = 0; i < POSize; i++) {
    gate *POGate = theCircuit[PISize + gateSize + i];
    // if the fautls can be detected in one of the PO, then it's detected.
    if (POGate->different == true ) {
      return 1;
    }
  }
  return 0;
}


//*******************************************
if (oriFault == 331 && curGate->gateID == (348 >> 3)) {
  cout << "***********blockFaultID " << blockFaultID << endl;
  set<int> connectedGates;
  findConnectedGatesDFS(theCircuit[348>>3], connectedGates);
  vector<gate*> connectedGatesVec;
  for (auto id : connectedGates) {
    connectedGatesVec.push_back(theCircuit[id]);
  }
  printFault2(331);
  printFault2(348);
  printCircuit(connectedGatesVec);
}
//*******************************************




void sameFaultCurToPIDFS(gate *curGate, set<int> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAF) {
  // base case
  if (curGate->gateType == aig) {
    int outStuckat = 1 - curGate->outValue;
    if (outStuckat == curGate->invOut) return;
  } else if (curGate->gateType == PI || curGate->gateType == constant) {
    return;
  }
  // check current inputs
  if (curGate->gateType == aig) {
    int stuckat1 = (1 - curGate->invIn1);
    // getFaultID(int gateID, int port, int stuckat)
    int blockFaultID1 = getFaultID(curGate->gateID, 1, stuckat1);
    if (faultList.find(blockFaultID1) != faultList.end() || redundantSSAF.find(blockFaultID1) != redundantSSAF.end()) {
      blockFaultsList.insert((redundantSSAF.find(blockFaultID1) != redundantSSAF.end()) ? blockFaultID1 * (-1) : blockFaultID1);
    }
    int stuckat2 = (1 - curGate->invIn2);
    int blockFaultID2 = getFaultID(curGate->gateID, 2, stuckat2);
    if (faultList.find(blockFaultID2) != faultList.end() || redundantSSAF.find(blockFaultID2) != redundantSSAF.end()) {
      blockFaultsList.insert((redundantSSAF.find(blockFaultID2) != redundantSSAF.end()) ? blockFaultID2 * (-1) : blockFaultID2);
    }
  } else if (curGate->gateType == bufInv) {
    int outStuckat = 1 - curGate->outValue;
    int stuckat = (outStuckat == curGate->invOut) == curGate->invIn1;
    int blockFaultID = getFaultID(curGate->gateID, 1, stuckat);
    if (faultList.find(blockFaultID) != faultList.end() || redundantSSAF.find(blockFaultID) != redundantSSAF.end()) {
      blockFaultsList.insert((redundantSSAF.find(blockFaultID) != redundantSSAF.end()) ? blockFaultID * (-1) : blockFaultID);
    }
  }
  // go to the fanin gate
  sameFaultCurToPIDFS(curGate->fanin1, blockFaultsList, faultList, redundantSSAF);
  if (curGate->gateType == aig) sameFaultCurToPIDFS(curGate->fanin2, blockFaultsList, faultList, redundantSSAF);
}

// check whether the given test patterns can cover entire faultList or not.
// also find the potentiallyUndetected.
// assume that the initial test pattern can already detect all SSAF.
// the faults that has no matching test patterns are redundant.
// get: vector<set<int>, vector<int>> faultToPatterns;   set<int> redundantSSAF;
void CheckGivenPatterns(set<int> &faultList, set<int> &redundantSSAF, vector<vector<int>> &SSAFPatterns) {
  set<int> visited;
  for (auto testVector : SSAFPatterns) {
    if (visited.size() == faultList.size()) break; // all faults are checked
    for (auto faultID : faultList) {
      if (visited.find(faultID) != visited.end()) continue;  // already checked
      vector<int> newFaults;
      newFaults.push_back(faultID);
      set<int> blockFaultsList;
      if (checkFaultAndTestVector(newFaults, testVector, blockFaultsList, faultList, redundantSSAF, 1) == 1) {
        // faultsSameVector.insert(faultID);
        faultToPatterns.insert(make_pair(faultID, testVector));
        visited.insert(faultID);
        if (blockFaultsList.size() > 0) {
          potentiallyUndetected.insert(make_pair(faultID, blockFaultsList));
        }
      }
    }
  }
  // the redundant fault among our fault model
  for (auto faultID : faultList) {
    if (visited.find(faultID) == visited.end()) {
      redundantSSAF.insert(faultID);
    }
  }
}





//------------------Find the propagation path of the faults under the given test pattern and the potential undetected faults -------------------
// function: recursively find the same fault in the path of current fault's gate to PI.
// run in the non-faulty circuit with correct values
// for AIG, only when outValue != invOut, can find the same fault.
void sameFaultCurToPIDFS(gate *curGate, set<int> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAF, int oriFault) {
  // base case
  if (curGate->gateType == PI || curGate->gateType == constant) {
    return;
  }
  int outInverse = 1 - curGate->outValue;
  // if the change of one of gates' value will results in outStuckat value, then that value is the stuckat fault we want to find
  if (curGate->gateType == aig) {
    int in1Value = curGate->fanin1->outValue;
    int in1Inverse = 1 - in1Value;
    int in2Value = curGate->fanin2->outValue;
    int in2Inverse = 1 - in2Value;
    // *******check why 868 is overlooked...******
    /*
    if (oriFault == 2090)  {
      cout << "207value********here****22222*** " << curGate->gateID << endl;
      //cout << curGate->fanin1->outName << " in1Diff " << curGate->fanin1->different << endl;
      //cout << curGate->fanin2->outName << " in2Diff " << curGate->fanin2->different << endl;
      //cout << curGate->outName << " outDiff " << curGate->different << endl;
    }*/
    // ***************
    // case1. if the inversing value will also make the output inversing, that inversing value is the stuck at fault we want to find
    // TODO only the value that changes the output will be choosed.
    if (curGate->getOutValue(in1Inverse, in2Value) == outInverse) {
      int blockFaultID = getFaultID(curGate->gateID, 1, in1Inverse);
      if (faultList.find(blockFaultID) != faultList.end() || redundantSSAF.find(blockFaultID) != redundantSSAF.end()) {
        blockFaultsList.insert(blockFaultID);
      }
      // the current position may not be in our model, but the front position(near to PO) may be, we need to keeping track  to front place...
      sameFaultCurToPIDFS(curGate->fanin1, blockFaultsList, faultList, redundantSSAF, oriFault);
    }
    if (curGate->getOutValue(in1Value, in2Inverse) == outInverse) {
      int blockFaultID = getFaultID(curGate->gateID, 2, in2Inverse);
      if (faultList.find(blockFaultID) != faultList.end() || redundantSSAF.find(blockFaultID) != redundantSSAF.end()) {
        blockFaultsList.insert(blockFaultID);
      }
      sameFaultCurToPIDFS(curGate->fanin2, blockFaultsList, faultList, redundantSSAF, oriFault);
    }
    // case2. the D or ~D is required to be blocked in this gate(they mutually counteract and disappear in output),
    // otherwire it may block the fault propagation in other path.
    // if there is a stuckat fault make D or ~D propagate again, two propagataion path may conoverge somewhere then block.
    // TODO. in this case, also we need to check the previous value. need to write a new function.
    if (curGate->fanin1->different == 1 && curGate->fanin2->different == 1) {
      if ((curGate->fanin1->outValue == curGate->invIn1) != (curGate->fanin2->outValue == curGate->invIn2)) {
        // this fault allow D(~D) in another input to be propagated
        int blockFaultID = getFaultID(curGate->gateID, 1, curGate->invIn1);
        if (faultList.find(blockFaultID) != faultList.end() || redundantSSAF.find(blockFaultID) != redundantSSAF.end()) {
          blockFaultsList.insert(blockFaultID);
        }
        blockFaultID = getFaultID(curGate->gateID, 2, curGate->invIn2);
        if (faultList.find(blockFaultID) != faultList.end() || redundantSSAF.find(blockFaultID) != redundantSSAF.end()) {
          blockFaultsList.insert(blockFaultID);
        }
      }
    }
    // if it's inv, just put it into potential list
  } else if (curGate->gateType == bufInv) {
    int outStuckat = 1 - curGate->outValue;
    int stuckat = (outStuckat == curGate->invOut) == curGate->invIn1;
    int blockFaultID = getFaultID(curGate->gateID, 1, stuckat);
    if (faultList.find(blockFaultID) != faultList.end() || redundantSSAF.find(blockFaultID) != redundantSSAF.end()) {
      blockFaultsList.insert(blockFaultID);
    }
    sameFaultCurToPIDFS(curGate->fanin1, blockFaultsList, faultList, redundantSSAF, oriFault);
  }
}

// function: recursively find the SSA faults' path; also try to find the faults that may block it.
// run in the non-faulty circuit with correct values
// base case : reach PO. return true(has check visited in previous level)
// recursion rule: select the visited fanout and go into next level.
//                 check it's side value to find the block faults.
// if none of fanout return true, we will return false.
bool findBlockSSADFS(gate *curGate, gate *preGate, set<int> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAF, int oriFault) {
  // base case
  if (curGate->gateType == PO) {
      curGate->isPath = true;
      return true;
  }
  bool isPath = false;
  // check all the propagation paths to find the possible block fault.
  for (auto fanout : curGate->fanout) {
    // if one of its fanout can propagate the value, this gate is in the propagation path
    if (fanout->different == true && findBlockSSADFS(fanout, curGate, blockFaultsList, faultList, redundantSSAF, oriFault) == true) {
      curGate->isPath = true;
      isPath = true;
      // try to find the SSAF that may block the fault
      if (curGate->gateType == aig) {
        // check side value.
        // TODO only the value that changes the output will be choosed.
        int sidePort = (curGate->fanin1 == preGate) ?  2 : 1;
        int sideStuckat = (curGate->fanin1 == preGate) ? (1 - curGate->invIn2) : (1 - curGate->invIn1);
        int blockFaultID = getFaultID(curGate->gateID, sidePort, sideStuckat);
        if (faultList.find(blockFaultID) != faultList.end() || redundantSSAF.find(blockFaultID) != redundantSSAF.end()) {
          blockFaultsList.insert(blockFaultID);
        }
        //*********
        /*
        if (oriFault == 334 && curGate->gateID == 41)  {
          cout << "334value********here****111111*** " << curGate->gateID << endl;
          //cout << "preGate " << preGate->outName << endl;
          //cout << "curGate->fanin1 " << curGate->fanin1->outName << " curGate->fanin2 " << curGate->fanin2->outName << endl;
          //cout << "next gate " << (curGate->fanin1 == preGate ? curGate->fanin2->outName : curGate->fanin1->outName) << endl;
        }*/
        //*********
        //*******************************************
        /*
        if (oriFault == 3282 && curGate->gateID == (3278 >> 3)) {
          //cout << "***********blockFaultID " << sideBlockFaultID << endl;
          set<int> connectedGates;
          findConnectedGatesDFS(theCircuit[3278 >> 3], connectedGates);
          vector<gate*> connectedGatesVec;
          for (auto id : connectedGates) {
            connectedGatesVec.push_back(theCircuit[id]);
          }
          printCircuit(connectedGatesVec);

          // printCircuitBlif(connectedGatesVec);

          // printForTestbench(connectedGatesVec, 14);
        }
        */
        //*******************************************
        // search from side value to PI or constant.
        sameFaultCurToPIDFS((curGate->fanin1 == preGate ? curGate->fanin2 : curGate->fanin1), blockFaultsList, faultList, redundantSSAF, oriFault);
      }
      // check the fault in the same path. If the fault in same path is the redundant fault, it will block cur fault.
      // check gate input
      int samePathPort = (curGate->fanin1 == preGate) ?  1 : 2;
      int samePathBlockFaultID0 = getFaultID(curGate->gateID, samePathPort, 0);
      int samePathBlockFaultID1 = getFaultID(curGate->gateID, samePathPort, 1);
      if (redundantSSAF.find(samePathBlockFaultID0) != redundantSSAF.end()) {
        blockFaultsList.insert(samePathBlockFaultID0);
      }
      if (redundantSSAF.find(samePathBlockFaultID1) != redundantSSAF.end()) {
        blockFaultsList.insert(samePathBlockFaultID1);
      }
      // check gate output
      samePathPort = 3;
      samePathBlockFaultID0 = getFaultID(curGate->gateID, samePathPort, 0);
      samePathBlockFaultID1 = getFaultID(curGate->gateID, samePathPort, 1);
      if (redundantSSAF.find(samePathBlockFaultID0) != redundantSSAF.end()) {
        blockFaultsList.insert(samePathBlockFaultID0);
      }
      if (redundantSSAF.find(samePathBlockFaultID1) != redundantSSAF.end()) {
        blockFaultsList.insert(samePathBlockFaultID1);
      }
    }
  }
  return isPath;
}




bool findBlockSSADFS(gate *curGate, gate *preGate, set<int> &blockFaultsList, unordered_set<int> &faultList, unordered_set<int> &redundantSSAFList, int oriFault) {
  // base case
  if (curGate->gateType == PO) {
      curGate->isPath = true;
      return true;
  }
  bool isPath = false;
  // check all the propagation paths to find the possible block fault.
  for (auto fanout : curGate->fanout) {
    // if one of its fanout can propagate the value, this gate is in the propagation path
    if (fanout->different == true && findBlockSSADFS(fanout, curGate, blockFaultsList, faultList, redundantSSAFList, oriFault) == true) {
      curGate->isPath = true;
      isPath = true;
      // try to find the SSAF that may block the fault
      if (curGate->gateType == aig) {
        // check side value.
        // TODO only the value that changes the output will be choosed.
        int curInv = (curGate->fanin1 == preGate) ? curGate->invIn1 : curGate->invIn2;
        int curStuckat = (curGate->fanin1 == preGate) ? (1 - curGate->fanin1->outValue) : (1 - curGate->fanin2->outValue);
        int sideInv = (curGate->fanin1 == preGate) ? curGate->invIn2 : curGate->invIn1;
        int sideStuckat = (curGate->fanin1 == preGate) ? (1 - curGate->fanin2->outValue) : (1 - curGate->fanin1->outValue);
        int sidePort = (curGate->fanin1 == preGate) ? 2 : 1;
        if ( (curInv == curStuckat) && (sideInv != sideStuckat) ) {
          int blockFaultID = getFaultID(curGate->gateID, sidePort, sideStuckat);
          if (faultList.find(blockFaultID) != faultList.end() || redundantSSAFList.find(blockFaultID) != redundantSSAFList.end()) {
            blockFaultsList.insert(blockFaultID);
          }
          sameFaultCurToPIDFS((curGate->fanin1 == preGate ? curGate->fanin2 : curGate->fanin1), blockFaultsList, faultList, redundantSSAFList, oriFault);
        }
        //*********
        /*
        if (oriFault == 334 && curGate->gateID == 41)  {
          cout << "334value********here****111111*** " << curGate->gateID << endl;
          //cout << "preGate " << preGate->outName << endl;
          //cout << "curGate->fanin1 " << curGate->fanin1->outName << " curGate->fanin2 " << curGate->fanin2->outName << endl;
          //cout << "next gate " << (curGate->fanin1 == preGate ? curGate->fanin2->outName : curGate->fanin1->outName) << endl;
        }*/
        //*********
        //*******************************************
        /*
        if (oriFault == 3282 && curGate->gateID == (3278 >> 3)) {
          //cout << "***********blockFaultID " << sideBlockFaultID << endl;
          set<int> connectedGates;
          findConnectedGatesDFS(theCircuit[3278 >> 3], connectedGates);
          vector<gate*> connectedGatesVec;
          for (auto id : connectedGates) {
            connectedGatesVec.push_back(theCircuit[id]);
          }
          printCircuit(connectedGatesVec);

          // printCircuitBlif(connectedGatesVec);

          // printForTestbench(connectedGatesVec, 14);
        }
        */
        //*******************************************
        // search from side value to PI or constant.
      }
      // check the fault in the same path. If the fault in same path is the redundant fault, it will block cur fault.
      // check gate input
      int samePathPort = (curGate->fanin1 == preGate) ?  1 : 2;
      int samePathBlockFaultID0 = getFaultID(curGate->gateID, samePathPort, 0);
      int samePathBlockFaultID1 = getFaultID(curGate->gateID, samePathPort, 1);
      if (redundantSSAFList.find(samePathBlockFaultID0) != redundantSSAFList.end()) {
        blockFaultsList.insert(samePathBlockFaultID0);
      }
      if (redundantSSAFList.find(samePathBlockFaultID1) != redundantSSAFList.end()) {
        blockFaultsList.insert(samePathBlockFaultID1);
      }
      // check gate output
      samePathPort = 3;
      samePathBlockFaultID0 = getFaultID(curGate->gateID, samePathPort, 0);
      samePathBlockFaultID1 = getFaultID(curGate->gateID, samePathPort, 1);
      if (redundantSSAFList.find(samePathBlockFaultID0) != redundantSSAFList.end()) {
        blockFaultsList.insert(samePathBlockFaultID0);
      }
      if (redundantSSAFList.find(samePathBlockFaultID1) != redundantSSAFList.end()) {
        blockFaultsList.insert(samePathBlockFaultID1);
      }
    }
  }
  return isPath;
}




void propagatePI(){
  for(int i = 0; i < theCircuit.size(); i++){
    int preValue = theCircuit[i]->outValue;
    theCircuit[i]->setOut();
    int curValue = theCircuit[i]->outValue;
    theCircuit[i]->different = (preValue != curValue);
  }
}





// -----------generate multiple faults, and check the coverage of them----------------
// SSAFList: initial SSA fault list. allNFaults: the multiple faults generate according to SSAFList.
// N multile faults number. mode = 0, only generate the N simultaneous faults. mode = 1, generate all faults <= N.
// generateAllNSA(collapsedSSAFList, allDoubleFaults_collapsed, 2, 1);
void generateAllNSA(set<int> &SSAFList, vector<vector<int>> &allNFaults, int N, int mode) {
  if (N > SSAFList.size()) return;
  vector<int> list;
  vector<int> curFaults;
  for (auto elem : SSAFList) {
    list.push_back(elem);
  }
  generateAllNSA_helper(list, curFaults,allNFaults, 0, N, mode);
}
void generateAllNSA_helper(vector<int> &list, vector<int> &curFaults, vector<vector<int>> &allNFaults, int index, int N, int mode) {
  if (mode == 0 && curFaults.size() == N) {
    vector<int> temp(curFaults);
    allNFaults.push_back(temp);
    return;
  } else if (mode == 1 && index == N) {
    return;
  }
  for (int i = index; i < list.size(); i++) {
    curFaults.push_back(list[i]);
    if (mode == 1) {
      vector<int> temp(curFaults);
      allNFaults.push_back(temp);
    }
    generateAllNSA_helper(list, curFaults,allNFaults, i + 1, N, mode);
    curFaults.pop_back();
  }
}





void printFault(int ID) {
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

// ----------------
void printPotentiallyDSA(map<int, set<int>> &potentiallyUndetected) {
  cout << "\npotentiallyUndetected:" << endl;
  for (map<int, set<int>>::iterator iter = potentiallyUndetected.begin(); iter != potentiallyUndetected.end(); iter++) {
    int faultID = iter->first;
    set<int> blockFaultSet = iter->second;
    cout << faultID << ": ";
    for (auto blockFault : blockFaultSet) {
      cout << blockFault << " ";
    }
    cout << endl << endl;
  }
}

void printPotentiallyDSA2(map<int, set<int>> &potentiallyUndetected, map<int, vector<int>> &SSAFToPatterns) {
  cout << "\npotentiallyUndetected:" << endl;
  for (map<int, set<int>>::iterator iter = potentiallyUndetected.begin(); iter != potentiallyUndetected.end(); iter++) {
    int faultID = iter->first;
    set<int> blockFaultSet = iter->second;
    printFault2(faultID);
    printTestVector(SSAFToPatterns[faultID]);
    for (auto blockFault : blockFaultSet) {
      printFault2(blockFault);
    }
    cout << endl << endl;
  }
}

void printUndetectedDSA(set<set<int>> &undetectedDSA, testgenebysat *testBySAT) {
  cout << "\n\n\nUndetected DSA:" << endl;
  for (auto DSA : undetectedDSA) {
    for (auto faultID : DSA) {
      cout << faultID << " ";
      printFault2(faultID);
      vector<int> curFault;
      curFault.push_back(faultID);
      vector<int> pattern;
      if (testBySAT->generateTestBySAT_1(curFault, pattern) == 0) {
        cout << "******Redundant SSAF******" << endl;
      }
    }
    cout << "\n\n";
  }
}

void printTestVector(vector<int> &testVector) {
  cout << "test pattern: ";
  for (auto i : testVector) {
    cout << i;
  }
  cout << endl;
}

void printFault2(int ID) {
  int faultID = ID;                         cout << "faultID: " << faultID;
  int GateID = (faultID >> 3);           cout << ", GateID: " << GateID;
  int port = (faultID >> 1) & 3;           cout << ", port: ";
  if (port < 3) {
    if (port == 1) {
      cout << theCircuit[GateID]->in1Name;
    } else {
      cout << theCircuit[GateID]->in2Name;
    }
  }
  else {
    cout << "output";
  }
  int stuckat = faultID & 1;                cout << ", stuckat: " << stuckat;
  cout << ", gateType ";
  printGateType(theCircuit[GateID]->gateType);
  cout << endl;
}

void printCircuit(vector <gate*> &curCircuit) {
  for (int i = 0; i < curCircuit.size(); i++) {
    cout << curCircuit[i]->gateID << " ";
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
    cout << " diff" << curCircuit[i]->different;
    cout << endl;
  }
  cout << endl;
}

void printCircuit_64(vector <gate*> &curCircuit) {
  for (int i = 0; i < curCircuit.size(); i++) {
    cout << curCircuit[i]->gateID << " ";
    if (curCircuit[i]->gateType == null) {
      cout << "Big OR gate";
    } else if (curCircuit[i]->gateType == constant) {
      cout << curCircuit[i]->outName << "(val" << curCircuit[i]->outValue_64 << " inv" << curCircuit[i]->invOut << ") " <<  curCircuit[i]->outValue;
    } else if (curCircuit[i]->gateType == PI) {
      cout << curCircuit[i]->outName << "(val" << curCircuit[i]->outValue_64 << ") ";
    } else {
      cout << curCircuit[i]->fanin1->outName << "(val" << curCircuit[i]->fanin1->outValue_64 << " inv" << curCircuit[i]->invIn1 << ") ";
      if (curCircuit[i]->gateType == aig || curCircuit[i]->gateType == OR || curCircuit[i]->gateType == XOR) {
        cout << curCircuit[i]->fanin2->outName << "(val" << curCircuit[i]->fanin2->outValue_64 << " inv" << curCircuit[i]->invIn2 << ") ";
      }
      cout << curCircuit[i]->outName << "(val" << curCircuit[i]->outValue_64 << " inv" << curCircuit[i]->invOut <<  ") ";
    }
    // constant, bufInv, aig, PO, PI, OR, XOR
    cout << " "; printGateType(curCircuit[i]->gateType);
    cout << endl;
  }
  cout << endl;
}

void printNfaults(vector<vector<int>>multipleFaults) {
  for (auto faultSet : multipleFaults) {
    cout << "=======================" << endl;
    for (auto fault : faultSet) {
      printFault2(fault);
    }
    cout << "=======================" << endl;
  }
}

void printCircuitBlif(vector <gate*> &curCircuit) {
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

void printForTestbench(vector <gate*> &theCircuit, int inputSize) {
  cout << "integer w_file;\ninitial\nbegin" << endl;
  cout << "w_file = $fopen(\"data_out.txt\");" << endl;
  for (int i = 0; i < inputSize; i++) {
    cout << theCircuit[i]->outName << " = " << theCircuit[i]->outValue << ";" << endl;
  }
  cout << "#80;" << endl;
  for (auto curGate : theCircuit) {
    cout << "$fwrite(w_file,\"\\n" << curGate->gateID << " " << curGate->outName << "= \"," << curGate->outName <<");" << endl;
  }
  cout << "$fclose(w_file);" << endl;
  cout << "end" << endl;
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
