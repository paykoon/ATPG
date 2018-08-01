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
#include "CircuitSimulation.h"

using namespace std;
using namespace Gate;
using namespace Circuit;
using namespace Glucose;
using namespace TESTGENEBYSAT;
using namespace Simulation;


namespace ATPG{
  class atpg{
    public:
      vector <gate*> theCircuit;
      map<int, vector<uint64_t>> gateToRelatedGates;
      int PISize, POSize, gateSize;
      static const int WSIZE = 64;
      static const uint64_t one_64 = 0xffffffffffffffff;

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

      vector<vector<int>> AllPatterns;

      simulation *simulate;
      testgenebysat *testBySAT;
      // ----------------------------------


      atpg(circuit *pCircuit, char *patternFile, simulation *simulate, testgenebysat *testBySAT) {
        this->PISize = pCircuit->PISize;
        this->POSize = pCircuit->POSize;
        this->gateSize = pCircuit->gateSize;
        this->theCircuit = pCircuit->theCircuit;
        this->gateToRelatedGates = pCircuit->gateToRelatedGates;
        this->simulate = simulate;
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
        cout << "1. SSA Fault list generation. " << endl;
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

        /*
        set<int> connectedGates;
        findConnectedGatesDFS(theCircuit[52], connectedGates);
        vector <gate*> curCircuit;
        for (int gateID : connectedGates) {
          curCircuit.push_back(theCircuit[gateID]);
        }
        printCircuit(curCircuit);
        */

        cout << "   Build the connection between SSAF and the initial test set." << endl;
        cout << "   (Check the coverage for SSA, and generate additional test if it's not enough)" << endl;
        pairSSAPatternWithSSAF(allSSAFList, collapsedSSAFList, redundantSSAF, SSAFPatterns);
        cout << "   Number of redundant faults(among all faults): " << redundantSSAF.size() << endl;
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        endTime = clock();
        cout << "----------Time: " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------\n\n" << endl;

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
        cout << "----------Time: " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------\n\n" << endl;

        cout << "----------Additional test patterns generation for the undetected DSA faults----------" << endl;
        startTime = clock();
        generateDSATest(undetectedDSA);
        endTime = clock();
        cout << "   additional Test Pattern size: " << DSAFPatterns.size() << ", " << (double)DSAFPatterns.size()/SSAFPatterns.size() << " comparing with SSAF Patterns"<< endl;
        cout << "   redundantDSAF size: " << redundantDSAF.size() << ", " << (double)redundantDSAF.size()/undetectedDSA.size() << " among all undetected DSA"<< endl;
        cout << "----------Time: " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------\n\n" << endl;


        /*
        vector<int> curFault;
        curFault.push_back(418);
        curFault.push_back(2506);
        curFault.push_back(3050);
        AllPatterns.assign(SSAFPatterns.begin(), SSAFPatterns.end());
        AllPatterns.assign(DSAFPatterns.begin(), DSAFPatterns.end());
        if (simulate->checkallPatterns(curFault, AllPatterns) >= 0) {
          cout << "****detected****" << endl;
        } else {
          cout << "****undetected*****" << endl;
        }
        */
        /*
        set<set<int>> undetectedDSAbyDSAPT;
        set<set<int>> undetectedTSAbyDSAPT;
        vector<vector<int>> testVectors;
        testVectors.assign(SSAFPatterns.begin(), SSAFPatterns.end());
        for (auto DSA : DSAFPatterns) {
          testVectors.push_back(DSA);
        }
        //testDSA_pattern(allSSAFList, testVectors, undetectedDSAbyDSAPT);
        //printUndetectedDSA(undetectedDSAbyDSAPT);
        testTSA_pattern(allSSAFList, testVectors, undetectedTSAbyDSAPT);
        */
        return 1;
      }

      int getFaultID(int gateID, int port, int stuckat) {
        return (gateID << 3) + (port << 1) + stuckat;
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
        // get the undetected SSAF by original SSAF test
        vector<int> newFaults;
        set<int> checked;
        for (auto faultID : allFaultList) {
          newFaults.clear();
          newFaults.push_back(faultID);
          int patternIdx = simulate->checkallPatterns(newFaults, SSAFPatterns);
          if (patternIdx >= 0) {
            SSAFToPatterns.insert(make_pair(faultID, SSAFPatterns[patternIdx]));
            checked.insert(faultID);
          }
        }
        // check those undetected SSAF.
        set<int> undetectedSSA;
        for (auto faultID : allFaultList) {
          if (checked.find(faultID) == checked.end()) {
            undetectedSSA.insert(faultID);
          }
        }
        vector<int> curFault;
        vector<int> testVector;
        vector<vector<int>> newSSAPatterns;
        for (auto faultID : undetectedSSA) {
          testVector.clear();
          curFault.clear();
          curFault.push_back(faultID);
          if(simulate->checkallPatterns(curFault, newSSAPatterns) >= 0) continue;
          if (testBySAT->generateTestBySAT_1(curFault, testVector) > 0) {
            newSSAPatterns.push_back(testVector);
          } else {
            redundantSSAF.insert(faultID);
          }
        }
        if (newSSAPatterns.size() > 0) {
          cout << "   **The original SSAF test is not sufficient, generate new test pattern: " << newSSAPatterns.size() << endl;
          SSAFPatterns.assign(newSSAPatterns.begin(), newSSAPatterns.end());
        } else {
          cout << "   The original SSAF test is sufficient to cover all SSAF." << endl;
        }

        /*
        // the redundant fault among our fault model
        for (auto faultID : allFaultList) {
          if (checked.find(faultID) == checked.end()) {
            redundantSSAF.insert(faultID);
          }
        }
        */

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
        vector<uint64_t> relatedGates = simulate->getAllRelatedGates(newFaults);
        simulate->resetAllVisitedisPath(1, relatedGates);
        simulate->resetFaultsInCircuit();
        simulate->injectFaultsInCircuit(newFaults);
        simulate->assignPIs(testVector);
        simulate->propagatePI(1, relatedGates);

        simulate->resetAllVisitedisPath(1, relatedGates);
        simulate->resetFaultsInCircuit();
        simulate->assignPIs(testVector);
        simulate->propagatePI(1, relatedGates);

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

      // -------get the undetected DSA given the potentially undetected list----------
      // note: redundant will be represented as minus number;
      int isSet(int pos, vector<uint64_t> &vec) {
        int w = pos / WSIZE;
        int b = pos % WSIZE;
        uint64_t val = (vec[w] >> b);
        return (val & 1ULL) > 0 ? 1 : 0;
      }

      int faultIDToGateID(int faultID) {
        return (faultID >> 3);
      }

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
              if (abs(gateID1 - gateID2) < 10) {
                undetectedDSA.insert(DSA);
              }
            } else {
              redundantDSAF.insert(DSA);
            }
          }
        }
      }

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
          if(simulate->checkallPatterns(newFaults, SSAFPatterns) < 0) {
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
        for (auto DSA : undetectedDSA) {
          curFault.clear();
          for (auto faultID : DSA) {
            curFault.push_back(faultID);
          }
          //first try to use new test patterns to detect them
          if(simulate->checkallPatterns(curFault, DSAFPatterns) >= 0) continue;
          // if it cannot be detected, then generate test by SAT
          testVector.clear();
          if (testBySAT->generateTestBySAT_1(curFault, testVector) == 1) {
            DSAFPatterns.push_back(testVector);
          } else {
            redundantDSAF.insert(DSA);
          }
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
                  if(simulate->checkallPatterns(curFault, testVectors) < 0) {
                    set<int> temp;
                    for (auto fault : curFault) temp.insert(fault);
                    DSAtmp.insert(temp);
                  }
              }
          }
          int DSARedundantNumber = 0;

          for (auto faultSet : DSAtmp) {
            vector<int> pattern;
            vector<int> inputFault;
            inputFault.assign(faultSet.begin(), faultSet.end());
            if (testBySAT->generateTestBySAT_1(inputFault, pattern) == 0) {
            } else {
              undetectedDSAbyDSAPT.insert(faultSet);
            }
          }
          cout << "undetected DSA number: " << undetectedDSAbyDSAPT.size() << endl;
      }


      void testTSA_pattern(set<int> &SSAFList, vector<vector<int>> &testVectors, set<set<int>> &undetectedTSAbyDSAPT) {
        set<set<int>> TSAtmp;
        vector<int> list;
        vector<int> curFaults;
        for (auto elem : SSAFList) {
          list.push_back(elem);
        }
        vector<int> curFault;
        set<int> empty;
        int count = 0;
        vector<int> pattern;
        for (int i = 0; i < list.size() - 2; i++) {
          for (int j = i + 1; j < list.size() - 1; j++) {
            for (int k = j + 1; k < list.size(); k++) {
              curFault.clear();
              curFault.push_back(list[i]);
              curFault.push_back(list[j]);
              curFault.push_back(list[k]);
              if(simulate->checkallPatterns(curFault, testVectors) < 0) {
                set<int> temp;
                for (auto fault : curFault) temp.insert(fault);
                  TSAtmp.insert(temp);
  	              pattern.clear();
                  if (testBySAT->generateTestBySAT_1(curFault, pattern) > 0) {
                    count++;
                    cout << "\n\ncount: " << count << endl;
                    for (auto faultID : curFault) {
                      cout << faultID << " ";
                      printFault2(faultID);
                      vector<int> SSAFault;
                      SSAFault.push_back(faultID);
  		                pattern.clear();
                      if (testBySAT->generateTestBySAT_1(SSAFault, pattern) == 0) {
                        cout << "******Redundant SSAF******" << endl;
                      }
                    }
                    vector<int> DSAFault;
                    for (int a = 0; a < curFault.size() - 1; a++) {
                      for (int b = a + 1; b < curFault.size(); b++) {
                        DSAFault.clear();
                        DSAFault.push_back(curFault[a]);
                        DSAFault.push_back(curFault[b]);
                        pattern.clear();
                        if (testBySAT->generateTestBySAT_1(DSAFault, pattern) == 0) {
                          cout << curFault[a] << " " << curFault[b] << " " << endl;
                          cout << "******Redundant DSAF******" << endl;
                        }
                      }
                    }
                  }
               }
             }
           }
       }
        cout << "undetected TSA number: " << count << endl;
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
  };
}

#endif
