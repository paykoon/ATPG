#ifndef _MSATest_H
#define _MSATest_H

#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <queue>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include "time.h"
#include "Gate.h"
#include "glucose.h"
#include "Circuit.h"
#include "Gate.h"
#include "CNFGeneration.h"
#include "ATPG.h"

using namespace std;
using namespace Gate;
using namespace Circuit;
using namespace Glucose;
using namespace CNFGeneration;
using namespace ATPG;

namespace MSATEST {
  class msatest {
    /*
    public:
      atpg *ATPGObject;
      vector <gate*> theCircuit;
      int PISize, POSize, gateSize;
      set<int> collapsedFaultList;
      set<int> allFaultList;
      set<int> notIncollapsedFaultList;
      set<int> redundantSSAF;
      // key: faults, value: corresponding test vector
      vector<pair<set<int>, vector<int>>> faultToPatterns;
      // ----------undetected DSA----------
      map<int, set<int>> potentiallyUndetected;
      vector<pair<int, int>> undetectedDSA;

      msatest(atpg *ATPGInit) {
        this->theCircuit = ATPGInit->theCircuit;
        this->PISize = ATPGInit->PISize;
        this->POSize = ATPGInit->POSize;
        this->gateSize = ATPGInit->gateSize;
        this->collapsedFaultList = ATPGInit->collapsedFaultList;
        this->allFaultList = ATPGInit->allFaultList;
        this->notIncollapsedFaultList = ATPGInit->notIncollapsedFaultList;
        this->redundantSSAF = ATPGInit->redundantSSAF;
        this->faultToPatterns = ATPGInit->faultToPatterns;
        this->ATPGObject = ATPGInit;
      }
      ~msatest() {
        delete this;
      }

      // Recursive function. find all the SSA faults that block current fault's propagation
      // for each gate: check it's side value.
      //                only go to the faulty gates
      // base case: output gate, return.
      void findBlockSSADFS(gate *curGate, int preGateID, set<int> blockFaultsList) {
        if (curGate->gateType == PO) return;
        else if (curGate->gateType == aig || curGate->gateType == OR || curGate->gateType == XOR) {

        }
        for (auto nextGate : curGate->fanout) {
          if (nextGate->faultGate == )
        }
        findBlockSSA(gate *curGate, int preGateID, set<int> blockFaultsList)
      }

      // also check the redundant fault.
      int findPotentiallyUndetectedDSA() {
        set<int> faultSet;
        vector<int> testVector;
        vector<int> newFaults;
        for (vector<pair<set<int>, vector<int>>>::iterator iter = faultToPatterns.begin(); iter != faultToPatterns.end(); iter++) {
          faultSet = iter->first;
          testVector = iter->second;
          for (auto faultID : faultSet) {
            newFaults.push_back(faultID);
            ATPGObject->propagateFault(newFaults, testVector);
            // find the fault locate in its propagation path.
            gate *faultGate = theCircuit[faultID >> 3];

          }
        }
      }
      */
  };
}

#endif
