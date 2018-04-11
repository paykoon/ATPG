
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


// follow the faultyOut mark
// recursive function to find the faults that can be detected by same vector
void findFaultsSameTest_helper(gate *curGate, set<int> &sameFaults) {
  if (curGate->gateType == PO) {
    return;
  }
  // check the current gate. only for current fault model
  if (curGate->gateType == aig) {
    if (curGate->fanin1->gateType == PI || curGate->fanin1->fanout.size() > 1) {
      int gateID = curGate->gateID;
      int port = 1;
      int activate = curGate->fanin1->outValue;
      int stuckat = 1 - activate;
        // check the side value
      if (curGate->fanin2->outValue == curGate->invIn2) {
        sameFaults.insert(getFaultID(gateID, port, stuckat));
      }
    }
    if (curGate->fanin2->gateType == PI || curGate->fanin2->fanout.size() > 1) {
      int gateID = curGate->gateID;
      int port = 2;
      int activate = curGate->fanin2->outValue;
      int stuckat = 1 - activate;
      if (curGate->fanin1->outValue == curGate->invIn1) {
        sameFaults.insert(getFaultID(gateID, port, stuckat));
      }
    }
  }
  // go to the next level
  for (auto fanout : curGate->fanout) {
    // due to the side value, some gates may not propagate fault.
    // only go the gate that propagate the fault
    if (fanout->faultyOut == true) {
      findFaultsSameTest_helper(fanout, sameFaults);
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
