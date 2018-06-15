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
using namespace CNFGeneration;

namespace ATPG{
  class atpg{
    public:
      int copyCount;
      vector <gate*> theCircuit;
      int PISize, POSize, gateSize;
      // the circuit will be changed if faults are added to circuit.
      // the original unchanged gate will be stored here.
      map<int, gate*>preGateInTheCircuit;
      int faultsInsertedtheCircuit;
      // ----Generate the CNF to do SAT------
      vector<gate*> oriAndFauCir;
      vector<vector<vector<int>>> CNFOriAndFauCir;
      // the gate changed due to the fault.
      // key : gateID in "oriAndFauCir", value is the gate in oriAndFauCir
      map<int, gate*> preGateInOriAndFauCir;
      int faultsInsertedoriAndFauCir;
      //-------fault list and test pattern----------------
      set<int> collapsedSSAFList;
      set<int> allSSAFList;
      set<int> notIncollapsedSSAFList;
      set<int> redundantSSAF;
      vector<vector<int>> SSAFPatterns;
      // key: faults, value: corresponding test vector
      map<int, vector<int>> SSAFToPatterns;
      // ---------------------------------
      // key: fault. value: its blocked faults. minus means the redundant fault
      map<int, set<int>> potentiallyUndetected;
      set<set<int>> undetectedDSA;


      atpg(circuit *pCircuit, char *patternFile) {
        copyCount = 1;
        this->PISize = pCircuit->PISize;
        this->POSize = pCircuit->POSize;
        this->gateSize = pCircuit->gateSize;
        this->theCircuit = pCircuit->theCircuit;
        this->faultsInsertedtheCircuit = 0;
        this->faultsInsertedoriAndFauCir = 0;
        ATPGInit(patternFile);
      }

      ~atpg() {
        delete this;
      }

      // if initialization has problem, return 0
      // else return 1;
      int ATPGInit(char *patternFile) {
        double startTime, endTime, preTime, curTime;

        cout << "\n\n----------Initialization of ATPG----------" << endl;
        startTime = clock();
        cout << "1. SSA Fault list generation is started. " << endl;
        if ( !generateFaultList() ) return 0;
        preTime = clock();
        cout << "   SSA Fault number is " << collapsedSSAFList.size() << " (Collapsed: AIG's input wire connected to PI or fanout)" << endl;
        cout << "   Total SSA faults number: " << allSSAFList.size() << ", " << 100*(double)(collapsedSSAFList.size()) / (double)(allSSAFList.size()) << "% fault are selected by the fault model" << endl;
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;


        cout << "2. Initial CNF generation is started." << endl;
        preTime = clock();
        generateOriAndFau(oriAndFauCir);
        generateCNF(oriAndFauCir);
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;


        cout << "3. Read initial SSAF test patterns." << endl;
        preTime = clock();
        if(patternParser(patternFile) == 1) {
          cout << "   " << SSAFPatterns.size() << " SSAF patterns are read" << endl;
        } else {
          cout << "   Pattern file has problem"<< endl;
          return 0;
        }
        cout << "   Build the connection between SSAF and the initial test set." << endl;
        cout << "   (We just assume that the initial test patterns can cover all SSAF)" << endl;
        PairSSAPatternWithSSAF(allSSAFList, redundantSSAF, SSAFPatterns);
        cout << "   Number of redundant faults(among all faults): " << redundantSSAF.size() << endl;
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        endTime = clock();
        cout << "----------The initialization of the ATPG takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------\n\n" << endl;


        cout << "\n\n----------DSA Faults checking----------" << endl;
        startTime = clock();
        cout << "1. Search the potential undetected DSA Faults." << endl;
        preTime = clock();
        findPotentialUndetectedDSAF(SSAFToPatterns, redundantSSAF, theCircuit, collapsedSSAFList, potentiallyUndetected);
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        /*
        cout << "**************check here**************" << endl;
        vector<int> newFaults; newFaults.push_back(854);set<int> empty;
        checkFaultAndTestVector(newFaults, SSAFToPatterns[843], empty, empty, empty, 0);]
        */
        //***********************
        /*
        cout << "\n\n\n************currently checking**************" << endl;
        printFault2(843);
        printFault2(850);
        printFault2(852);
        printFault2(854);
        printFault2(860);
        printFault2(868);
        cout << "************currently checking**************\n\n\n" << endl;
        */
        //vector<int> newFaults; newFaults.push_back(843); newFaults.push_back(854);
        //set<int> empty;
        //cout << "detected??"<< endl;
        //cout << checkFaultAndTestVector(newFaults, SSAFToPatterns[843], empty, empty, empty, 0) << endl;
        //************************
        // *****************************************
        /*
        vector<int>newFaults;
        set<int> connectedGates;
        findConnectedGatesDFS(theCircuit[604 >> 3], connectedGates);
        vector<gate*> connectedGatesVec;
        for (auto id : connectedGates) {
          connectedGatesVec.push_back(theCircuit[id]);
        }
        */
        /*
        newFaults.push_back(604); newFaults.push_back(503);

        resetAllVisitedisPath();
        resetFaultsInCircuit();
        // inject faults and propagate the value
        injectFaultsInCircuit(newFaults);
        assignPIs(SSAFToPatterns[604]);
        propagatePI();
        printCircuit(connectedGatesVec);


        cout << "***********Comparison********* " << "3282" << endl;

        resetAllVisitedisPath();
        resetFaultsInCircuit();
        assignPIs(SSAFToPatterns[604]);
        propagatePI();
        printCircuit(connectedGatesVec);
        */
        // ****************************************


        cout << "2. Compress undetected DSA Faults." << endl;
        preTime = clock();
        pairUndetectedDSAF(potentiallyUndetected, undetectedDSA, redundantSSAF);
        cout << "   Potential Undetected DSA number (without compressing): " << undetectedDSA.size() << endl;
        int preSize = undetectedDSA.size();
        compressUndetectedDSAF(undetectedDSA);
        cout << "   Undetected DSA number (after compressing): " << undetectedDSA.size()  << " Compressing ratio: " << (double)undetectedDSA.size()/preSize << endl;
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        endTime = clock();
        cout << "----------DSA Faults checking takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------\n\n" << endl;

        // printUndetectedDSA(undetectedDSA);


        // vector<vector<int>> allDoubleFaults_collapsed;
        // generateAllNSA(collapsedSSAFList, allDoubleFaults_collapsed, 2, 0);
        // vector<vector<int>> allDoubleFaults;
        // generateAllNSA(allSSAFList, allDoubleFaults, 2, 0);
        /*
        cout << "\n\n\nCircuit Simulate\n" << endl;
        set<set<int>> undetectedDSAbySSAPT_Simulte;
        testDSA(allSSAFList, SSAFPatterns, undetectedDSAbySSAPT_Simulte);
        // getIgnoredNFaultsCircuitSimulate(allDoubleFaults, SSAFPatterns, undetectedDSAbySSAPT_Simulte);
        //cout << "\n\n\nSAT\n" << endl;
        //set<set<int>> undetectedDSAbySSAPT_SAT;
        //getIgnoredNFaultsSAT(allDoubleFaults_collapsed, SSAFPatterns, undetectedDSAbySSAPT_SAT);

        cout << "\n\n\n******Analyzation(not picked by out method)********" << endl;
        analyzeIgnoredUndetected(undetectedDSAbySSAPT_Simulte, undetectedDSA);
        */

        /*
        set<int> connectedGates;
        findConnectedGatesDFS(theCircuit[41], connectedGates);
        vector <gate*> partOfCircuit;
        for (int id : connectedGates) {
          partOfCircuit.push_back(theCircuit[id]);
        }
        printCircuit(partOfCircuit);
        printCircuitBlif(partOfCircuit);
        printForTestbench(partOfCircuit, 5);
        */

        return 1;
      }

      int getFaultID(int gateID, int port, int stuckat) {
        return (gateID << 3) + (port << 1) + stuckat;
      }

      int faultIDToGateID(int faultID) {
        return (faultID >> 3);
      }

      void resetAllVisitedisPath() {
        for (int i = 0; i < theCircuit.size(); i++) {
          theCircuit[i]->visited = false;
          theCircuit[i]->different = false;
          theCircuit[i]->isPath = false;
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
            collapsedSSAFList.insert(newFault);
            newFault = (i << 3) + (1 << 1) + 1;   //stuck-at 1 at input 1 of gate i
            collapsedSSAFList.insert(newFault);
          }
          if(theCircuit[i]->fanin2->gateType == PI || theCircuit[i]->fanin2->fanout.size() > 1){
            newFault = (i << 3) + (2 << 1) + 0;   //stuck-at 0 at input 2 of gate i
            collapsedSSAFList.insert(newFault);
            newFault = (i << 3) + (2 << 1) + 1;   //stuck-at 1 at input 2 of gate i
            collapsedSSAFList.insert(newFault);
          }
        }
        // generate all faults
        for (int i = 0; i < theCircuit.size(); i++) {
          // all faults(in input)
          newFault = (i << 3) + (1 << 1) + 0;   //stuck-at 0 at input 1 of gate i
          allSSAFList.insert(newFault);
          newFault = (i << 3) + (1 << 1)+ 1;   //stuck-at 1 at input 1 of gate i
          allSSAFList.insert(newFault);
          newFault = (i << 3) + (3 << 1) + 0;   //stuck-at 0 at output of gate i
          allSSAFList.insert(newFault);
          newFault = (i << 3) + (3 << 1) + 1;   //stuck-at 1 at output of gate i
          allSSAFList.insert(newFault);
          Type gateType = theCircuit[i]->gateType;
          if (gateType == XOR || gateType == OR || gateType == aig) {
            newFault = (i << 3) + (2 << 1) + 0;   //stuck-at 0 at input 2 of gate i
            allSSAFList.insert(newFault);
            newFault = (i << 3) + (2 << 1)+ 1;   //stuck-at 1 at input 2 of gate i
            allSSAFList.insert(newFault);
          }
        }
        // get the faults that are not selected by the fault model
        for (set<int>::iterator allIter = allSSAFList.begin(); allIter != allSSAFList.end(); allIter++) {
          set<int>::iterator collIter = collapsedSSAFList.find(*allIter);
          if (collIter == collapsedSSAFList.end()) {
            notIncollapsedSSAFList.insert(*allIter);
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

      // if PI is first propagated in faulty circuit then propagate in correct circuit
      // if the new out value is different from the previous one, set it as the visited
      // whether it's really faulty gates need to check later
      void propagatePI(){
        for(int i = 0; i < theCircuit.size(); i++){
          int preValue = theCircuit[i]->outValue;
          theCircuit[i]->setOut();
          int curValue = theCircuit[i]->outValue;
          theCircuit[i]->different = (preValue != curValue);
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
          gateClause.clear();
        }
        preGateInOriAndFauCir.clear();
        while (faultsInsertedoriAndFauCir > 0) {
          faultsInsertedoriAndFauCir--;
          CNFOriAndFauCir.pop_back();  // delete the constant wire which put in last
        }
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
          faultsInsertedoriAndFauCir++;
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
            faultGate->invIn1 = 1;
            faultGate->invOut = 1;
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


      //----------------inject and reset faults into original circuit--------------------
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
          faultsInsertedtheCircuit++;
          string name = "stuck-at"+to_string(stuckat);
          string stuckatStr = to_string(stuckat);
          gate *stuckatCons = new gate(name, stuckatStr);
          stuckatCons->gateID = theCircuit.size() + i;
          theCircuit.push_back(stuckatCons);

          if (port != 3) {  // gate input.
            if (faultGate->gateType == constant) {
              faultGate->outValue = stuckat;
            } else { // AIG or PI.
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
          } else { // gate output.
            faultGate->gateType = bufInv;
            faultGate->fanin1 = stuckatCons;
            faultGate->in1Name = stuckatCons->outName;
            faultGate->invIn1 = 1;
            faultGate->invOut = 1;
          }
        }
        return 1;
      }

      int resetFaultsInCircuit() {
        if (theCircuit.size() == 0) return 0;
        //if (theCircuit.size() == PISize + POSize + gateSize) return 0; // no fault is added to the circuit
        for (map<int, gate*>::iterator iter = preGateInTheCircuit.begin(); iter != preGateInTheCircuit.end(); iter++) {
          int gateID = iter->first;
          int outValue = theCircuit[gateID]->outValue;
          gate *preGate = iter->second;
          preGate->copyGate(theCircuit[gateID]); // reset the gate
          theCircuit[gateID]->outValue = outValue;  // the outvalue will kept(will be used to check propagation path latter)
        }
        // delete the stuckat constant wire
        while (faultsInsertedtheCircuit > 0) {
          faultsInsertedtheCircuit--;
          theCircuit.pop_back();
        }
        preGateInTheCircuit.clear();
        return 1;
      }
      //----------------inject and reset faults into original circuit--------------------


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
      bool findBlockSSADFS(gate *curGate, gate *preGate, set<int> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAFList, int oriFault) {
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

      // given the faultID and test vector, mark the gate in the propgation path as "isPath = true"
      // first propagate PI in faulty circuit, then do the same thing in original circuit.
      // so the outValue remains in the cirucit is the value to activate and propagte faults
      // also find the potentiallyUndetected.
      // return 1, if faults can be tested by the test pattern, else return 0.
      // mode = 0, if dont want to find the block faults, then set blockFaultsList, faultList and redundantSSAF to empty.
      // mode = 1. also find the block fautls(***currently only for DSA)
      int checkFaultAndTestVector(vector<int> &newFaults, vector<int> &testVector, set<int> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAF, int mode) {
        resetAllVisitedisPath();
        resetFaultsInCircuit();
        // inject faults and propagate the value
        injectFaultsInCircuit(newFaults);
        assignPIs(testVector);
        propagatePI();

        resetAllVisitedisPath();
        resetFaultsInCircuit();
        // progate the value in original circuit
        assignPIs(testVector);
        propagatePI();

        // ****only for DSA fault now*******
        int gateID = newFaults[0] >> 3;
        int port = (newFaults[0] >> 1) & 3;
        for (int i = 0; i < POSize; i++) {
          gate *POGate = theCircuit[PISize + gateSize + i];
          // if the fautls can be detected in one of the PO, then it's detected.
          if (POGate->different == true) {
            // find the SSA that may block it in its propagatioin path.
            gate *curGate = theCircuit[gateID];
            // ***only work for current fault model.
            if (mode == 1) {
              gate *preGate = (port == 1) ? curGate->fanin1 : curGate->fanin2;
              findBlockSSADFS(theCircuit[gateID], preGate, blockFaultsList, faultList, redundantSSAF, newFaults[0]);
            }
            return 1;
          }
        }
        return 0;
      }

      // find the potentiallyUndetected.
      // assume that the initial test pattern can already detect all SSAF.
      // get: map<int, set<int>> &potentiallyUndetected;
      void findPotentialUndetectedDSAF(map<int, vector<int>> &SSAFToPatterns, set<int> &redundantSSAF, vector<gate*> &theCircuit, set<int> &faultList, map<int, set<int>> &potentiallyUndetected) {
        for (auto iter : SSAFToPatterns) {
          int faultID = iter.first;
          vector<int> newFaults;
          newFaults.push_back(faultID);
          set<int> blockFaultsList;
          vector<int> testVector = iter.second;
          checkFaultAndTestVector(newFaults, testVector, blockFaultsList, faultList, redundantSSAF, 1);
          if (blockFaultsList.size() > 0) {
             potentiallyUndetected.insert(make_pair(faultID, blockFaultsList));
          }
        }
      }
      //------------------Find the propagation path of the faults under the given test pattern and the potential undetected faults -------------------

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
      // get: vector<set<int>, vector<int>> SSAFToPatterns;   set<int> redundantSSAF;
      void PairSSAPatternWithSSAF(set<int> &faultList, set<int> &redundantSSAF, vector<vector<int>> &SSAFPatterns) {
        set<int> checked;
        for (auto testVector : SSAFPatterns) {
          if (checked.size() == faultList.size()) break; // all faults are checked
          for (auto faultID : faultList) {
            if (checked.find(faultID) != checked.end()) continue;  // already checked
            vector<int> newFaults;
            newFaults.push_back(faultID);
            set<int> empty;
            if (checkFaultAndTestVector(newFaults, testVector, empty, empty, empty, 0) == 1) {
              SSAFToPatterns.insert(make_pair(faultID, testVector));
              checked.insert(faultID);
            }
          }
        }
        // the redundant fault among our fault model
        for (auto faultID : faultList) {
          if (checked.find(faultID) == checked.end()) {
            redundantSSAF.insert(faultID);
          }
        }
      }
      //------------------Process about the initial SSAF test pattern------------------

      // -----find all gates connected to curGate-----
      // first find the PO then backtrace to PI
      void findConnectedGatesDFS(gate *curGate, set<int> &connectedGates) {
        if (curGate->gateType == PO) {
          findConnectedGates_helperDFS(curGate, connectedGates);
          return;
        }
        for (auto nextGate: curGate->fanout) {
          findConnectedGatesDFS(nextGate, connectedGates);
        }
      }
      void findConnectedGates_helperDFS(gate *curGate, set<int> &connectedGates) {
        connectedGates.insert(curGate->gateID);
        if (curGate->gateType == PI || curGate->gateType == constant) {
          return;
        }
        findConnectedGates_helperDFS(curGate->fanin1, connectedGates);
        if (curGate->gateType == aig) findConnectedGates_helperDFS(curGate->fanin2, connectedGates);
      }
      // --------------------------------------------

      // -------get the undetected DSA given the potentially undetected list----------
      // note: redundant will be represented as minus number;
      void pairUndetectedDSAF(map<int, set<int>> &potentiallyUndetected, set<set<int>> &undetectedDSA, set<int> &redundantSSAF) {
        for (map<int, set<int>>::iterator iter = potentiallyUndetected.begin(); iter != potentiallyUndetected.end(); iter++) {
          int first = iter->first;
          set<int> firstSet = iter->second;
          for (auto second : firstSet) {
            if (potentiallyUndetected.find(second) != potentiallyUndetected.end() || redundantSSAF.find(second) != redundantSSAF.end()) {
              set<int> secondSet = potentiallyUndetected[second];
              if (secondSet.find(first) != secondSet.end() || redundantSSAF.find(second) != redundantSSAF.end()) {
                set<int> temp;
                temp.insert(first);
                temp.insert(second);
                undetectedDSA.insert(temp);
              }
            }
          }
        }
        // add the double faults that are both redundant to the list.
        vector<vector<int>> redundantSSAFComb;
        generateAllNSA(redundantSSAF, redundantSSAFComb, 2, 0);
        for (auto DoubleRedundantSSAF : redundantSSAFComb) {
          set<int> DSASet;
          for (auto faultID : DoubleRedundantSSAF) {
            DSASet.insert(faultID);
          }
          undetectedDSA.insert(DSASet);
        }
      }

      void compressUndetectedDSAF(set<set<int>> &undetectedDSA) {
        set<set<int>> finalUndetected;
        for (set<set<int>>::iterator iter = undetectedDSA.begin(); iter != undetectedDSA.end(); iter++) {
          set<int> DSA = *iter;
          vector<int> newFaults;
          for (auto faultID : DSA) {
            newFaults.push_back(faultID);
          }
          set<int> empty;
          bool isDetected = false;
          for (auto testVector: SSAFPatterns) {
            if (checkFaultAndTestVector(newFaults, testVector, empty, empty, empty, 0) == 1) {
              isDetected = true;
              break;
            }
          }
          if (isDetected == false) {
            set<int>temp;
            temp.insert(DSA.begin(), DSA.end());
            finalUndetected.insert(temp);
          }
        }
        undetectedDSA.clear();
        undetectedDSA.insert(finalUndetected.begin(), finalUndetected.end());
      }
      // -----------------------------------------------------------------------------

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

      // get the N faults that cannot be detected by given test.
      // check by fault simulation.
      void getIgnoredNFaultsCircuitSimulate(vector<vector<int>> &allNFaults, vector<vector<int>> &testVectors, set<set<int>> &undetectedDSAbySSAPT) {
        set<int> empty;
        // *************************
        //cout << "*********before checking***********" << endl;
        // **************************
        for (auto curFault : allNFaults) {
          bool successFlag = false;
          for (auto testVector : testVectors) {
            if (checkFaultAndTestVector(curFault, testVector, empty, empty, empty, 0) == 1) {
              successFlag = true;
              break;
            }
          }
          if (successFlag == false) {
            set<int> temp;
            for (auto fault : curFault) temp.insert(fault);
            undetectedDSAbySSAPT.insert(temp);
          }
        }
        // *************************
        //cout << "*********after checking***********" << endl;
        // **************************
        cout << "\n\n\n\n\nthe ignoredFaults by the given SSAF test(check by circuitSimulation)" << endl;
        for (auto faultSet : undetectedDSAbySSAPT) {
          for (auto fault : faultSet) {
            cout << fault;
            if (redundantSSAF.find(fault) != redundantSSAF.end()) {
              cout << "(RSSAF)";
            }
            cout << " ";
          }
          vector<int> pattern;
          vector<int> inputFault;
          inputFault.assign(faultSet.begin(), faultSet.end());
          //if (generateTestBySAT(inputFault, pattern) == 0) {
          //  cout << " ***R N F***";
          //}
          cout << endl;
        }
        cout << "Undetected DSA number(by circuitSimulation): " << undetectedDSAbySSAPT.size() << endl;
      }

      void getIgnoredNFaultsSAT(vector<vector<int>> &allNFaults, vector<vector<int>> &testVectors, set<set<int>> &undetectedDSAbySSAPT) {
        set<int> empty;
        for (auto curFault : allNFaults) {
          bool successFlag = false;
          for (auto testVector : testVectors) {
            if (generateTestBySAT(curFault, testVector) == 1) {
              successFlag = true;
              break;
            }
          }
          if (successFlag == false) {
            set<int> temp;
            for (auto fault : curFault) temp.insert(fault);
            undetectedDSAbySSAPT.insert(temp);
          }
        }
        cout << "\n\n\n\n\nthe ignoredFaults by the given SSAF test(check by SAT)" << endl;
        for (auto faultSet : undetectedDSAbySSAPT) {
          for (auto fault : faultSet) {
            cout << fault;
            if (redundantSSAF.find(fault) != redundantSSAF.end()) {
              cout << "(RSSAF)";
            }
            cout << " ";
          }
          vector<int> pattern;
          vector<int> inputFault;
          inputFault.assign(faultSet.begin(), faultSet.end());
          if (generateTestBySAT(inputFault, pattern) == 0) {
            cout << " ***R N F***";
          }
          cout << endl;
        }
        cout << "Undetected DSA number(by SAT): " << undetectedDSAbySSAPT.size() << endl;
      }

      void testDSA(set<int> &SSAFList, vector<vector<int>> &testVectors, set<set<int>> &undetectedDSAbySSAPT) {
          vector<int> list;
          vector<int> curFaults;
          for (auto elem : SSAFList) {
            list.push_back(elem);
          }
          vector<int> curFault;
          set<int> empty;
          for (int i = 0; i < list.size() - 1; i++) {
              for (int j = i + 1; j < list.size(); j++) {
                  curFault.clear();
                  curFault.push_back(list[i]);
                  curFault.push_back(list[j]);
                  bool successFlag = false;
                  for (auto testVector : testVectors) {
                    if (checkFaultAndTestVector(curFault, testVector, empty, empty, empty, 0) == 1) {
                    // if (generateTestBySAT(curFault, testVector) == 1) {
                      successFlag = true;
                      break;
                    }
                  }
                  if (successFlag == false) {
                    set<int> temp;
                    for (auto fault : curFault) temp.insert(fault);
                    undetectedDSAbySSAPT.insert(temp);
                  }
              }
          }
          /*
          cout << "\n\n\n\n\nthe ignoredFaults by the given SSAF test(check by circuitSimulation)" << endl;
          for (auto faultSet : undetectedDSAbySSAPT) {
            for (auto fault : faultSet) {
              cout << fault;
              if (redundantSSAF.find(fault) != redundantSSAF.end()) {
                cout << "(RSSAF)";
              }
              cout << " ";
            }
            vector<int> pattern;
            vector<int> inputFault;
            inputFault.assign(faultSet.begin(), faultSet.end());
            if (generateTestBySAT(inputFault, pattern) == 0) {
              cout << " ***R N F***";
            }
            cout << endl;
          }*/
          cout << "Undetected DSA number(by circuitSimulation): " << undetectedDSAbySSAPT.size() << endl;
      }

      void analyzeIgnoredUndetected(set<set<int>> &correct, set<set<int>> &wrong) {
        set<set<int>>ignored;
        for (auto DSA : correct) {
          if (wrong.find(DSA) == wrong.end()) {
            ignored.insert(DSA);
          }
        }
        int DSARedundantNumber = 0;
        vector<int> pattern;
        vector<int> inputFault;
        for (auto faultSet : ignored) {
          for (auto fault : faultSet) {
            pattern.clear();
            inputFault.clear();
            inputFault.push_back(fault);
            cout << fault;
            //if (redundantSSAF.find(fault) != redundantSSAF.end()) {
            if (generateTestBySAT(inputFault, pattern) == 0) {
              cout << "(RSSAF)";
            }
            cout << " ";
          }
          pattern.clear();
          inputFault.clear();
          inputFault.assign(faultSet.begin(), faultSet.end());
          if (generateTestBySAT(inputFault, pattern) == 0) {
            cout << " ***R N F***";
            DSARedundantNumber++;
          }
          cout << endl;
        }
        cout << "DSARedundantNumber:" << DSARedundantNumber << ", all:" << ignored.size() << endl;
      }
      // ---------------------------------------

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

      void printPotentiallyDSA2(map<int, set<int>> &potentiallyUndetected) {
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

      void printUndetectedDSA(set<set<int>> &undetectedDSA) {
        cout << "\nUndetected DSA:" << endl;
        for (auto DSA : undetectedDSA) {
          for (auto faultID : DSA) {
            cout << faultID << " ";
            //printFault2(faultID);
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

      int printFault2(int ID) {
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
          /*
          if (curCircuit[i]->isPath) {
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
          cout << " diff" << curCircuit[i]->different;
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
  };
}

#endif
