#ifndef _CIRCUITSIMULATION_H
#define _CIRCUITSIMULATION_H
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <queue>
#include <string>
#include "Gate.h"
#include "glucose.h"
#include "Circuit.h"
#include "Gate.h"
#include "CNFGeneration.h"
using namespace std;
using namespace Gate;
using namespace Circuit;
using namespace Glucose;


namespace Simulation {
  class simulation {
  public:
    vector <gate*> theCircuit;
    int PISize, POSize, gateSize;
    map<int, vector<uint64_t>> gateToRelatedGates;
    static const int WSIZE = 64;
    static const uint64_t one_64 = 0xffffffffffffffff;

    // the circuit will be changed if faults are added to circuit.
    // the original unchanged gate will be stored here.
    map<int, gate*>preGateInTheCircuit;
    int faultsInsertedtheCircuit;

    simulation(circuit *pCircuit) {
      this->theCircuit = pCircuit->theCircuit;
      this->PISize = pCircuit->PISize;
      this->gateSize = pCircuit->gateSize;
      this->POSize = pCircuit->POSize;
      this->gateToRelatedGates = pCircuit->gateToRelatedGates;
      this->faultsInsertedtheCircuit = 0;
    }
    ~simulation() {}

    int isSet(int pos, vector<uint64_t> &vec) {
      int w = pos / WSIZE;
      int b = pos % WSIZE;
      uint64_t val = (vec[w] >> b);
      return (val & 1ULL) > 0 ? 1 : 0;
    }

    int faultIDToGateID(int faultID) {
      return (faultID >> 3);
    }

    vector<uint64_t> getAllRelatedGates(vector<int> &faults) {
      vector<uint64_t> relatedGates((theCircuit.size() - 1) / WSIZE + 1, 0);
      for (auto faultID : faults) {
        int gateID = faultIDToGateID(faultID);
        for (int i = 0; i < relatedGates.size(); i++) {
          relatedGates[i] |= gateToRelatedGates[gateID][i];
        }
      }
      return relatedGates;
    }

    // mode == 0, reset the entire circuit
    // mode == 1, reset the given gates
    void resetAllVisitedisPath(int mode, vector<uint64_t> &relatedGates) {
      if (mode == 0) {
        for (int i = 0; i < theCircuit.size(); i++) {
          theCircuit[i]->different = false;
          theCircuit[i]->isPath = false;
          theCircuit[i]->visited = false;
        }
      } else {
        int w = 0;
        int size = relatedGates.size();
        while (w < size) {
          while (w < size && relatedGates[w] == 0) w++;
          if (w == size) break;
          uint64_t val = relatedGates[w];
          int gateID = w * WSIZE;
          for (int i = 0; i < WSIZE; i++) {
            if ((val & 1) == 1) {
              theCircuit[gateID]->different = false;
              theCircuit[gateID]->isPath = false;
              theCircuit[i]->visited = false;
            }
            val >>= 1;
            gateID++;
          }
          w++;
        }
        for (int i = PI + gateSize; i < theCircuit.size(); i++) {
          theCircuit[i]->different = false;
          theCircuit[i]->isPath = false;
          theCircuit[i]->visited = false;
        }
      }
    }

    int assignPIs(vector<int> &inValues) {
      if (inValues.size() != PISize) {
        cout << "\n***Input vector does not match the size of PI***\n" << endl;
        return 0;
      }
      for (int i = 0;i < inValues.size(); i++){
          theCircuit[i]->setPI(inValues[i]);
      }
      return 1;
    }

    //----------------inject and reset faults into original circuit--------------------
    // inject faults in theCircuit
    int injectFaultsInCircuit(vector<int> &newFaults) {
      if (theCircuit.size() == 0) return 0;
      gate *faultGate;
      for (int i = 0; i < newFaults.size(); i++) {
        int faultID = newFaults[i];
        int gateID = (faultID >> 3);
        int port = (faultID >> 1) & 3;
        int stuckat = faultID & 1;
        faultGate = theCircuit[gateID];
        if ((faultGate->gateType != aig && faultGate->gateType != OR && faultGate->gateType != XOR) || port == 0){
          if (port == 2 || port == 0) {
            cout << "Skip this fault since it has problem." << endl;
            printFault(faultID);
            continue;
          }
        }
        gate *copy = new gate();
        faultGate->copyGate(copy);
        preGateInTheCircuit.insert(pair<int, gate*>(gateID, copy));
        faultsInsertedtheCircuit++;
        string name = "stuck-at" + to_string(stuckat);
        string stuckatStr = to_string(stuckat);
        gate *stuckatCons = new gate(name, stuckatStr);
        stuckatCons->gateID = theCircuit.size() + i;
        theCircuit.push_back(stuckatCons);
        if (port != 3) {  // gate input.
          if (faultGate->gateType == PI || faultGate->gateType == constant) {
            faultGate->gateType = bufInv;
          }
          if (port == 1) { // input1
            faultGate->fanin1 = stuckatCons;
            faultGate->in1Name = stuckatCons->outName;
          } else {         // input2
            faultGate->fanin2 = stuckatCons;
            faultGate->in2Name = stuckatCons->outName;
          }
        } else { // gate output.
          faultGate->changeOutputStuckat(stuckatCons);
        }
      }
      return 1;
    }

    int resetFaultsInCircuit() {
      if (theCircuit.size() == 0) return 0;
      gate *preGate;
      int gateID = 0;
      int outValue = 0;
      for (map<int, gate*>::iterator iter = preGateInTheCircuit.begin(); iter != preGateInTheCircuit.end(); iter++) {
        gateID = iter->first;
        outValue = theCircuit[gateID]->outValue;
        preGate = iter->second;
        preGate->copyGate(theCircuit[gateID]); // reset the gate
        if (theCircuit[gateID]->gateType != constant) {
          theCircuit[gateID]->outValue = outValue;  // the outvalue will kept(will be used to check propagation path latter)
        }
        delete preGate;
      }
      preGateInTheCircuit.clear();
      // delete the stuckat constant wire
      gate* deletedGate;
      while (faultsInsertedtheCircuit > 0) {
        faultsInsertedtheCircuit--;
        deletedGate = theCircuit[theCircuit.size() - 1];
        delete deletedGate;
        theCircuit.pop_back();
      }
      return 1;
    }
    //----------------inject and reset faults into original circuit--------------------

    // mode == 0, propagate the value in the entire circuit
    // mode == 1, propagate the value only in the given gates
    // if PI is first propagated in faulty circuit then propagate in correct circuit
    // if the new out value is different from the previous one, set it as the visited
    // whether it's really faulty gates need to check later
    void propagatePI(int mode, vector<uint64_t> &relatedGates){
      if (mode == 0) {
        for(int i = 0; i < theCircuit.size(); i++){
          int preValue = theCircuit[i]->outValue;
          theCircuit[i]->setOut();
          int curValue = theCircuit[i]->outValue;
          theCircuit[i]->different = (preValue != curValue);
        }
      } else {
        int w = 0;
        int size = relatedGates.size();
        while (w < size) {
          while (w < size && relatedGates[w] == 0) w++;
          if (w == size) break;
          uint64_t val = relatedGates[w];
          int gateID = w * WSIZE;
          for (int i = 0; i < WSIZE; i++) {
            if ((val & 1) == 1) {
              int preValue = theCircuit[gateID]->outValue;
              theCircuit[gateID]->setOut();
              int curValue = theCircuit[gateID]->outValue;
              theCircuit[gateID]->different = (preValue != curValue);
            }
            val >>= 1;
            gateID++;
          }
          w++;
        }
      }
    }


    // ----------------------propagate 64 values at the same time-------------------------------
    int assignPIs_64(vector<uint64_t> &inValues_64){
      if (inValues_64.size() != PISize){
        cout << "\n***Input vector does not match the size of PI***\n" << endl;
        return 0;
      }
      for (int i = 0; i < inValues_64.size(); i++){
          theCircuit[i]->setPI_64(inValues_64[i]);
      }
      return 1;
    }

    // mode == 0, propagate the value in the entire circuit
    // mode == 1, propagate the value only in the given gates
    void propagatePI_64(int mode, vector<uint64_t> &relatedGates, uint64_t max){
      if (mode == 0) {
        for(int i = 0; i < theCircuit.size(); i++){
          theCircuit[i]->setOut_64(max);
        }
      } else {
        int w = 0;
        int size = relatedGates.size();
        while (w < size) {
          while (w < size && relatedGates[w] == 0) w++;
          if (w == size) break;
          uint64_t val = relatedGates[w];
          int gateID = w * WSIZE;
          for (int i = 0; i < WSIZE; i++) {
            if ((val & 1) == 1) {
              theCircuit[gateID]->setOut_64(max);
            }
            val >>= 1;
            gateID++;
          }
          w++;
        }
      }
    }

    int getLeft1Pos(uint64_t i) {
      i = i & ~(i-1); // only the right most "1" will remain.
      int n = -1;
      if (i & 0xffffffff00000000) {
        n += 32;
        i >>= 32;
      }
      if (i & 0x00000000ffff0000) {
        n += 16;
        i >>= 16;
      };
      if (i & 0x000000000000ff00) {
        n += 8;
        i >>= 8;
      };
      if (i & 0x00000000000000f0) {
        n += 4;
        i >>= 4;
      };
      if (i & 0x000000000000000c) {
        n += 2;
        i >>= 2;
      };
      return n + i;
    }

    // propagate 64 set of PI to entire circuit one time.
    // return the index of test pattern that can detect the faults.
    // return -1 if no test pattern can detect that fault.
    int check64Patterns(vector<int> &newFaults, vector<uint64_t> &testVector_64, uint64_t bitMax) {
      vector<uint64_t> relatedGates = getAllRelatedGates(newFaults);
      vector<uint64_t> isDiff_64(POSize, 0);
      resetFaultsInCircuit();
      injectFaultsInCircuit(newFaults);
      assignPIs_64(testVector_64);
      propagatePI_64(1, relatedGates, bitMax);
      for (int i = 0; i < POSize; i++) {
        isDiff_64[i] = theCircuit[PISize + gateSize + i]->outValue_64;
      }
      resetFaultsInCircuit();
      assignPIs_64(testVector_64);
      propagatePI_64(1, relatedGates, bitMax);
      for (int i = 0; i < POSize; i++) {
        isDiff_64[i] = theCircuit[PISize + gateSize + i]->outValue_64 ^ isDiff_64[i];
      }
      for (int i = 0; i < POSize; i++) {
        if (isDiff_64[i] != 0) {
          return getLeft1Pos(isDiff_64[i]);
        }
      }
      return -1;
    }

    // return -1 if no test can detect the fault.
    // else return corresponding pattern index. And the corresponding pattern will be in "detectedPattern"
    // example
    //    b        0 1 0 1 1 0 ...
    // curNum      1 1 0 0 1 0 ...
    //             p             patternLength
    // patterns_64 64 64 64....(each number means one of the PI value of the 64 patterns)
    int checkallPatterns(vector<int> &newFaults, vector<vector<int>> &patterns) {
      if (patterns.size() == 0 || newFaults.size() == 0) return -1;
      int patternNum = patterns.size();
      int patternLength = patterns[0].size();
      vector<int> isDetected(patternNum, 0);
      // each time check 64 patterns simultaneous.
      int count = (patternNum - 1) / WSIZE;
      for (int i = 0; i <= count; i++) {
        vector<uint64_t> patterns_64(patternLength, 0);
        // put 64 patterns to pattern_64
        // if remaining patterns are smaller than 64, then only check the remaining ones.
        int curNum = (i < count) ? WSIZE : (patternNum - count * WSIZE);
        int startIdx = WSIZE * i;
        for (int p = 0; p < patternLength; p++) {
          for (int b = 0; b < curNum; b++) {
            patterns_64[p] |= ((uint64_t)patterns[startIdx + b][p] << b);
          }
        }
        uint64_t bitMax = (curNum == WSIZE) ? one_64 : (((uint64_t)1 << curNum) - 1);
        int patternIdx = check64Patterns(newFaults, patterns_64, bitMax);
        if (patternIdx >= 0) {
          return startIdx + patternIdx;
        }
      }
      return -1;
    }
    // ----------------------propagate 64 values at the same time-------------------------------

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
  };
}

#endif
