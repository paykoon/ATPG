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
      atpg(circuit *pCircuit) {
        copyCount = 1;
        this->PISize = pCircuit->PISize;
        this->POSize = pCircuit->POSize;
        this->gateSize = pCircuit->gateSize;
        this->theCircuit = pCircuit->theCircuit;
        ATPGInit();
      }

      ~atpg() {
        delete this;
      }

      void ATPGInit() {
        double startTime, endTime, preTime, curTime;
        startTime = clock();
        preTime = clock();
        if ( !generateFaultList() ) return;
        curTime = clock();
        cout << endl << "----------Initialization of ATPG----------" << endl;
        cout << "1. Fault list generating is completed. Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        cout << "   Fault number is " << collapsedFaultList.size() << " (Collapsed: AIG's input wire connected to PI or fanout)" << endl;
        cout << "   Total faults number: " << allFaultList.size() << ", compression ratio " << (double)(collapsedFaultList.size()) / (double)(allFaultList.size())<< endl;
        preTime = clock();
        generateOriAndFau(oriAndFauCir);
        generateCNF(oriAndFauCir);
        curTime = clock();
        cout << "2. Initial CNF generating is completed. Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        /*
        preTime = clock();
        // TO DO.........
        curTime = clock();
        cout << "3. Test vectors of SSAF(collapsed) are prepared (****TO DO****). Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;

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
        for (map<int, gate*>::iterator iter = preGateInTheCircuit.begin(); iter != preGateInTheCircuit.end(); iter++) {
          int gateID = iter->first;
          gate *preGate = iter->second;
          preGate->copyGate(theCircuit[gateID]); // reset the gate
          theCircuit.pop_back(); // delete the stuckat constant wire
        }
        preGateInTheCircuit.clear();
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
          newInput->fanout.push_back(faultyInput);
          newInput->fanout.push_back(originalInput);
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
          // fanin1 for original output, fanin2 for faulty output.
          string oriOutName = theCircuit[PISize+gateSize+i]->outName;
          string in1Name = "originalOut_"+to_string(i), in2Name = "faultyOut_"+to_string(i), outName = "XOR_"+oriOutName;
          string invIn1 = "1", invIn2 = "1", invOut = "1";
          gate *newXOR = new gate(XOR, in1Name, in2Name, outName, invIn1, invIn2, invOut);
          oriAndFauCir.push_back(newXOR);
          gate *originalOutput = oriAndFauCir[PISize+gateSize+i];
          gate *faultyOutput = oriAndFauCir[origialSize+PISize+gateSize+i];
          originalOutput->fanout.push_back(newXOR);
          originalOutput->gateType = bufInv;
          faultyOutput->fanout.push_back(newXOR);
          faultyOutput->gateType = bufInv;
          newXOR->fanin1 = originalOutput;
          newXOR->fanin2 = faultyOutput;
          //connect all XOR with output wires
          string name = "newOut_"+oriOutName;
          gate *newOutput = new gate(PO, name);
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
        // push_back a blank gate to make the "CNFOriAndFauCir" and "oriAndFauCir" same.
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
        // reset the circuit "oriAndFauCir"
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
        }
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
          printCircuit(oriAndFauCir);
          printCNF(CNFOriAndFauCir);
          cout << endl;
        }
      }

      void testFaultInjectInCircuit() {
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
      }

      void printCircuit(vector <gate*> &curCircuit) {
        for (int i = 0; i < curCircuit.size(); i++) {
          cout << i << " " << curCircuit[i]->in1Name << " ";
          if (curCircuit[i]->gateType == aig || curCircuit[i]->gateType == OR || curCircuit[i]->gateType == XOR) {
            cout << curCircuit[i]->in2Name << " ";
          }
          cout << curCircuit[i]->outName << " ";
          // constant, bufInv, aig, PO, PI, OR, XOR
          printGateType(curCircuit[i]->gateType);
          cout << endl;
        }
        cout << endl;
      }

      void printCNF(vector<vector<vector<int>>> &CNFFormula) {
        for (int m = 0; m < CNFFormula.size(); m++) {  // gate
          cout << m << ": ";
          for (int i = 0; i < CNFFormula[m].size(); i++) {//clauses
            for (int j = 0; j < CNFFormula[m][i].size(); j++) {//literals
                if (CNFFormula[m][i][j] > 0) {
                    cout << CNFFormula[m][i][j] - 1 << " ";
                } else {
                    cout << "-" << (-1)*CNFFormula[m][i][j] - 1 << " ";
                }
            }
            cout << ", ";
          }
          cout << endl;
        }
      }
      // --------------------------------------

      int copyCount;
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
  };
}

#endif
