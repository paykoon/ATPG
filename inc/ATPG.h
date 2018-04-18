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
#include "CNFGeneration.h"
using namespace std;
using namespace Gate;
using namespace Circuit;
using namespace Glucose;
using namespace CNFGeneration;

namespace ATPG{
  class atpg{
    public:
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
        cout << "   Total faults number: " << allFaultList.size() << ", " << (double)(collapsedFaultList.size()) / (double)(allFaultList.size()) << " fault are selected by the fault model" << endl;

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
        cout << "   Build the connection between SSAF and the initial test set." << endl;
        cout << "   (We just assume that the initial test patterns can already cover all SSAF)" << endl;
        CheckGivenPatterns(collapsedFaultList, SSAFPatterns);
        cout << "   " <<  faultToPatterns.size() << "/"  << SSAFPatterns.size() << " test patterns are used."<< endl;
        cout << "   Number of redundant faults(among our SSAF model): " << redundantSSAF.size() << endl;

        endTime = clock();
        cout << "----------The initialization of the ATPG takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------" << endl << endl;
        return 1;
      }

      int getFaultID(int gateID, int port, int stuckat) {
        return (gateID << 3) + (port << 1) + stuckat;
      }

      void resetAllVisitedFaultyOut() {
        for (int i = 0; i < theCircuit.size(); i++) {
          theCircuit[i]->visited = false;
          theCircuit[i]->faultyOut = false;
        }
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
        // get the faults that are not selected by the fault model
        for (set<int>::iterator allIter = allFaultList.begin(); allIter != allFaultList.end(); allIter++) {
          set<int>::iterator collIter = collapsedFaultList.find(*allIter);
          if (collIter == collapsedFaultList.end()) {
            notIncollapsedFaultList.insert(*allIter);
          }
        }
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

      //----------------------SAT Process----------------------
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
        cnfgeneration *clauseGene = new cnfgeneration();
        for (int i = 0; i < curCircuit.size(); i++) {
          gate *curGate = curCircuit[i];
          clauseGene->generateClause(gateClause, curGate);
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
        cnfgeneration *clauseGene = new cnfgeneration();
        // reset the circuit "oriAndFauCir" and "CNFOriAndFauCir"
        for (map<int, gate*>::iterator iter = preGateInOriAndFauCir.begin(); iter != preGateInOriAndFauCir.end(); iter++) {
          // bit index:     [ ,3]        [2  1]    0
          //                gateID       port     stuckat
          // port: 01 input1, 10 input2, 11 output
          int fauGateID = iter->first;
          gate *preFauGate = iter->second;
          preFauGate->copyGate(oriAndFauCir[fauGateID]);
          clauseGene->generateClause(gateClause, preFauGate);
          // preFauGate->generateClause(gateClause);
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
          // stuckatCons->generateClause(gateClause);
          clauseGene->generateClause(gateClause, stuckatCons);
          CNFOriAndFauCir.push_back(gateClause);
          gateClause.clear();
          // faultGate->generateClause(gateClause);
          clauseGene->generateClause(gateClause, faultGate);
          CNFOriAndFauCir[fauGateID].clear();
          CNFOriAndFauCir[fauGateID].assign(gateClause.begin(), gateClause.end());
          gateClause.clear();
        }
        return 1;
      }

      // return 1 if SAT. else UNSAT, which means redundant
      int generateTestBySAT(vector<int> &newFaults, vector<int> &testVector) {
        glucose *SATSolver = new glucose();
        injectFaultsInCNF(newFaults);
        if (SATSolver->SATCircuit(CNFOriAndFauCir, testVector, theCircuit.size(), PISize) == 1) {
          return 1;
        } else {
          return 0;
        }
      }
      //----------------------SAT Process---------------------------


      //------------------Find the propagation path of the faults under the given test pattern------------------
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

        resetAllVisitedFaultyOut();
        resetFaultsInCircuit();
        // progate the value in original circuit
        assignPIs(testVector);
        propagatePI();
        findSSAFPath(newFaults);
        for (int i = 0; i < POSize; i++) {
          gate *POGate = theCircuit[PISize + gateSize + i];
          // if the fautls can be detected in one of the PO, then it's detected.
          if (POGate->faultyOut == true) {
            return 1;
          }
        }
        return 0;
      }
      //------------------Find the propagation path of the faults under the given test pattern------------------


      //------------------Process about the initial SSAF test pattern------------------
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
      // assume that the initial test pattern can already detect all SSAF
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
            }
          }
          faultToPatterns.insert(pair<set<int>, vector<int>>(faultsSameVector, testVector));
        }
        for (auto faultID : faultList) {
          if (visited.find(faultID) == visited.end()) {
            redundantSSAF.insert(faultID);
          }
        }
      }
      //------------------Process about the initial SSAF test pattern------------------

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
