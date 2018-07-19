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
#include "Testgenebysat.h"
using namespace std;
using namespace Gate;
using namespace Circuit;
using namespace Glucose;
using namespace TESTGENEBYSAT;

namespace ATPG{
  class atpg{
    public:
      vector <gate*> theCircuit;
      map<int, vector<uint64_t>> gateToRelatedGates;
      static const int WSIZE = 64;
      static const uint64_t one_64 = 0xffffffffffffffff;
      int PISize, POSize, gateSize;
      // the circuit will be changed if faults are added to circuit.
      // the original unchanged gate will be stored here.
      map<int, gate*>preGateInTheCircuit;
      set<int> preInsertedFault;
      int faultsInsertedtheCircuit;

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

      set<set<int>> redundantDSAF;
      vector<vector<int>> DSAFPatterns;

      testgenebysat *testBySAT;

      set<set<int>> undetectedDSA2;
      // ----------------------------------


      atpg(circuit *pCircuit, char *patternFile, testgenebysat *testBySAT) {
        this->PISize = pCircuit->PISize;
        this->POSize = pCircuit->POSize;
        this->gateSize = pCircuit->gateSize;
        this->theCircuit = pCircuit->theCircuit;
        this->gateToRelatedGates = pCircuit->gateToRelatedGates;
        this->faultsInsertedtheCircuit = 0;
        this->testBySAT = testBySAT;
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
        preTime = clock();
        if ( !generateFaultList() ) return 0;
        cout << "   SSA Fault number is " << collapsedSSAFList.size() << " (Collapsed: AIG's input wire connected to PI or fanout)" << endl;
        cout << "   Total SSA faults number: " << allSSAFList.size() << ", " << 100*(double)(collapsedSSAFList.size()) / (double)(allSSAFList.size()) << "% fault are selected by the fault model" << endl;
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;

        cout << "2. Read initial SSAF test patterns." << endl;
        preTime = clock();
        if(patternParser(patternFile) == 1) {
          cout << "   " << SSAFPatterns.size() << " SSAF patterns are read" << endl;
        } else {
          cout << "   Pattern file has problem"<< endl;
          return 0;
        }
        cout << "   Build the connection between SSAF and the initial test set." << endl;
        cout << "   (We just assume that the initial test patterns can cover all SSAF)" << endl;
        pairSSAPatternWithSSAF(allSSAFList, collapsedSSAFList, redundantSSAF, SSAFPatterns);
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

        cout << "2. Compress undetected DSA Faults." << endl;
        preTime = clock();
        pairUndetectedDSAF(potentiallyUndetected, undetectedDSA, redundantSSAF);
        int preSize = undetectedDSA.size();
        cout << "   Undetected DSA number (without compressing): " << undetectedDSA.size() << endl;
        compressUndetectedDSAF(undetectedDSA);
        cout << "   Undetected DSA number (after compressing): " << undetectedDSA.size()  << ", Compressing ratio: " << (double)undetectedDSA.size()/preSize << endl;
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        endTime = clock();
        cout << "----------DSA Faults checking takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------\n\n" << endl;


        cout << "----------Additional test patterns generation for the undetected DSA faults----------" << endl;
        startTime = clock();
        generateDSATest(undetectedDSA);
        endTime = clock();
        cout << "   additional Test Pattern size: " << DSAFPatterns.size() << ", " << (double)DSAFPatterns.size()/SSAFPatterns.size() << " comparing with SSAF Patterns"<< endl;
        cout << "   redundantDSAF size: " << redundantDSAF.size() << ", " << (double)redundantDSAF.size()/undetectedDSA.size() << " among all undetected DSA"<< endl;
        cout << "----------Additional test patterns generation takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------\n\n" << endl;


        /*
        for (auto iter : SSAFToPatterns) {
          cout << "fault: " << iter.first << endl;
          cout << "";
          printTestVector(iter.second);
          vector<int> newFaults; newFaults.push_back(iter.first);
          set<int> empty;
          if(findSSAFsBlocked(newFaults, iter.second, empty, empty, empty, 0) == 0) {
            cout << "undetected" << endl;
          }
          cout << endl;
        }
        //*******************************
        //*******************************
        /*
        // 2722 12826
        curFault.clear();
        testVector.clear();
        empty.clear();
        curFault.push_back(2722);
        curFault.push_back(12826);
        if (testBySAT->generateTestBySAT(curFault, testVector) == 1) {
          if (findSSAFsBlocked(curFault, testVector, empty, empty, empty, 0) == 1) {
            cout << "***************equal****************" << endl;
          } else {
            cout << "***************not equal****************" << endl;
          }
        }
        printTestVector(testVector);
        */
        //*******************************
        //*******************************


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
        set<set<int>> undetectedDSAbyDSAPT;
        vector<vector<int>> testVectors;
        testVectors.assign(SSAFPatterns.begin(), SSAFPatterns.end());
        for (auto DSA : DSAFPatterns) {
          testVectors.push_back(DSA);
        }
        testDSA_pattern(allSSAFList, testVectors, undetectedDSAbyDSAPT);
        printUndetectedDSA(undetectedDSAbyDSAPT);
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
        // getUndetectedDSA();

        return 1;
      }


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
      void pairSSAPatternWithSSAF(set<int> &allFaultList, set<int> &collapsedSSAFList, set<int> &redundantSSAF, vector<vector<int>> &SSAFPatterns) {
        vector<int> newFaults;
        set<int> checked;
        for (auto faultID : allFaultList) {
          newFaults.clear();
          newFaults.push_back(faultID);
          int patternIdx = checkallPatterns(newFaults, SSAFPatterns);
          if (patternIdx >= 0) {
            SSAFToPatterns.insert(make_pair(faultID, SSAFPatterns[patternIdx]));
            checked.insert(faultID);
          }
        }
        // the redundant fault among our fault model
        for (auto faultID : allFaultList) {
          if (checked.find(faultID) == checked.end()) {
            redundantSSAF.insert(faultID);
          }
        }
        for (auto faultID : allFaultList) {
          if (checked.find(faultID) == checked.end()) {
            redundantSSAF.insert(faultID);
          }
        }


        /*
        set<int> checked1;
        vector<int> newFaults1;
        set<int> empty1;
        for (auto testVector : SSAFPatterns) {
          if (checked1.size() == allFaultList.size()) break; // all faults are checked
          for (auto faultID : allFaultList) {
            if (checked1.find(faultID) != checked1.end()) continue;  // already checked
            newFaults1.clear();
            newFaults1.push_back(faultID);
            if (findSSAFsBlocked(newFaults1, testVector, empty1, empty1, empty1, 0) == 1) {
              checked1.insert(faultID);
            }
          }
        }
        set<int> redundantSSAF1;
        for (auto faultID : allFaultList) {
          if (checked1.find(faultID) == checked1.end()) {
            redundantSSAF1.insert(faultID);
          }
        }
        cout << "ignored redundant SSAF" << endl;
        for (auto faultID : redundantSSAF1) {
          if (redundantSSAF.find(faultID) == redundantSSAF.end()) {
            vector<int> curFault;
            curFault.push_back(faultID);
            vector<int> testVector;
            if (testBySAT->generateTestBySAT(curFault, testVector) == 0) {
              cout << "Real redundant" << endl;
            }
            cout << faultID << endl;
          }
        }
        */
      }
      //------------------Process about the initial SSAF test pattern------------------

      int getFaultID(int gateID, int port, int stuckat) {
        return (gateID << 3) + (port << 1) + stuckat;
      }

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
              }
              val >>= 1;
              gateID++;
            }
            w++;
          }
          for (int i = PI + gateSize; i < theCircuit.size(); i++) {
            theCircuit[i]->different = false;
            theCircuit[i]->isPath = false;
          }
        }
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
      int findSSAFsBlocked(vector<int> &newFaults, vector<int> &testVector, set<int> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAF, int mode) {
        //*****************
        /*
        set<int> connectedGates;
        findConnectedGatesDFS(theCircuit[faultIDToGateID(12826)], connectedGates);
        vector <gate*> curCircuit;
        for (auto faultID : connectedGates) {
          curCircuit.push_back(theCircuit[faultID]);
        }
        */
        //printCircuit(theCircuit);
        //****************
        vector<uint64_t> relatedGates = getAllRelatedGates(newFaults);
        resetAllVisitedisPath(1, relatedGates);
        resetFaultsInCircuit();
        injectFaultsInCircuit(newFaults);
        assignPIs(testVector);
        propagatePI(1, relatedGates);

        resetAllVisitedisPath(1, relatedGates);
        resetFaultsInCircuit();
        assignPIs(testVector);
        propagatePI(1, relatedGates);


        // ****only for DSA fault now*******
        int gateID = newFaults[0] >> 3;
        int port = (newFaults[0] >> 1) & 3;
        gate *POGate;
        gate *curGate;
        gate *preGate;
        for (int i = 0; i < POSize; i++) {
          POGate = theCircuit[PISize + gateSize + i];
          // if the fautls can be detected in one of the PO, then it's detected.
          if (POGate->different == true) {
            curGate = theCircuit[gateID];
            if (mode == 1) {
              preGate = (port == 1) ? curGate->fanin1 : curGate->fanin2;
              findBlockSSADFS(theCircuit[gateID], preGate, blockFaultsList, faultList, redundantSSAF, newFaults[0]);
            }
            return 1;
          }
        }
        return 0;
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

      // find the potentiallyUndetected.
      // assume that the initial test pattern can already detect all SSAF.
      // get: map<int, set<int>> &potentiallyUndetected;
      void findPotentialUndetectedDSAF(map<int, vector<int>> &SSAFToPatterns, set<int> &redundantSSAF, vector<gate*> &theCircuit, set<int> &faultList, map<int, set<int>> &potentiallyUndetected) {
        vector<int> newFaults;
      	set<int>blockFaultsList;
      	vector<int>testVector;
	      for (auto iter : SSAFToPatterns) {
          int faultID = iter.first;
          newFaults.clear();
	        newFaults.push_back(faultID);
          blockFaultsList.clear();
	        testVector = iter.second;
          findSSAFsBlocked(newFaults, testVector, blockFaultsList, faultList, redundantSSAF, 1);
          if (blockFaultsList.size() > 0) {
             potentiallyUndetected.insert(make_pair(faultID, blockFaultsList));
          }
        }
      }
      //------------------Find the propagation path of the faults under the given test pattern and the potential undetected faults -------------------


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
        set<int> firstSet;
      	set<int> secondSet;
      	set<int> temp;
        int count = 0;
      	for (map<int, set<int>>::iterator iter = potentiallyUndetected.begin(); iter != potentiallyUndetected.end(); iter++) {
          int first = iter->first;
          firstSet = iter->second;
          for (auto second : firstSet) {
            if (potentiallyUndetected.find(second) != potentiallyUndetected.end() || redundantSSAF.find(second) != redundantSSAF.end()) {
              secondSet = potentiallyUndetected[second];
              if (secondSet.find(first) != secondSet.end() || redundantSSAF.find(second) != redundantSSAF.end()) {
                temp.clear();
                temp.insert(first);
                temp.insert(second);
                undetectedDSA.insert(temp);
              }
            }
          }
        }
        // add the double faults that are both redundant to the list.
        // only add two faults that are mutually in the related gates.
        vector<int> vec;
        for (auto faultID : redundantSSAF) {
          vec.push_back(faultID);
        }
        int size = vec.size();
        for (int i = 0; i < size - 1; i++) {
          for (int j = i + 1; j < size; j++) {
            int gateID1 = faultIDToGateID(vec[i]);
            int gateID2 = faultIDToGateID(vec[j]);
            set<int> DSA;
            DSA.insert(vec[i]);
            DSA.insert(vec[j]);
            if (isSet(gateID2, gateToRelatedGates[gateID1]) == 1 && isSet(gateID1, gateToRelatedGates[gateID2]) == 1) {
              undetectedDSA.insert(DSA);
            } else {
              redundantDSAF.insert(DSA);
            }
          }
        }
      }

      //***********************************
      //***********************************
      /*
      void getUndetectedDSA() {
        set<vector<int>> allTestPattern;
        for (auto pattern : SSAFPatterns) {
          allTestPattern.insert(pattern);
        }
        allTestPattern.insert(DSAFPatterns.begin(), DSAFPatterns.end());

        set<set<int>> ignoredDSA;
        vector<int> newFaults;
        set<int> empty;
        for (auto DSA : undetectedDSA2) {
          newFaults.clear();
          // 2722 12826
          int faultID1 = 2722;
          int faultID2 = 12826;
          newFaults.push_back(faultID1);
          newFaults.push_back(faultID2);
          //for (auto faultID : DSA) {
          //  newFaults.push_back(faultID);
          //}
          bool detected = false;
          for (auto testVector : allTestPattern) {
            if (findSSAFsBlocked(newFaults, testVector, empty, empty, empty, 0) == 1) {
                detected = true;
                break;
            }
          }
          if (detected == false) {
            cout << "********undetected*******"<< endl;
          }
        }
        //cout << "ignoredDSA Number: " << ignoredDSA.size() << endl;
        vector<int> curFault;
        curFault.push_back(faultID1);
        vector<int> testVector;
        if (!testBySAT->generateTestBySAT(curFault, testVector)) {
          cout << faultID1 << " is SSA redundant" << endl;
        }
        curFault.clear();
        testVector.clear();
        curFault.push_back(faultID2);
        if (!testBySAT->generateTestBySAT(curFault, testVector)) {
          cout << faultID2 << " is SSA redundant" << endl;
        }

        curFault.push_back(faultID1);
        testVector.clear();
        if (!testBySAT->generateTestBySAT(curFault, testVector)) {
          cout << faultID1 << " " << faultID2 << " is DSA redundant" << endl;
        }
      }
      */

      void compressUndetectedDSAF(set<set<int>> &undetectedDSA) {
        set<set<int>> finalUndetected;
        set<int>DSA;
      	vector<int>newFaults;
      	set<int>empty;
      	set<int>temp;
      	int count = 0;
      	int tmp=0;
      	for (set<set<int>>::iterator iter = undetectedDSA.begin(); iter != undetectedDSA.end(); iter++) {
	      	DSA = *iter;
      	  newFaults.clear();
          for (auto faultID : DSA) {
            newFaults.push_back(faultID);
          }
          /*
          bool isDetected = false;
          for (auto testVector: SSAFPatterns) {
            if (findSSAFsBlocked(newFaults, testVector, empty, empty, empty, 0) == 1) {
              isDetected = true;
              break;
            }
          }
          if (isDetected == false) {
            temp.clear();
            temp.insert(DSA.begin(), DSA.end());
            finalUndetected.insert(temp);
          }
          */
          if(checkallPatterns(newFaults, SSAFPatterns) < 0) {
            temp.clear();
            temp.insert(DSA.begin(), DSA.end());
            finalUndetected.insert(temp);
          }
        }
        undetectedDSA.clear();
        undetectedDSA.insert(finalUndetected.begin(), finalUndetected.end());
      }
      // -----------------------------------------------------------------------------

      void generateDSATest(set<set<int>> &undetectedDSA) {
        vector<int> curFault;
        vector<int> testVector;
        set<int> empty;
        set<set<int>> DSANeedNewPattern;
        for (auto DSA : undetectedDSA) {
          curFault.clear();
          for (auto faultID : DSA) {
            curFault.push_back(faultID);
          }
          //first try to use new test patterns to detect them
          /* bool isDetected = false;
          for (auto newTestVector : DSAFPatterns) {
            if (findSSAFsBlocked(curFault, newTestVector, empty, empty, empty, 0) == 1) {
              isDetected = true;
              break;
            }
          }
          if (isDetected == true) continue;  */
          if(checkallPatterns(curFault, DSAFPatterns) >= 0) continue;
          // if it cannot be detected, then generate test by SAT
          testVector.clear();
          if (testBySAT->generateTestBySAT_1(curFault, testVector) == 1) {
            DSAFPatterns.push_back(testVector);
            DSANeedNewPattern.insert(DSA);
          } else {
            redundantDSAF.insert(DSA);
          }
        }
        cout << "   Number of DSA faults needs new test pattern is: " << DSANeedNewPattern.size() << endl;
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
                    if (findSSAFsBlocked(curFault, testVector, empty, empty, empty, 0) == 1) {
                    //if (testBySAT->generateTestBySAT(curFault, testVector) == 1) {
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
          cout << "\n\n\n\n\nthe ignoredFaults by the given SSAF test(check by circuitSimulation)" << endl;
          int DSARedundantNumber = 0;
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
            if (testBySAT->generateTestBySAT(inputFault, pattern) == 0) {
              cout << " ***R N F***";
              DSARedundantNumber++;
            }
            cout << endl;
          }
          cout << "Really Undetected(Except redundant): " << undetectedDSAbySSAPT.size() - DSARedundantNumber << endl;
          cout << "Undetected DSA number(by circuitSimulation): " << undetectedDSAbySSAPT.size() << endl;
      }

      void testDSA_pattern(set<int> &SSAFList, vector<vector<int>> &testVectors, set<set<int>> &undetectedDSAbyDSAPT) {
          set<set<int>> DSAtmp;
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
                    if (findSSAFsBlocked(curFault, testVector, empty, empty, empty, 0) == 1) {
                      successFlag = true;
                      break;
                    }
                  }
                  if (successFlag == false) {
                    set<int> temp;
                    for (auto fault : curFault) temp.insert(fault);
                    DSAtmp.insert(temp);
                  }
              }
          }
          cout << "\n\n\n\n\nthe ignoredFaults by the given DSA test(check by circuitSimulation)" << endl;
          int DSARedundantNumber = 0;
          for (auto faultSet : DSAtmp) {
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
            if (testBySAT->generateTestBySAT(inputFault, pattern) == 0) {
              cout << " ***R N F***";
              DSARedundantNumber++;
            } else {
              undetectedDSAbyDSAPT.insert(faultSet);
            }
            cout << endl;
          }
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
            if (redundantSSAF.find(fault) != redundantSSAF.end()) {
            //if (generateTestBySAT(inputFault, pattern) == 0) {
              cout << "(RSSAF)";
            }
            cout << " ";
          }
          pattern.clear();
          inputFault.clear();
          inputFault.assign(faultSet.begin(), faultSet.end());
          if (testBySAT->generateTestBySAT(inputFault, pattern) == 0) {
            cout << " ***R N F***";
            DSARedundantNumber++;
          }
          cout << endl;
        }
        cout << "DSARedundantNumber:" << DSARedundantNumber << ", all:" << ignored.size() << endl;
      }
      // ---------------------------------------

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
        cout << "\n\n\nUndetected DSA:" << endl;
        for (auto DSA : undetectedDSA) {
          for (auto faultID : DSA) {
            cout << faultID << " ";
            printFault2(faultID);
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
  };
}

#endif
