#ifndef _TESTGENEBYSAT_H
#define _TESTGENEBYSAT_H
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

using namespace std;
using namespace Gate;
using namespace Circuit;
using namespace Glucose;


namespace TESTGENEBYSAT {
  class testgenebysat {
  public:
    // ----Generate the CNF to do SAT------
    vector<gate*> oriAndFauCir;
    vector<vector<vector<int>>> CNFOriAndFauCir;
    // the gate changed due to the fault.
    // key : gateID in "oriAndFauCir", value is the gate in oriAndFauCir
    map<int, gate*> preGateInOriAndFauCir;
    map<int, vector<uint64_t>> gateToRelatedGates;
    int faultsInsertedoriAndFauCir;
    int copyCount;
    vector<gate*>theCircuit;
    int PISize;
    int gateSize;
    int POSize;
    static const int WSIZE = 64;

    testgenebysat(circuit *pCircuit) {
      this->theCircuit = pCircuit->theCircuit;
      this->faultsInsertedoriAndFauCir = 0;
      this->PISize = pCircuit->PISize;
      this->gateSize = pCircuit->gateSize;
      this->POSize = pCircuit->POSize;
      this->gateToRelatedGates = pCircuit->gateToRelatedGates;
      this->copyCount = 1;
      initialize();
    }

    ~testgenebysat() {}

    void initialize() {
      cout << "----------Initialization of CNF formula----------" << endl;
      double startTime, endTime, preTime, curTime;
      preTime = clock();
      generateOriAndFau(oriAndFauCir, theCircuit);
      generateCNF(oriAndFauCir, CNFOriAndFauCir);
      curTime = clock();
      cout << "----------Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds----------"  << endl;
    }

    // generate current clause in CNF format
    // ID in cnf formula is gate ID + 1. And it will be -1 in glucose.h
    void generateClause(vector<vector<int>> &gateClause, gate* curGate) {
      Type gateType = curGate->gateType;
      bool invIn1 = curGate->invIn1;
      bool invIn2 = curGate->invIn2;
      bool invOut = curGate->invOut;
      int gateID = curGate->gateID;
      gate *fanin1 = curGate->fanin1;
      gate *fanin2 = curGate->fanin2;
      bool outValue = curGate->outValue;

      gateClause.clear();
      vector<int>clause;
      // Generally CNF formula's ID starts from 1, but our circuits's gate ID start from 0
      // so we should do gateID + 1
      if (gateType == PI) {
        //(varOut + ~varOut). Means dont care.
        int varIn1 = (gateID+1)*(-1);
        int varOut = gateID+1;
        clause.push_back(varIn1);
        clause.push_back(varOut);
        gateClause.push_back(clause);
        clause.clear();
      } else if (gateType == PO || gateType == bufInv) {
        //(varIn1 + ~varOut)(~varIn1+varOut)
        int varIn1 = invIn1 ? (fanin1->gateID+1) : (fanin1->gateID+1)*(-1);
        int varOut = invOut ? (gateID+1) : (gateID+1)*(-1);
        clause.push_back(varIn1);
        clause.push_back(varOut*(-1));
        gateClause.push_back(clause);
        clause.clear();
        clause.push_back(varIn1*(-1));
        clause.push_back(varOut);
        gateClause.push_back(clause);
      } else if (gateType == constant) {
        // varOut * constant value
        int varOut = (gateID+1)*(outValue > 0 ? 1 : -1);
        clause.push_back(varOut);
        gateClause.push_back(clause);
      } else if (gateType == aig) { //AIG
        //(varIn1+~varOut)(varIn2+~varOut)(~varIn1+~varIn2+varOut)
        int varIn1 = invIn1 ? (fanin1->gateID+1) : (fanin1->gateID+1)*(-1);
        int varIn2 = invIn2 ? (fanin2->gateID+1) : (fanin2->gateID+1)*(-1);
        int varOut = invOut ? (gateID+1) : (gateID+1) * (-1);
        // (varIn1+~varOut)
        clause.push_back(varIn1);
        clause.push_back(varOut*(-1));
        gateClause.push_back(clause);
        clause.clear();
        // (varIn2+~varOut)
        clause.push_back(varIn2);
        clause.push_back(varOut*(-1));
        gateClause.push_back(clause);
        clause.clear();
        // (~varIn1+~varIn2+varOut)
        clause.push_back(varIn1*(-1));
        clause.push_back(varIn2*(-1));
        clause.push_back(varOut);
        gateClause.push_back(clause);
      } else if (gateType == OR) {
        //(~varIn1+varOut)(~varIn2+varOut)(varIn1+varIn2+~varOut)
        int varIn1 = invIn1 ? (fanin1->gateID+1) : (fanin1->gateID+1)*(-1);
        int varIn2 = invIn2 ? (fanin2->gateID+1) : (fanin2->gateID+1)*(-1);
        int varOut = invOut ? (gateID+1) : (gateID+1) * (-1);
        // (~varIn1+varOut)
        clause.push_back(varIn1*(-1));
        clause.push_back(varOut);
        gateClause.push_back(clause);
        clause.clear();
        // (~varIn2+varOut)
        clause.push_back(varIn2*(-1));
        clause.push_back(varOut);
        gateClause.push_back(clause);
        clause.clear();
        // (varIn1+varIn2+~varOut)
        clause.push_back(varIn1);
        clause.push_back(varIn2);
        clause.push_back(varOut*(-1));
        gateClause.push_back(clause);
      } else if (gateType == XOR) {
        //(varIn1+varIn2+~varOut)(varIn1+~varIn2+varOut)(~varIn1+~varIn2+~varOut)(~varIn1+varIn2+varOut)
        int varIn1 = invIn1 ? (fanin1->gateID+1) : (fanin1->gateID+1)*(-1);
        int varIn2 = invIn2 ? (fanin2->gateID+1) : (fanin2->gateID+1)*(-1);
        int varOut = invOut ? (gateID+1) : (gateID+1) * (-1);
        // (varIn1+varIn2+~varOut)
        clause.push_back(varIn1);
        clause.push_back(varIn2);
        clause.push_back(varOut*(-1));
        gateClause.push_back(clause);
        clause.clear();
        // (varIn1+~varIn2+varOut)
        clause.push_back(varIn1);
        clause.push_back(varIn2*(-1));
        clause.push_back(varOut);
        gateClause.push_back(clause);
        clause.clear();
        // (~varIn1+~varIn2+~varOut)
        clause.push_back(varIn1*(-1));
        clause.push_back(varIn2*(-1));
        clause.push_back(varOut*(-1));
        gateClause.push_back(clause);
        clause.clear();
        // (~varIn1+varIn2+varOut)
        clause.push_back(varIn1*(-1));
        clause.push_back(varIn2);
        clause.push_back(varOut);
        gateClause.push_back(clause);
        clause.clear();
      }
    }

    // cannot use copy gate, since if we copy a circuit, all fanin and fanout will be changed.
    void copyCircuit(vector <gate*> &copy) {
      for (int i = 0; i < theCircuit.size(); i++) {
        // new a gate, then change its information
        gate *newGate = new gate();
        newGate->gateType = theCircuit[i]->gateType;
        newGate->in1Name = theCircuit[i]->in1Name + "_" + to_string(copyCount);
        newGate->in2Name = theCircuit[i]->in2Name + "_" + to_string(copyCount);
        newGate->outName = theCircuit[i]->outName + "_" + to_string(copyCount);
        newGate->invIn1 = theCircuit[i]->invIn1;
        newGate->invIn2 = theCircuit[i]->invIn2;
        newGate->invOut = theCircuit[i]->invOut;
        newGate->isPath = theCircuit[i]->isPath;
        newGate->gateID = theCircuit[i]->gateID;
        newGate->outValue = theCircuit[i]->outValue;
        copy.push_back(newGate);
      }
      for (int i = 0; i < theCircuit.size(); i++) {
        // PI and constant dont have fanin
        if (theCircuit[i]->gateType != PI && theCircuit[i]->gateType != constant) {
          int fanin1ID = theCircuit[i]->fanin1->gateID;
          copy[i]->fanin1 = copy[fanin1ID];
        }
        // PO doesnt have fanout
        if (theCircuit[i]->gateType != PO) {
          for (int j = 0; j < theCircuit[i]->fanout.size(); j++) {
            int fanoutID = theCircuit[i]->fanout[j]->gateID;
            copy[i]->fanout.push_back(copy[fanoutID]);
          }
        }
        if (theCircuit[i]->gateType == aig || theCircuit[i]->gateType == OR || theCircuit[i]->gateType == XOR) {
          int fanin2ID = theCircuit[i]->fanin2->gateID;
          copy[i]->fanin2 = copy[fanin2ID];
        }
      }
      copyCount++;
    }

    //----------------------SAT Process----------------------
    // creat oriAndFauCir, which is used to generate the CNFFormula
    // oriAndFauCir: original circuit | faulty circuit | new input | new XOR | new output
    // connect the new input with the original and faulty circuits. Do the same things for XOR output.
    void generateOriAndFau(vector<gate*> &oriAndFauCir, vector<gate*> &curCircuit) {
      oriAndFauCir.reserve(curCircuit.size() * 3);
      copyCircuit(oriAndFauCir);
      vector<gate*> temp;
      copyCircuit(temp);
      oriAndFauCir.insert(oriAndFauCir.end(), temp.begin(), temp.end());
      int originalSize = curCircuit.size();
      // generate new inputs, and connect them with the input wires of origianl and faulty circuits
      // change gate type(PI->buf)
      for (int i = 0; i < PISize; i++) {
        string name = "newIN_"+curCircuit[i]->outName;
        gate *newInput = new gate(PI, name);
        oriAndFauCir.push_back(newInput);
        gate *originalInput = oriAndFauCir[i];
        gate *faultyInput = oriAndFauCir[i+originalSize];
        newInput->fanout.push_back(originalInput);
        newInput->fanout.push_back(faultyInput);
        faultyInput->fanin1 = newInput;
        faultyInput->gateType = bufInv;
        faultyInput->in1Name = newInput->outName;
        originalInput->fanin1 = newInput;
        originalInput->gateType = bufInv;
        originalInput->in1Name = newInput->outName;
      }
      // generate the XOR gates, change the gate type of original PO to buf;
      vector<gate*> finalOutput;
      for (int i = 0; i < POSize; i++) {
        gate *originalOutput = oriAndFauCir[PISize+gateSize+i];
        gate *faultyOutput = oriAndFauCir[originalSize+PISize+gateSize+i];
        // fanin1 for original output, fanin2 for faulty output.
        string oriOutName = curCircuit[PISize+gateSize+i]->outName;
        string in1Name = originalOutput->outName, in2Name = faultyOutput->outName, outName = "XOR_"+oriOutName;
        string invIn1 = "1", invIn2 = "1", invOut = "1";
        gate *newXOR = new gate(XOR, in1Name, in2Name, outName, invIn1, invIn2, invOut);
        oriAndFauCir.push_back(newXOR);
        originalOutput->fanout.push_back(newXOR);
        originalOutput->gateType = bufInv;
        faultyOutput->fanout.push_back(newXOR);
        faultyOutput->gateType = bufInv;
        newXOR->fanin1 = originalOutput;
        newXOR->fanin2 = faultyOutput;
        //connect all XOR with output wires
        string name = "newOut_"+oriOutName;
        gate *newOutput = new gate(PO, name);
        newOutput->in1Name = newXOR->outName;
        finalOutput.push_back(newOutput);
        newOutput->fanin1 = newXOR;
        newXOR->fanout.push_back(newOutput);
      }
      oriAndFauCir.insert(oriAndFauCir.end(), finalOutput.begin(), finalOutput.end());
      // re-assign the gateID
      for (int i = 0; i < oriAndFauCir.size(); i++) {
        oriAndFauCir[i]->gateID = i;
      }
    }

    // generate CNF formula,only generate for one time, and it will be reused and held in memoery
    // covert all gates into CNF
    // at last insert the clause that (out1 + out2 + out3 + ..) ensure at least one of the output is SAT
    // CNF formula:
    // original circuit | faulty circuit | new input | new XOR | new output | an "OR" gate for all outputs
    void generateCNF(vector <gate*> &curCircuit, vector<vector<vector<int>>> &CNFOriAndFauCir) {
      CNFOriAndFauCir.reserve(curCircuit.size() * 3);
      vector<vector<int>> gateClause;
      vector<int> output;
      for (int i = 0; i < curCircuit.size(); i++) {
        gate *curGate = curCircuit[i];
        generateClause(gateClause, curGate);
        CNFOriAndFauCir.push_back(gateClause);
        if (curGate->gateType == PO) {
          output.push_back(i+1);
        }
        gateClause.clear();
      }
      // insert the clause : (out1 + out2 + out3 + ..)
      gateClause.clear();
      gateClause.push_back(output);
      CNFOriAndFauCir.push_back(gateClause);
      // push_back a blank gate to make the "CNFOriAndFauCir" and "oriAndFauCir" size becoming the same.
      oriAndFauCir.push_back(new gate());
    }

    // insert stuck-at faults into CNF formula "CNForiAndFauCir". Can be used for any number of faults
    // previous faults will be reset if we call this function.
    // the fault can be inserted to any gates output(output's fault can be collapsed and combined with input's faults).
    // If there are two faults happening in the same place, only the front one will go into effect.
    // CNF formula:
    // original circuit | faulty circuit | new input | new XOR | new output | an "OR" gate for all outputs | constant wire(stuck at faults)
    // circuit size        circuit size    PI size      PO size   PO size           1
    // creat oriAndFauCir, which is used to generate the CNFFormula
    // oriAndFauCir: original circuit | faulty circuit | new input | new XOR | new output | a blank gate
    int injectFaultsInCNF(vector<int> &newFaults) {
      int originalSize = theCircuit.size();
      if (originalSize == 0) return 0;
      int preSize = originalSize + originalSize + PISize + POSize + POSize + 1;
      vector<vector<int>> gateClause;
      // inject new faults
      for (int i = 0; i < newFaults.size(); i++) {
        int faultID = newFaults[i];
        int oriGateID = (faultID >> 3);
        int fauGateID = oriGateID + originalSize;
        int port = (faultID >> 1) & 3;
        int stuckat = faultID & 1;
        int gateType = theCircuit[oriGateID]->gateType;
        gate *faultGate = oriAndFauCir[fauGateID];
        if ((faultGate->gateType != aig && faultGate->gateType != OR && faultGate->gateType != XOR) || port == 0){
          if (port == 2 || port == 0) {
            cout << "Skip this fault since it has problem." << endl;
            printFault(faultID);
            continue;
          }
        }
        gate *copy = new gate();
        faultGate->copyGate(copy);
        preGateInOriAndFauCir.insert(pair<int, gate*>(fauGateID, copy));
        faultsInsertedoriAndFauCir++;
        // contant wire connected to the stuckat inputs
        // name by stuckat+faultID
        string name = "stuck-at"+to_string(stuckat);
        string stuckatStr = to_string(stuckat);
        gate *stuckatCons = new gate(name, stuckatStr);
        stuckatCons->gateID = preSize + i;
        oriAndFauCir.push_back(stuckatCons);
        if (port != 3) {  // input wire
          if (gateType == PI || gateType == constant) {
            faultGate->gateType = bufInv;
          }
          if (port == 1) { // input1
            faultGate->fanin1 = stuckatCons;
            faultGate->in1Name = stuckatCons->outName;
          } else {         // input2
            faultGate->fanin2 = stuckatCons;
            faultGate->in2Name = stuckatCons->outName;
          }
        } else {  // output wire. change the gate to bufinv, whose input connect to stuck at constant wire.
          faultGate->gateType = bufInv;
          faultGate->fanin1 = stuckatCons;
          faultGate->in1Name = stuckatCons->outName;
          faultGate->invIn1 = 1;
          faultGate->invOut = 1;
        }
        // insert the constant stuckat wires to the end of the CNF formula
        // in the previous operation, we just clear stuckat constant wire's vector but not delete them (to reduce time complexity)
        // so here we can reuse these vectors
        // stuckatCons->generateClause(gateClause);
        generateClause(gateClause, stuckatCons);
        CNFOriAndFauCir.push_back(gateClause);
        gateClause.clear();
        // faultGate->generateClause(gateClause);
        generateClause(gateClause, faultGate);
        CNFOriAndFauCir[fauGateID].clear();
        CNFOriAndFauCir[fauGateID].assign(gateClause.begin(), gateClause.end());
        gateClause.clear();
      }
      return 1;
    }

    int resetFaultsInCNF() {
      int originalSize = theCircuit.size();
      if (originalSize == 0) return 0;
      gate *preFauGate;
      gate *deletedGate;
      vector<vector<int>> gateClause;
      // reset the circuit "oriAndFauCir" and "CNFOriAndFauCir"
      for (map<int, gate*>::iterator iter = preGateInOriAndFauCir.begin(); iter != preGateInOriAndFauCir.end(); iter++) {
        // bit index:     [ ,3]        [2  1]    0
        //                gateID       port     stuckat
        // port: 01 input1, 10 input2, 11 output
        int fauGateID = iter->first;
        preFauGate = iter->second;
        preFauGate->copyGate(oriAndFauCir[fauGateID]);
        gateClause.clear();
        generateClause(gateClause, preFauGate);
        // preFauGate->generateClause(gateClause);
        delete preFauGate;
        deletedGate = oriAndFauCir[oriAndFauCir.size() - 1];
        delete deletedGate;
        oriAndFauCir.pop_back();
        CNFOriAndFauCir[fauGateID].clear();
        CNFOriAndFauCir[fauGateID].assign(gateClause.begin(), gateClause.end());
      }
      preGateInOriAndFauCir.clear();
      while (faultsInsertedoriAndFauCir > 0) {
        faultsInsertedoriAndFauCir--;
        CNFOriAndFauCir.pop_back();  // delete the constant wire which put in last time
      }
      return 1;
    }

    // return 1 if SAT. else UNSAT, which means redundant
    int generateTestBySAT(vector<int> &newFaults, vector<int> &testVector) {
      glucose *SATSolver = new glucose();
      resetFaultsInCNF();
      injectFaultsInCNF(newFaults);
      int result = SATSolver->SATCircuit(CNFOriAndFauCir, testVector, theCircuit.size(), PISize);
      delete SATSolver;
      return result;
    }
    //----------------------SAT Process---------------------------

    int faultIDToGateID(int faultID) {
      return (faultID >> 3);
    }

    int isSet(int pos, vector<uint64_t> &vec) {
      int w = pos / WSIZE;
      int b = pos % WSIZE;
      uint64_t val = (vec[w] >> b);
      return (val & 1ULL) > 0 ? 1 : 0;
    }

    // get the related circuit; and all the PI ID in that part of circuit.
    void getAllRelatedGates(vector<int> &faults, vector<gate*> &curCircuit, vector<int> &PIIDs, vector<int> &gatePOSize, map<int,int> &oriCirIDToCur, vector<uint64_t> &relatedGates) {
      for (auto faultID : faults) {
        int gateID = faultIDToGateID(faultID);
        for (int i = 0; i < relatedGates.size(); i++) {
          relatedGates[i] |= gateToRelatedGates[gateID][i];
        }
      }
      int w = 0;
      int size = relatedGates.size();
      gate *curGate;
      int curID = 0;
      while (w < size) {
        while (w < size && relatedGates[w] == 0) w++;
        if (w == size) break;
        uint64_t val = relatedGates[w];
        int gateID = w * WSIZE;
        for (int i = 0; i < WSIZE; i++) {
          if ((val & 1) == 1) {
            curGate = theCircuit[gateID];
            curCircuit.push_back(curGate);
            oriCirIDToCur.insert(make_pair(gateID, curID));
            curID++;
            if (curGate->gateType == PI) {
              PIIDs.push_back(gateID);
            } else if (curGate->gateType == PO) {
              gatePOSize[1]++;
            } else {
              gatePOSize[0]++;
            }
          }
          val >>= 1;
          gateID++;
        }
        w++;
      }
    }

    void copyCircuit_1(vector <gate*> &copy, vector<gate*> &curCircuit, map<int, int> &oriCirIDToCur, int copyCount, vector<uint64_t> &relatedGates) {
      for (int i = 0; i < curCircuit.size(); i++) {
        // new a gate, then change its information
        gate *newGate = new gate();
        newGate->gateType = curCircuit[i]->gateType;
        newGate->in1Name = curCircuit[i]->in1Name + "_" + to_string(copyCount);
        newGate->in2Name = curCircuit[i]->in2Name + "_" + to_string(copyCount);
        newGate->outName = curCircuit[i]->outName + "_" + to_string(copyCount);
        newGate->invIn1 = curCircuit[i]->invIn1;
        newGate->invIn2 = curCircuit[i]->invIn2;
        newGate->invOut = curCircuit[i]->invOut;
        newGate->isPath = curCircuit[i]->isPath;
        newGate->gateID = curCircuit[i]->gateID;
        newGate->outValue = curCircuit[i]->outValue;
        newGate->gateID = i;
        copy.push_back(newGate);
      }
      gate *curGate;
      for (int i = 0; i < curCircuit.size(); i++) {
        curGate = curCircuit[i];
        // PI and constant have no fanin
        if (curGate->gateType != PI && curGate->gateType != constant) {
          int fanin1ID = oriCirIDToCur[curGate->fanin1->gateID];
          copy[i]->fanin1 = copy[fanin1ID];
        }
        if (curGate->gateType == aig || curGate->gateType == OR || curGate->gateType == XOR) {
          int fanin2ID = oriCirIDToCur[curGate->fanin2->gateID];
          copy[i]->fanin2 = copy[fanin2ID];
        }
        // PO doesnt have fanout
        if (curGate->gateType != PO) {
          for (int j = 0; j < curGate->fanout.size(); j++) {
            int fanoutID = curGate->fanout[j]->gateID;
            if (isSet(fanoutID, relatedGates) == 1) {
              fanoutID = oriCirIDToCur[fanoutID];
              copy[i]->fanout.push_back(copy[fanoutID]);
            }
          }
        }
      }
    }

    // oriAndFauCir: original circuit | faulty circuit | new input | new XOR | new output
    void generateOriAndFau_1(vector<gate*> &oriAndFauCir, vector<gate*> &curCircuit, int PISize, int gateSize, int POSize, map<int, int> &oriCirIDToCur, vector<uint64_t> &relatedGates) {
      oriAndFauCir.reserve(curCircuit.size() * 3);
      copyCircuit_1(oriAndFauCir, curCircuit, oriCirIDToCur, 1, relatedGates);
      vector<gate*> temp;
      temp.reserve(curCircuit.size());
      copyCircuit_1(temp, curCircuit, oriCirIDToCur, 2, relatedGates);
      oriAndFauCir.insert(oriAndFauCir.end(), temp.begin(), temp.end());
      int originalSize = curCircuit.size();
      // generate new inputs, and connect them with the input wires of origianl and faulty circuits
      // change gate type(PI->buf)
      for (int i = 0; i < PISize; i++) {
        string name = "newIN_" + curCircuit[i]->outName;
        gate *newInput = new gate(PI, name);
        oriAndFauCir.push_back(newInput);
        gate *originalInput = oriAndFauCir[i];
        gate *faultyInput = oriAndFauCir[i + originalSize];
        newInput->fanout.push_back(originalInput);
        newInput->fanout.push_back(faultyInput);
        faultyInput->fanin1 = newInput;
        faultyInput->gateType = bufInv;
        faultyInput->in1Name = newInput->outName;
        originalInput->fanin1 = newInput;
        originalInput->gateType = bufInv;
        originalInput->in1Name = newInput->outName;
      }
      // generate the XOR gates, change the gate type of original PO to buf;
      vector<gate*> finalOutput;
      for (int i = 0; i < POSize; i++) {
        gate *originalOutput = oriAndFauCir[PISize+gateSize+i];
        gate *faultyOutput = oriAndFauCir[originalSize+PISize+gateSize+i];
        // fanin1 for original output, fanin2 for faulty output.
        string oriOutName = curCircuit[PISize+gateSize+i]->outName;
        string in1Name = originalOutput->outName, in2Name = faultyOutput->outName, outName = "XOR_"+oriOutName;
        string invIn1 = "1", invIn2 = "1", invOut = "1";
        gate *newXOR = new gate(XOR, in1Name, in2Name, outName, invIn1, invIn2, invOut);
        oriAndFauCir.push_back(newXOR);
        originalOutput->fanout.push_back(newXOR);
        originalOutput->gateType = bufInv;
        faultyOutput->fanout.push_back(newXOR);
        faultyOutput->gateType = bufInv;
        newXOR->fanin1 = originalOutput;
        newXOR->fanin2 = faultyOutput;
        //connect all XOR with output wires
        string name = "newOut_"+oriOutName;
        gate *newOutput = new gate(PO, name);
        newOutput->in1Name = newXOR->outName;
        finalOutput.push_back(newOutput);
        newOutput->fanin1 = newXOR;
        newXOR->fanout.push_back(newOutput);
      }
      oriAndFauCir.insert(oriAndFauCir.end(), finalOutput.begin(), finalOutput.end());
      // re-assign the gateID
      for (int i = 0; i < oriAndFauCir.size(); i++) {
        oriAndFauCir[i]->gateID = i;
      }
    }

    // oriAndFauCir: original circuit | faulty circuit | new input | new XOR | new output | stuck at values.
    int injectFaultsInOriAndFau_1(vector<int> &newFaults, vector<gate*> &curCircuit, vector<gate*> &oriAndFauCir, int PISize, int POSize, map<int,int> &oriCirIDToCur) {
      int originalSize = curCircuit.size();
      if (originalSize == 0) return 0;
      int preSize = originalSize + originalSize + PISize + POSize + POSize + 1;
      vector<vector<int>> gateClause;
      // inject new faults
      for (int i = 0; i < newFaults.size(); i++) {
        int faultID = newFaults[i];
        int oriGateID = oriCirIDToCur[(faultID >> 3)];
        int fauGateID = oriGateID + originalSize;
        int port = (faultID >> 1) & 3;
        int stuckat = faultID & 1;
        int gateType = curCircuit[oriGateID]->gateType;
        gate *faultGate = oriAndFauCir[fauGateID];
        if ((faultGate->gateType != aig && faultGate->gateType != OR && faultGate->gateType != XOR) || port == 0){
          if (port == 2 || port == 0) {
            cout << "Skip this fault since it has problem." << endl;
            printFault(faultID);
            continue;
          }
        }
        // contant wire connected to the stuckat inputs
        // name by stuckat+faultID
        string name = "stuck-at"+to_string(stuckat);
        string stuckatStr = to_string(stuckat);
        gate *stuckatCons = new gate(name, stuckatStr);
        stuckatCons->gateID = preSize + i;
        oriAndFauCir.push_back(stuckatCons);
        if (port != 3) {  // input wire
          if (gateType == PI || gateType == constant) {
            faultGate->gateType = bufInv;
          }
          if (port == 1) { // input1
            faultGate->fanin1 = stuckatCons;
            faultGate->in1Name = stuckatCons->outName;
          } else {         // input2
            faultGate->fanin2 = stuckatCons;
            faultGate->in2Name = stuckatCons->outName;
          }
        } else {  // output wire. change the gate to bufinv, whose input connect to stuck at constant wire.
          faultGate->gateType = bufInv;
          faultGate->fanin1 = stuckatCons;
          faultGate->in1Name = stuckatCons->outName;
          faultGate->invIn1 = 1;
          faultGate->invOut = 1;
        }
      }
      return 1;
    }

    // CNF:          original circuit | faulty circuit | new input | new XOR | new output | stuck at values | an "OR" gate for all outputs
    void generateCNF_1(vector <gate*> &oriAndFauCir, vector<vector<vector<int>>> &CNFOriAndFauCir) {
      CNFOriAndFauCir.reserve(oriAndFauCir.size() + 10);
      vector<vector<int>> gateClause;
      vector<int> output;
      for (int i = 0; i < oriAndFauCir.size(); i++) {
        gate *curGate = oriAndFauCir[i];
        generateClause(gateClause, curGate);
        CNFOriAndFauCir.push_back(gateClause);
        if (curGate->gateType == PO) {
          output.push_back(i + 1);
        }
        gateClause.clear();
      }
      // insert the clause : (out1 + out2 + out3 + ..)
      gateClause.clear();
      gateClause.push_back(output);
      CNFOriAndFauCir.push_back(gateClause);
    }

    void generateRelatedCNF(vector<int> &newFaults, vector<gate*> &curCircuit, vector<vector<vector<int>>> &CNFOriAndFauCir, vector<int> &PIIDs, vector<gate*> &oriAndFauCir) {
      map<int, int> oriCirIDToCur;
      vector<int> gatePOSize;
      vector<uint64_t> relatedGates((theCircuit.size() - 1) / WSIZE + 1, 0);
      gatePOSize.push_back(0); gatePOSize.push_back(0);
      getAllRelatedGates(newFaults, curCircuit, PIIDs, gatePOSize, oriCirIDToCur, relatedGates);
      int gateSize = gatePOSize[0];
      int POSize = gatePOSize[1];
      generateOriAndFau_1(oriAndFauCir, curCircuit, PIIDs.size(), gateSize, POSize, oriCirIDToCur, relatedGates);
      injectFaultsInOriAndFau_1(newFaults, curCircuit, oriAndFauCir, PIIDs.size(), POSize, oriCirIDToCur);
      generateCNF_1(oriAndFauCir, CNFOriAndFauCir);
    }

    // testVectorTmp contains the test for the related Circuit's input, we need to add the dont care values.
    void getCompletedTestVector(vector<int> &PIIDs, vector<int> &testVectorTmp, vector<int> &testVector) {
      int PIIDCount = 0;
      for (int i = 0; i < PISize; i++) {
        if (PIIDCount < PIIDs.size() && PIIDs[PIIDCount] == i) {
          testVector.push_back(testVectorTmp[PIIDCount]);
          PIIDCount++;
        } else {
          //testVector.push_back(0);  // the PI that we dont care.
          testVector.push_back(rand() % 2);
        }
      }
    }

    int generateTestBySAT_1(vector<int> &newFaults, vector<int> &testVector) {
      vector<gate*> curCircuit;
      vector<int> PIIDs;
      vector<gate*> oriAndFauCir;
      vector<vector<vector<int>>> CNFOriAndFauCir;
      vector<int> testVectorTmp;
      generateRelatedCNF(newFaults, curCircuit, CNFOriAndFauCir, PIIDs, oriAndFauCir);
      glucose *SATSolver = new glucose();
      int result = SATSolver->SATCircuit(CNFOriAndFauCir, testVectorTmp, curCircuit.size(), PIIDs.size());
      if (result == 1) {
        getCompletedTestVector(PIIDs, testVectorTmp, testVector);
      }
      for (auto curGate : oriAndFauCir) {
        delete curGate;
      }
      delete SATSolver;
      return result;
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

  };
}

#endif
