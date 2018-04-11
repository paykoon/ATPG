#ifndef _ATPG_H
#define _ATPG_H

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
#include "time.h"
#include "Gate.h"
#include "glucose.h"
#include <stdlib.h>
#include "Circuit.h"
#include "Gate.h"
using namespace std;
using namespace Gate;
using namespace Circuit;
using namespace Glucose;

namespace ATPG{
  class atpg{
    public:
      atpg(circuit *pCircuit, char *patternFile) {
        copyCount = 1;
        this->PISize = pCircuit->PISize;
        this->POSize = pCircuit->POSize;
        this->gateSize = pCircuit->gateSize;
        this->theCircuit = pCircuit->theCircuit;
        ClassCircuit = pCircuit;
        ATPGInit(patternFile);
      }

      ~atpg() {
        delete this;
      }

      // if initialization has problem, return 0
      // else return 1;
      int ATPGInit(char *patternFile) {
        double startTime, endTime, preTime, curTime;
        startTime = clock();
        preTime = clock();
        cout << endl << "----------Initialization of ATPG----------" << endl;
        cout << "1. Fault list generating is started. " << endl;
        if ( !generateFaultList() ) return 0;
        curTime = clock();
        cout << "   Fault list generating is completed. Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        cout << "   Fault number is " << collapsedFaultList.size() << " (Collapsed: AIG's input wire connected to PI or fanout)" << endl;
        cout << "   Total faults number: " << allFaultList.size() << ", compressed ratio " << (double)(collapsedFaultList.size()) / (double)(allFaultList.size())<< endl;

        cout << "2. Initial CNF generating is started." << endl;
        preTime = clock();
        generateOriAndFau(oriAndFauCir);
        generateCNF(oriAndFauCir);
        curTime = clock();
        cout << "   Initial CNF generating is completed. Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;

        cout << "3. Read initial SSAF test patterns." << endl;
        if(patternParser(patternFile) == 1) {
          cout << "   " << SSAFPatterns.size() << " SSAF patterns are read" << endl;
        } else {
          cout << "   Pattern file has problem"<< endl;
          return 0;
        }
        cout << "   Check the coverage of the initial SSAF test patterns." << endl;

        vector<int> newFaults;
        newFaults.push_back(1252);
        injectFaultsInCNF(newFaults);
        printCircuitCNFBlif(oriAndFauCir);

        /*
        CheckGivenPatterns(collapsedFaultList, SSAFPatterns);
        printFaults(redundantSSAF);cout << endl << endl << endl;

        TestSSAFPatterns(collapsedFaultList);
        printFaults(redundantSSAF);cout << endl << endl << endl;

        cout << "\n\nredundant" << endl;
        redundantFaultTest(collapsedFaultList);
        */
/*
        preTime = clock();
        findAllSSAFRedundant(allFaultList);
        curTime = clock();
        cout << "4. Find all redundant single stuck-at faults. Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        if (redundantSSAF.size() > 0) {
          cout << "   " << redundantSSAF.size() << " redundant SSAF found"<< "" << endl;
          printFaults(redundantSSAF);
        } else {
          cout << "The circuit has no redundant single stuck-at fault." << endl;
        }
        */
        endTime = clock();
        cout << "----------The initialization of the ATPG takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------" << endl << endl;
        return 1;
      }

      // return 0 if nothing is inside the circuit
      // 1. generate the faults that locate at aig input wire connecting to the PI or fanout.
      // 2. also generate all faults(without collapsing)
      // fault number's meaning:
      // bit index:     [ ,3]        [2  1]    0
      //                gateID       port     stuckat
      int generateFaultList(){
        if(theCircuit.size() == 0)  return 0;
        int newFault;
        // generate the faults according to the fault selection model
        for(int i = 0;i < theCircuit.size(); i++){
          if(theCircuit[i]->gateType != aig) continue;
          // the faults in the AIG input which are connected to PI or fanouts
          if(theCircuit[i]->fanin1->gateType == PI || theCircuit[i]->fanin1->fanout.size() > 1){
            newFault = (i << 3) + (1 << 1) + 0;   //stuck-at 0 at input 1 of gate i
            collapsedFaultList.insert(newFault);
            newFault = (i << 3) + (1 << 1) + 1;   //stuck-at 1 at input 1 of gate i
            collapsedFaultList.insert(newFault);
          }
          if(theCircuit[i]->fanin2->gateType == PI || theCircuit[i]->fanin2->fanout.size() > 1){
            newFault = (i << 3) + (2 << 1) + 0;   //stuck-at 0 at input 2 of gate i
            collapsedFaultList.insert(newFault);
            newFault = (i << 3) + (2 << 1) + 1;   //stuck-at 1 at input 2 of gate i
            collapsedFaultList.insert(newFault);
          }
        }
        // generate all faults
        for (int i = 0; i < theCircuit.size(); i++) {
          // all faults(in input)
          newFault = (i << 3) + (1 << 1) + 0;   //stuck-at 0 at input 1 of gate i
          allFaultList.insert(newFault);
          newFault = (i << 3) + (1 << 1)+ 1;   //stuck-at 1 at input 1 of gate i
          allFaultList.insert(newFault);
          newFault = (i << 3) + (3 << 1) + 0;   //stuck-at 0 at output of gate i
          allFaultList.insert(newFault);
          newFault = (i << 3) + (3 << 1) + 1;   //stuck-at 1 at output of gate i
          allFaultList.insert(newFault);
          Type gateType = theCircuit[i]->gateType;
          if (gateType == XOR || gateType == OR || gateType == aig) {
            newFault = (i << 3) + (2 << 1) + 0;   //stuck-at 0 at input 2 of gate i
            allFaultList.insert(newFault);
            newFault = (i << 3) + (2 << 1)+ 1;   //stuck-at 1 at input 2 of gate i
            allFaultList.insert(newFault);
          }
        }
        // generate the faults that all not selected by the fault model
        for (set<int>::iterator allIter = allFaultList.begin(); allIter != allFaultList.end(); allIter++) {
          set<int>::iterator collIter = collapsedFaultList.find(*allIter);
          if (collIter == collapsedFaultList.end()) {
            notIncollapsedFaultList.insert(*allIter);
          }
        }
        return 1;
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
          newGate->faultyOut = theCircuit[i]->faultyOut;
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

      //--------------------------------------
      // creat oriAndFauCir, which is used to generate the CNFFormula
      // oriAndFauCir: original circuit | faulty circuit | new input | new XOR | new output
      // connect the new input with the original and faulty circuits. Do the same things for XOR output.
      void generateOriAndFau(vector<gate*> &oriAndFauCir) {
        oriAndFauCir.reserve(theCircuit.size() * 3);
        copyCircuit(oriAndFauCir);
        vector<gate*> temp;
        copyCircuit(temp);
        oriAndFauCir.insert(oriAndFauCir.end(), temp.begin(), temp.end());
        int origialSize = theCircuit.size();
        // generate new inputs, and connect them with the input wires of origianl and faulty circuits
        // change gate type(PI->buf)
        for (int i = 0; i < PISize; i++) {
          string name = "newIN_"+theCircuit[i]->outName;
          gate *newInput = new gate(PI, name);
          oriAndFauCir.push_back(newInput);
          gate *originalInput = oriAndFauCir[i];
          gate *faultyInput = oriAndFauCir[i+origialSize];
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
          gate *faultyOutput = oriAndFauCir[origialSize+PISize+gateSize+i];
          // fanin1 for original output, fanin2 for faulty output.
          string oriOutName = theCircuit[PISize+gateSize+i]->outName;
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
      void generateCNF(vector <gate*> &curCircuit) {
        CNFOriAndFauCir.reserve(curCircuit.size()*3);
        vector<vector<int>> gateClause;
        vector<int> output;
        for (int i = 0; i < curCircuit.size(); i++) {
          gate *curGate = curCircuit[i];
          curGate->generateClause(gateClause);
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
        int origialSize = theCircuit.size();
        if (origialSize == 0) return 0;
        int preSize = origialSize + origialSize + PISize + POSize + POSize + 1;
        vector<vector<int>> gateClause;
        // reset the circuit "oriAndFauCir" and "CNFOriAndFauCir"
        for (map<int, gate*>::iterator iter = preGateInOriAndFauCir.begin(); iter != preGateInOriAndFauCir.end(); iter++) {
          // bit index:     [ ,3]        [2  1]    0
          //                gateID       port     stuckat
          // port: 01 input1, 10 input2, 11 output
          int fauGateID = iter->first;
          gate *preFauGate = iter->second;
          preFauGate->copyGate(oriAndFauCir[fauGateID]);
          preFauGate->generateClause(gateClause);
          oriAndFauCir.pop_back();
          CNFOriAndFauCir[fauGateID].clear();
          CNFOriAndFauCir[fauGateID].assign(gateClause.begin(), gateClause.end());
          CNFOriAndFauCir.pop_back();  // delete the constant wire which put in last
          gateClause.clear();
        }
        preGateInOriAndFauCir.clear();

        // inject new faults
        for (int i = 0; i < newFaults.size(); i++) {
          int faultID = newFaults[i];
          int oriGateID = (faultID >> 3);
          int fauGateID = oriGateID + origialSize;
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
          // contant wire connected to the stuckat inputs
          // name by stuckat+faultID
          string name = "stuck-at"+to_string(stuckat);
          string stuckatStr = to_string(stuckat);
          gate *stuckatCons = new gate(name, stuckatStr);
          stuckatCons->gateID = preSize + i;
          oriAndFauCir.push_back(stuckatCons);
          if (port != 3) {  // input wire
            if (gateType == constant) {
              faultGate->outValue = stuckat;
            } else {
              if (port == 1) { // input1
                faultGate->fanin1 = stuckatCons;
                faultGate->in1Name = stuckatCons->outName;
              } else {         // input2
                faultGate->fanin2 = stuckatCons;
                faultGate->in2Name = stuckatCons->outName;
              }
            }
          } else {  // output wire. change the gate to bufinv, whose input connect to stuck at constant wire.
            faultGate->gateType = bufInv;
            faultGate->fanin1 = stuckatCons;
            faultGate->in1Name = stuckatCons->outName;
          }
          // insert the constant stuckat wires to the end of the CNF formula
          // in the previous operation, we just clear stuckat constant wire's vector but not delete them (to reduce time complexity)
          // so here we can reuse these vectors
          stuckatCons->generateClause(gateClause);
          CNFOriAndFauCir.push_back(gateClause);
          gateClause.clear();
          faultGate->generateClause(gateClause);
          CNFOriAndFauCir[fauGateID].clear();
          CNFOriAndFauCir[fauGateID].assign(gateClause.begin(), gateClause.end());
          gateClause.clear();
        }
        return 1;
      }
      //--------------------------------------

      //--------------------------------------
      // find all redundant single stuck-at faults
      /*
      void findAllSSAFRedundant(set<int> &allFaultList) {
        vector<int> fault;
        for (set<int>::iterator iter = allFaultList.begin(); iter != allFaultList.end(); iter++) {
          fault.push_back(*iter);
          injectFaultsInCNF(fault);
          fault.clear();
          vector<int> SATResult;
          // The fault is redundant if no test vector is found.
          if(SATCircuit(CNFOriAndFauCir, SATResult) == 0) {
            redundantSSAF.insert(*iter);
          }
        }
      }
      */

      // send the entire CNFFormula to GLucose to do SAT.
      int SATCircuit(vector<vector<vector<int>>> &CNFOriAndFauCir, vector<int> &result) {
        if (CNFOriAndFauCir.size() == 0) return 0;
        //double startTime = clock();
        glucose *SATSolver = new glucose();
        vector<int> allValue;
        if (SATSolver->runGlucose(CNFOriAndFauCir, allValue)) {
          //cout << "SAT" << endl;
          // only take the PI value in "allValue" as result
          // original circuit | faulty circuit | new input | new XOR | new output | an "OR" gate for all outputs | constant wire(stuck at faults)
          // circuit size        circuit size    PI size      PO size   PO size           1
          int start = theCircuit.size() * 2;
          result.reserve(PISize);
          for (int i = 0; i < PISize; i++) {
            result.push_back(allValue[i+start]);
          }
          return 1;
        }
        else {
          //cout << "UNSAT" << endl;
          return 0;
        }
        //double endTime = clock();
        //cout << "The SAT takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds" << endl;
      }

      // return 1 if SAT. else UNSAT, which means redundant
      int generateTestBySAT(vector<int> &newFaults, vector<int> &testVector) {
        injectFaultsInCNF(newFaults);
        if (SATCircuit(CNFOriAndFauCir, testVector) == 1) {
          return 1;
        } else {
          return 0;
        }
      }
      //--------------------------------------

      // inject faults in theCircuit
      int injectFaultsInCircuit(vector<int> &newFaults) {
        if (theCircuit.size() == 0) return 0;
        for (int i = 0; i < newFaults.size(); i++) {
          int faultID = newFaults[i];
          int gateID = (faultID >> 3);
          int port = (faultID >> 1) & 3;
          int stuckat = faultID & 1;
          gate *faultGate = theCircuit[gateID];
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
          string name = "stuck-at"+to_string(stuckat);
          string stuckatStr = to_string(stuckat);
          gate *stuckatCons = new gate(name, stuckatStr);
          stuckatCons->gateID = theCircuit.size() + i;
          theCircuit.push_back(stuckatCons);
          if (port != 3) {  // input wire
            if (faultGate->gateType == constant) {
              faultGate->outValue = stuckat;
            } else {
              if (faultGate->gateType == PI) {
                faultGate->gateType = bufInv;
              }
              if (port == 1) { // input1
                faultGate->fanin1 = stuckatCons;
                faultGate->in1Name = stuckatCons->outName;
              } else {         // input2
                faultGate->fanin2 = stuckatCons;
                faultGate->in2Name = stuckatCons->outName;
              }
            }
          } else {
            faultGate->gateType = bufInv;
            faultGate->fanin1 = stuckatCons;
            faultGate->in1Name = stuckatCons->outName;
          }
        }
        return 1;
      }

      int resetFaultsInCircuit() {
        if (theCircuit.size() == 0) return 0;
        if (theCircuit.size() == PISize + POSize + gateSize) return 0; // no fault is added to the circuit
        for (map<int, gate*>::iterator iter = preGateInTheCircuit.begin(); iter != preGateInTheCircuit.end(); iter++) {
          int gateID = iter->first;
          int outValue = theCircuit[gateID]->outValue;
          gate *preGate = iter->second;
          preGate->copyGate(theCircuit[gateID]); // reset the gate
          theCircuit[gateID]->outValue = outValue;  // the outvalue will kept(will be used to check propagation path latter)
          theCircuit.pop_back(); // delete the stuckat constant wire
        }
        preGateInTheCircuit.clear();
        return 1;
      }

      int assignPIs(vector<int> &inValues){
        if (inValues.size() != PISize){
          cout << "\n***Input vector does not match the size of PI***\n" << endl;
          return 0;
        }
        for (int i = 0;i < inValues.size(); i++){
            theCircuit[i]->setPI(inValues[i]);
        }
        return 1;
      }

      //----------------------------------------
      // if PI is first propagate in faulty circuit then propagate in correct circuit
      // if the new out value is different from the previous one, set it as the visited
      // whether it's really faulty gates need to check later
      void propagatePI(){
        for(int i = 0; i < theCircuit.size(); i++){
          int preValue = theCircuit[i]->outValue;
          theCircuit[i]->setOut();
          int curValue = theCircuit[i]->outValue;
          theCircuit[i]->visited = (preValue != curValue);
        }
      }

      // try to find all path of all faults
      void findSSAFPath(vector<int> &newFaults) {
        for (auto faultID : newFaults) {
          int gateID = (faultID >> 3);
          if (theCircuit[gateID]->visited == true) {
            findSSAFPathDFS(theCircuit[gateID]);
          }
        }
      }
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
          if (fanout->visited == true && findSSAFPathDFS(fanout) == true) {
            curGate->faultyOut = true;
            result = true;
          }
        }
        return result;
      }

      // given the faultID and test vector, mark the gate in the propgation path as "faultyOut = true"
      // first propagate PI in faulty circuit, then do the samt thing in original circuit.
      // so the outValue remains in the cirucit is the value to activate and propagte faults
      // return 1 if faults can be tested by the test pattern, else return 0;
      int propagateFault(vector<int> &newFaults, vector<int> &testVector) {
        resetAllVisitedFaultyOut();
        resetFaultsInCircuit();
        // inject faults and propagate the value
        injectFaultsInCircuit(newFaults);
        assignPIs(testVector);
        propagatePI();
        //cout << "faulty propagation"<< endl;
        //printCircuit(theCircuit); //*****
        resetAllVisitedFaultyOut();
        resetFaultsInCircuit();
        // progate the value in original circuit
        assignPIs(testVector);
        propagatePI();
        //cout << "original propagation"<< endl;
        //printCircuit(theCircuit); //*****
        findSSAFPath(newFaults);
        //cout << "find path"<< endl;
        //printCircuit(theCircuit); //*****
        for (int i = 0; i < POSize; i++) {
          gate *POGate = theCircuit[PISize + gateSize + i];
          // if the fautls can be detected in one of the PO, then it's detected.
          if (POGate->faultyOut == true) {
            return 1;
          }
        }
        return 0;
      }
      //----------------------------------------

      int getFaultID(int gateID, int port, int stuckat) {
        return (gateID << 3) + (port << 1) + stuckat;
      }

      void resetAllVisitedFaultyOut() {
        for (int i = 0; i < theCircuit.size(); i++) {
          theCircuit[i]->visited = false;
          theCircuit[i]->faultyOut = false;
        }
      }

      // read the SSAF test pattern files
      int patternParser(char *patternFile) {
        ifstream file;
        file.open(patternFile);
        if(!file){
          cout << "Cannot open the file" << endl;
          return 0;
        }
        string line;
        int count = 0;
        while(!file.eof()) {
          count++;
          getline(file, line);
          if (line.size() == 0 || line.find("#") != string::npos) continue;
          if (line.size() != PISize) {
            cout << "No " << count <<  " pattern's size doesn't match PI size: "<< endl;
            cout << "Input pattern size: " << line.size() << ", PI size: " << PISize << endl;
            continue;
          }
          vector<int> pattern;
          for (int i = 0; i < line.size(); i++) {
            pattern.push_back((line[i] == '1'));
          }
          SSAFPatterns.push_back(pattern);
        }
        file.close();
        return 1;
      }


      // check whether the given test patterns can cover entire faultList or not.
      //
      // get: map<set<int>, vector<int>> faultToPatterns;   set<int> redundantSSAF;
      void CheckGivenPatterns(set<int> &faultList, vector<vector<int>> &SSAFPatterns) {
        set<int> visited;
        for (auto testVector : SSAFPatterns) {
          if (visited.size() == faultList.size()) break; // all faults are checked
          set<int> faultsSameVector;
          for (auto faultID : faultList) {
            if (visited.find(faultID) != visited.end()) continue;  // already checked
            vector<int> newFaults;
            newFaults.push_back(faultID);
            if (propagateFault(newFaults, testVector) == 1) {
              faultsSameVector.insert(faultID);
              visited.insert(faultID);
              int gateID = (faultID >> 3);
              gate *faultGate = theCircuit[gateID];
              findFaultsSameTest_helper(faultGate, faultsSameVector, faultList, faultID);
              visited.insert(faultsSameVector.begin(), faultsSameVector.end());
            }
          }
          faultToPatterns.insert(pair<set<int>, vector<int>>(faultsSameVector, testVector));
        }

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
            //****************
          /*  if (faultID == 1252) {
              cout << "\n\n\n\n\nwow here!!!!!!!!!!!!!! " << iniFaultID << "\n\n\n\n\n" << endl;
            }*/
            //**************
          }
          // input2
          port = 2;
          activate = curGate->fanin2->outValue;
          stuckat = 1 - activate;
          faultID = getFaultID(gateID, port, stuckat);
          // check the side value
          if (curGate->fanin1->outValue == curGate->invIn1 && faultList.find(faultID) != faultList.end()) {
            faultsSameVector.insert(faultID);
            //****************
            /*if (faultID == 1252) { // 997
              cout << "\n\n\n\n\nwow here!!!!!!!!!!!!!! " << iniFaultID << "\n\n\n\n\n" << endl;
            }*/
            //**************
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

      void TestSSAFPatterns(set<int> faultList) {
        set<int> visited;
        for (auto testVector : SSAFPatterns) {
          set<int> sameVector;
          for (auto faultID : faultList) {
            vector<int> newFaults;
            newFaults.push_back(faultID);
            if (propagateFault(newFaults, testVector) == 1) {
              sameVector.insert(faultID);
              visited.insert(faultID);
            }
          }
          faultToPatterns.insert(pair<set<int>, vector<int>>(sameVector, testVector));
        }

        for (auto faultID : faultList) {
          if (visited.find(faultID) == visited.end()) {
            vector<int> newFaults;
            newFaults.push_back(faultID);
            set<int> sameVector;
            sameVector.insert(faultID);
            vector<int> testVector;
            if (generateTestBySAT(newFaults, testVector)) {
              faultToPatterns.insert(pair<set<int>, vector<int>>(sameVector, testVector));
              cout << "\n\n\n\n11111111111111111\n\n\n\n"<< endl;
            } else {
              redundantSSAF.insert(faultID);
              visited.insert(faultID);
            }
          }
        }
        if (visited.size() == faultList.size()) {
          cout << "Test patterns are sufficient" << endl;
          cout << "redundantSSAF.size(): " << redundantSSAF.size() << endl;
        } else {
          cout << "Test patterns are insufficient" << endl;
          cout << "visited.size(): " << visited.size() << endl;
          cout << "redundantSSAF.size(): " << redundantSSAF.size() << endl;
          cout << "collapsedFaultList.size(): " << collapsedFaultList.size() << endl;
        }

      }

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
        printFaults(redundant);
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

      // -----use the check the result---------
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
          // constant, bufInv, aig, PO, PI, OR, XOR
          cout << " "; printGateType(curCircuit[i]->gateType);
          /*
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
          */
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

      int copyCount;
      circuit *ClassCircuit;
      vector <gate*> theCircuit;
      int PISize, POSize, gateSize;
      map<int, gate*>preGateInTheCircuit;
      // ----Generate the CNF to do SAT------
      vector<gate*> oriAndFauCir;
      vector<vector<vector<int>>> CNFOriAndFauCir;
      // the gate changed due to the fault.
      // key : gateID in "oriAndFauCir", value is the gate in oriAndFauCir
      map<int, gate*> preGateInOriAndFauCir;
      //-------------------------------------
      set<int> collapsedFaultList;
      set<int> allFaultList;
      set<int> notIncollapsedFaultList;
      set<int> redundantSSAF;
      vector<vector<int>> SSAFPatterns;
      // key: faults, value: corresponding test vector
      map<set<int>, vector<int>> faultToPatterns;
  };
}

#endif
