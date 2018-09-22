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
      map<int, int> twoFanoutGateToReconvGate;

      //-------fault list and test pattern----------------
      set<int> collapsedSSAFList;
      set<int> allSSAFList;
      set<int> notIncollapsedSSAFList;
      set<int> redundantSSAF;

      vector<vector<int>> SSAFPatterns;
      // key: faults, value: corresponding test vector
      map<int, vector<int>> SSAFToPatterns;
      // ---------------------------------
      // key: fault. value: its blocked faults.
      map<int, set<int>> potentiallyUndetected;
      set<set<int>> undetectedDSA;

      set<set<int>> redundantDSAF;
      vector<vector<int>> DSAFPatterns;
      map<set<int>,vector<int>> DSAFToPatterns; // DSA and pattern. if DSA is redundant, pattern size=0;

      vector<vector<int>> AllPatterns;

      simulation *simulate;
      testgenebysat *testBySAT;
      char *TSAFile;

      vector<vector<int>> TSAFPatterns;
      set<set<int>> redundantTSAFList;
      map<int, set<int>> SSAFToPathGates;
      // map<fault, map<gateID, set<the fault affects the side value in that gate>>>
      map<int, map<int, set<int>>> SSAFToFaultsBlockedInGate;

      vector<vector<int>> aigInputs;
      // ----------------------------------

      //***************for test*************
      set<set<int>> case1Fault, case2Fault;
      //************************

      atpg(circuit *pCircuit, char *patternFile, simulation *simulate, testgenebysat *testBySAT, char *TSAFile) {
        this->PISize = pCircuit->PISize;
        this->POSize = pCircuit->POSize;
        this->gateSize = pCircuit->gateSize;
        this->theCircuit = pCircuit->theCircuit;
        this->gateToRelatedGates = pCircuit->gateToRelatedGates;
        this->twoFanoutGateToReconvGate = pCircuit->twoFanoutGateToReconvGate;
        this->simulate = simulate;
        this->testBySAT = testBySAT;
        this->TSAFile = TSAFile;
        for (int i = 0; i <= 1; i++) {
          for (int j = 0; j <= 1; j++) {
            vector<int> oneInput;
            oneInput.push_back(i); oneInput.push_back(j);
            aigInputs.push_back(oneInput);
          }
        }
        ATPGInit(patternFile, TSAFile);
      }

      ~atpg() {}

      // if initialization has problem, return 0
      // else return 1;
      int ATPGInit(char *patternFile, char *TSAFile) {
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

        cout << "   Build the connection between SSAF and the initial test set." << endl;
        cout << "   (Check the coverage for SSA, and generate additional test if it's not enough)" << endl;
        pairSSAPatternWithSSAF(allSSAFList, collapsedSSAFList, redundantSSAF, SSAFPatterns);
        cout << "   Number of redundant faults(among all faults): " << redundantSSAF.size() << endl;
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        endTime = clock();
        cout << "----------Time: " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------\n\n" << endl;


        //***********************************
        /*
        vector<int> newFaults;
        newFaults.push_back(3316);
        set<int> empty;
        if (findSSAFsBlocked(newFaults, SSAFToPatterns[3316], empty, empty, empty, 0) == 1) cout << "*****detected*****" << endl;
        else cout << "*****undetected*****" << endl;

        vector<uint64_t> relatedGates;

        simulate->resetAllVisitedisPath(0, relatedGates);
        simulate->resetFaultsInCircuit();
        simulate->assignPIs(SSAFToPatterns[3316]);
        simulate->propagatePI(0, relatedGates);

        cout << "\n\n\n" << endl;
        set<int> connectedGates;
        findConnectedGatesDFS(theCircuit[faultIDToGateID(3316)], connectedGates);
        vector<gate*> curCircuit;
        int flag = 0;
        for (auto faultID : connectedGates) {
          curCircuit.push_back(theCircuit[faultID]);
        }
        printCircuit(curCircuit);
        cout << "\n\n\n" << endl;
        printCircuitBlif(curCircuit);
        */
        //***************************************


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
        AllPatterns.insert(AllPatterns.end(), SSAFPatterns.begin(), SSAFPatterns.end());
        AllPatterns.insert(AllPatterns.end(), DSAFPatterns.begin(), DSAFPatterns.end());
        cout << "----------Time: " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------\n\n" << endl;

        //************************
        // for test of ISQED paper.
        /*
        cout << "\n\n   fault overlook by previous version.\n\n" << endl;
        cout << "   cases1(D and ~D meet): " << case1Fault.size() << endl;
        int re = 0, nonRe = 0;
        for (auto DSA : case1Fault) {
          vector<int> curFault;
          vector<int> testVector;
          curFault.assign(DSA.begin(), DSA.end());
          if (testBySAT->generateTestBySAT_1(curFault, testVector) == 0) {
            re++;
          } else {
            nonRe++;
          }
        }
        cout << "   reDSA: " << re << endl << "   nonReDSA: " << nonRe << endl;
        cout << "\n   cases2(redundant in path): " << case2Fault.size() << endl;
        re = 0, nonRe = 0;
        for (auto DSA : case2Fault) {
          vector<int> curFault;
          vector<int> testVector;
          curFault.assign(DSA.begin(), DSA.end());
          if (testBySAT->generateTestBySAT_1(curFault, testVector) == 0) {
            re++;
          } else {
            nonRe++;
          }
        }
        cout << "   reDSA: " << re << endl << "   nonReDSA: " << nonRe << "\n\n\n";
        */
        //**************************************************

        cout << "\n\n\n----------TSA Faults Checking----------" << endl;
        startTime = clock();
        map<int, set<set<int>>> SSAToblockDSA1;
        map<int, set<set<int>>> SSAToblockDSA2;
        map<set<int>, set<int>> DSAToblockSSA;
        set<set<int>> undetectedTSA;
        cout << "1. Search the undetected TSA Faults." << endl;
        preTime = clock();
        findPotentialUndetectedTSAF(SSAToblockDSA1, SSAToblockDSA2, DSAToblockSSA);
        pairPotentialUndetectedTSAF(SSAToblockDSA1, SSAToblockDSA2, DSAToblockSSA, undetectedTSA);
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;

        cout << "\n2. Compress undetected TSA Faults." << endl;
        preSize = undetectedTSA.size();
        cout << "   Undetected TSA number (without compressing): " << undetectedTSA.size() << endl;
        compressundetectedTSA(undetectedTSA, AllPatterns);
        cout << "   Undetected TSA number (after compressing): " << undetectedTSA.size()  << ", Compressing ratio: " << (double)undetectedTSA.size()/preSize << endl;
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;

        cout << "\n3. Generate additional test patterns for undetected TSA Faults." << endl;
        preTime = clock();
        generateTSATest(undetectedTSA, TSAFPatterns, redundantTSAFList);
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        cout << "   additional Test Pattern size: " << TSAFPatterns.size() << endl;
        cout << "   redundantDSAF size: " << redundantTSAFList.size() << endl;
        AllPatterns.insert(AllPatterns.end(), TSAFPatterns.begin(), TSAFPatterns.end());
        endTime = clock();
        cout << "----------Time: " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------\n\n" << endl;

        /*
        //set<set<int>> TSAs;
        //TSAParser(TSAFile, TSAs);
        //checkTSAAgain(TSAs);
        //set<set<int>> undetectedDSAbyDSAPT;
        //set<set<int>> undetectedTSAbyDSAPT;
        //testDSA_pattern(allSSAFList, AllPatterns, undetectedDSAbyDSAPT);
        //printUndetectedDSA(undetectedDSAbyDSAPT);
        testTSA_pattern(allSSAFList, AllPatterns);
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

      //------------------Find the propagation path of the faults under the given test pattern and the potential undetected faults -------------------
      // function: recursively find the same fault in the path of current fault's gate to PI.
      // run in the non-faulty circuit with correct values
      // for AIG, only when outValue != invOut, can find the same fault.
      void sameFaultCurToPIDFS(gate *curGate, set<int> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAF, set<int> &blockFaultInCurrentGate, int oriFault) {
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
          if (curGate->getOutValue(in1Inverse, in2Value) == outInverse) {
            int blockFaultID = getFaultID(curGate->gateID, 1, in1Inverse);
            if (faultList.find(blockFaultID) != faultList.end() || redundantSSAF.find(blockFaultID) != redundantSSAF.end()) {
              blockFaultsList.insert(blockFaultID);
              blockFaultInCurrentGate.insert(blockFaultID);
            }
            // the current position may not be in our model, but the front position(near to PO) may be, we need to keeping track  to front place...
            sameFaultCurToPIDFS(curGate->fanin1, blockFaultsList, faultList, redundantSSAF, blockFaultInCurrentGate, oriFault);
          }
          if (curGate->getOutValue(in1Value, in2Inverse) == outInverse) {
            int blockFaultID = getFaultID(curGate->gateID, 2, in2Inverse);
            if (faultList.find(blockFaultID) != faultList.end() || redundantSSAF.find(blockFaultID) != redundantSSAF.end()) {
              blockFaultsList.insert(blockFaultID);
              blockFaultInCurrentGate.insert(blockFaultID);
            }
            sameFaultCurToPIDFS(curGate->fanin2, blockFaultsList, faultList, redundantSSAF, blockFaultInCurrentGate, oriFault);
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
                blockFaultInCurrentGate.insert(blockFaultID);
                //***for test***
                set<int>tmp;
                tmp.insert(oriFault);
                tmp.insert(blockFaultID);
                case1Fault.insert(tmp);
                //**************
              }
              blockFaultID = getFaultID(curGate->gateID, 2, curGate->invIn2);
              if (faultList.find(blockFaultID) != faultList.end() || redundantSSAF.find(blockFaultID) != redundantSSAF.end()) {
                blockFaultsList.insert(blockFaultID);
                blockFaultInCurrentGate.insert(blockFaultID);
                //***for test***
                set<int>tmp;
                tmp.insert(oriFault);
                tmp.insert(blockFaultID);
                case1Fault.insert(tmp);
                //**************
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
            blockFaultInCurrentGate.insert(blockFaultID);
          }
          sameFaultCurToPIDFS(curGate->fanin1, blockFaultsList, faultList, redundantSSAF, blockFaultInCurrentGate, oriFault);
        }
      }

      // function: recursively find the SSA faults' path; also try to find the faults that may block it.
      // run in the non-faulty circuit with correct values
      // base case : reach PO. return true(has check visited in previous level)
      // recursion rule: select the visited fanout and go into next level.
      //                 check it's side value to find the block faults.
      // if none of fanout return true, we will return false.
      bool findBlockSSADFS(gate *curGate, gate *preGate, set<int> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAFList, set<int> &pathGatesID, map<int, set<int>> &gateToFaultsBlock, int oriFault) {
        // base case
        if (curGate->gateType == PO) {
            return true;
        }
        bool isPath = false;
        // check all the propagation paths to find the possible block fault.
        for (auto fanout : curGate->fanout) {
          // if one of its fanout can propagate the value, this gate is in the propagation path
          if (fanout->different == true && findBlockSSADFS(fanout, curGate, blockFaultsList, faultList, redundantSSAFList, pathGatesID, gateToFaultsBlock, oriFault) == true) {
            isPath = true;
            // try to find the SSAF that may block the fault
            if (curGate->gateType == aig) {
              // check side value.
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
                set<int> blockFaultInCurrentGate;
                sameFaultCurToPIDFS((curGate->fanin1 == preGate ? curGate->fanin2 : curGate->fanin1), blockFaultsList, faultList, redundantSSAFList, blockFaultInCurrentGate, oriFault);
                gateToFaultsBlock.insert(make_pair(curGate->gateID, blockFaultInCurrentGate));
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
              //***for test***
              set<int>tmp;
              tmp.insert(oriFault);
              tmp.insert(samePathBlockFaultID0);
              case2Fault.insert(tmp);
              //**************
            }
            if (redundantSSAFList.find(samePathBlockFaultID1) != redundantSSAFList.end()) {
              blockFaultsList.insert(samePathBlockFaultID1);
              //***for test***
              set<int>tmp;
              tmp.insert(oriFault);
              tmp.insert(samePathBlockFaultID1);
              case2Fault.insert(tmp);
              //**************
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
        if (isPath) {
          pathGatesID.insert(curGate->gateID);
        }
        return isPath;
      }

      // given the faultID and test vector, mark the gate in the propgation path as "isPath = true"
      // first propagate PI in faulty circuit, then do the same thing in original circuit.
      // so the outValue remains in the cirucit is the value to activate and propagte faults
      // also find the potentiallyUndetected.
      // return 1, if faults can be tested by the test pattern, else return 0.
      // mode = 0, if dont want to find the block faults, then set blockFaultsList, faultList and redundantSSAF to empty.
      // mode = 1. also find the block faults of all fault in the newFaults list.
      // mode = 2, only check the first fault in the newFaults list.
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
        gate *POGate;
        gate *curGate;
        gate *preGate;
        for (int i = 0; i < POSize; i++) {
          POGate = theCircuit[PISize + gateSize + i];
          // if the faults can be detected in one of the PO, then it's detected.
          if (POGate->different == true) {
            if (mode > 0) {
              for (auto faultID : newFaults) {
                int gateID = faultID >> 3;
                int port = (faultID >> 1) & 3;
                curGate = theCircuit[gateID];
                preGate = (port == 1) ? curGate->fanin1 : curGate->fanin2;
                set<int> pathGatesID;
                map<int, set<int>> gateToFaultsBlock;
                findBlockSSADFS(theCircuit[gateID], preGate, blockFaultsList, faultList, redundantSSAF, pathGatesID, gateToFaultsBlock, faultID);
                if (newFaults.size() == 1) {
                  SSAFToFaultsBlockedInGate.insert(make_pair(faultID, gateToFaultsBlock));
                  SSAFToPathGates.insert(make_pair(faultID, pathGatesID));
                }
                // mode2, only check the path of first fault.
                if (mode == 2) break;
              }
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
      	set<int> blockFaultsList;
      	vector<int> testVector;
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

        set<set<int>> deleteDSAs;
        deleteDSARedundantPairs(redundantSSAF, deleteDSAs);
        cout << "*******the redundant SSAF pairs that are in same path: " <<  deleteDSAs.size() << endl;
        cout << "*******(must be redundant DSAF, will not be included in undetectedDSA)" << endl;

        // add the double faults that are both redundant to the list.
        // only add two faults that are mutually in the related gates.
        vector<int> vec;
        vec.assign(redundantSSAF.begin(), redundantSSAF.end());
        int size = vec.size();
        for (int i = 0; i < size - 1; i++) {
          for (int j = i + 1; j < size; j++) {
            int gateID1 = faultIDToGateID(vec[i]);
            int gateID2 = faultIDToGateID(vec[j]);
            set<int> DSA;
            DSA.insert(vec[i]);
            DSA.insert(vec[j]);
            if (isSet(gateID2, gateToRelatedGates[gateID1]) == 1 && isSet(gateID1, gateToRelatedGates[gateID2]) == 1 &&
                deleteDSAs.find(DSA) == deleteDSAs.end()) {
              undetectedDSA.insert(DSA);
            } else {
              redundantDSAF.insert(DSA);
            }
          }
        }
      }

      // --delete the redundant SSA fault pairs that are in the same path.
      void deleteDSARedundantPairs(set<int> &redundantSSAF, set<set<int>> &deleteDSAs) {
        for (auto faultID : redundantSSAF) {
          set<int> deleteList;
          gate *curGate = theCircuit[faultIDToGateID(faultID)];
          checkPath(curGate, NULL, deleteList, redundantSSAF);
          for (auto another : deleteList) {
            set<int> pair;
            pair.insert(faultID);
            pair.insert(another);
            deleteDSAs.insert(pair);
          }
        }
      }

      void checkPath(gate *curGate, gate *preGate, set<int> &deleteList, set<int> &redundantSSAF) {
        if (curGate->gateType == PO) return;
        int gateID = 0, port = 0, stuckat = 0, faultID = 0;
        gateID = curGate->gateID;
        // check redundant fault at input
        if (preGate != NULL) {
          port = (curGate->fanin1 == preGate) ? 1 : 2;
          stuckat = 0;
          faultID = getFaultID(gateID, port, stuckat);
          if (redundantSSAF.find(faultID) != redundantSSAF.end()) deleteList.insert(faultID);
          stuckat = 1;
          faultID = getFaultID(gateID, port, stuckat);
          if (redundantSSAF.find(faultID) != redundantSSAF.end()) deleteList.insert(faultID);
        }
        // check redundant fault at output
        port = 3;
        stuckat = 0;
        faultID = getFaultID(gateID, port, stuckat);
        if (redundantSSAF.find(faultID) != redundantSSAF.end()) deleteList.insert(faultID);
        stuckat = 1;
        faultID = getFaultID(gateID, port, stuckat);
        if (redundantSSAF.find(faultID) != redundantSSAF.end()) deleteList.insert(faultID);
        for (auto neiGate : curGate->fanout) {
          checkPath(neiGate, curGate, deleteList, redundantSSAF);
        }
      }
      // --

      void compressUndetectedDSAF(set<set<int>> &undetectedDSA) {
        set<set<int>> finalUndetected;
        set<int> DSA;
      	vector<int> newFaults;
      	set<int> empty;
      	set<int> temp;
        set<int> mapFault;
      	int count = 0;
      	int tmp=0;
      	for (set<set<int>>::iterator iter = undetectedDSA.begin(); iter != undetectedDSA.end(); iter++) {
	      	DSA = *iter;
      	  newFaults.clear();
          for (auto faultID : DSA) {
            newFaults.push_back(faultID);
          }
          int index = simulate->checkallPatterns(newFaults, SSAFPatterns);
          if(index < 0) {
            temp.clear();
            temp.insert(DSA.begin(), DSA.end());
            finalUndetected.insert(temp);
          } else {
            mapFault.clear();
            mapFault.insert(newFaults.begin(), newFaults.end());
            DSAFToPatterns.insert(make_pair(mapFault, SSAFPatterns[index]));
          }
        }
        undetectedDSA.clear();
        undetectedDSA.insert(finalUndetected.begin(), finalUndetected.end());
      }
      // -----------------------------------------------------------------------------

      // generate additional test for DSA faults.
      void generateDSATest(set<set<int>> &undetectedDSA) {
        vector<int> curFault;
        vector<int> testVector;
        set<int> empty;
        set<int> mapFault;

        int count_NonRedundantDSAF = 0;
        int count_RedundantDSAF = 0;

        int count_twoNonReSSA = 0;
        int count_twoNonReSSA_ReDSA = 0;
        int count_twoNonReSSA_NonReDSA = 0;

        int count_oneNonReSSA = 0;
        int count_oneNonReSSA_ReDSA = 0;
        int count_oneNonReSSA_NonReDSA = 0;

        int count_twoReSSA = 0;
        int count_twoReSSA_ReDSA = 0;
        int count_twoReSSA_NonReDSA = 0;

        for (auto DSA : undetectedDSA) {
          int ReDSA_flag = 0; //*******************************
          curFault.clear();
          for (auto faultID : DSA) {
            curFault.push_back(faultID);
          }
          //first try to use new test patterns to detect them
          int index = simulate->checkallPatterns(curFault, DSAFPatterns);
          if(index >= 0) {
            mapFault.clear();
            mapFault.insert(curFault.begin(), curFault.end());
            //DSAFToPatterns.insert(make_pair(mapFault, DSAFPatterns[index]));
            //*******just for collecting the experiment data of ISQED paper*********
            count_NonRedundantDSAF++; //*******************************
            if (redundantSSAF.find(curFault[0]) == redundantSSAF.end() && redundantSSAF.find(curFault[1]) == redundantSSAF.end()) {
              count_twoNonReSSA++;
              if (ReDSA_flag == 1) count_twoNonReSSA_ReDSA++;
              else count_twoNonReSSA_NonReDSA++;
            } else if (redundantSSAF.find(curFault[0]) != redundantSSAF.end() && redundantSSAF.find(curFault[1]) != redundantSSAF.end()) {
              count_twoReSSA++;
              if (ReDSA_flag == 1) count_twoReSSA_ReDSA++;
              else {
                count_twoReSSA_NonReDSA++;
              }
            } else {
              count_oneNonReSSA++;
              if (ReDSA_flag == 1) count_oneNonReSSA_ReDSA++;
              else count_oneNonReSSA_NonReDSA++;
            }
            //****************************************************
            continue;
          }
          // if it cannot be detected, then generate test by SAT
          testVector.clear();
          if (testBySAT->generateTestBySAT_1(curFault, testVector) == 1) {
            DSAFPatterns.push_back(testVector);
            count_NonRedundantDSAF++; //*******************************
            mapFault.clear();
            mapFault.insert(curFault.begin(), curFault.end());
            DSAFToPatterns.insert(make_pair(mapFault, testVector));
          } else {
            redundantDSAF.insert(DSA);
            count_RedundantDSAF++; //*******************************
            ReDSA_flag = 1;  //*******************************
          }

          //*******just for collect the experiment data*********
          if (redundantSSAF.find(curFault[0]) == redundantSSAF.end() && redundantSSAF.find(curFault[1]) == redundantSSAF.end()) {
            count_twoNonReSSA++;
            if (ReDSA_flag == 1) count_twoNonReSSA_ReDSA++;
            else count_twoNonReSSA_NonReDSA++;
          } else if (redundantSSAF.find(curFault[0]) != redundantSSAF.end() && redundantSSAF.find(curFault[1]) != redundantSSAF.end()) {
            count_twoReSSA++;
            if (ReDSA_flag == 1) count_twoReSSA_ReDSA++;
            else {
              count_twoReSSA_NonReDSA++;
            }
          } else {
            count_oneNonReSSA++;
            if (ReDSA_flag == 1) count_oneNonReSSA_ReDSA++;
            else count_oneNonReSSA_NonReDSA++;
          }
          //****************************************************
        }

        //****************************
        cout << "count_RedundantDSAF: " << count_RedundantDSAF << endl;
        cout << "count_NoRedundantDSAF: " << count_NonRedundantDSAF << "\n\n";

        cout << "count_twoNonReSSA: " << count_twoNonReSSA << endl;
        cout << "count_twoNonReSSA_ReDSA: " << count_twoNonReSSA_ReDSA << endl;
        cout << "count_twoNonReSSA_NonReDSA: " << count_twoNonReSSA_NonReDSA << "\n\n";

        cout << "count_oneNonReSSA: " << count_oneNonReSSA << endl;
        cout << "count_oneNonReSSA_ReDSA: " << count_oneNonReSSA_ReDSA << endl;
        cout << "count_oneNonReSSA_NonReDSA: " << count_oneNonReSSA_NonReDSA << "\n\n";

        cout << "count_twoReSSA: " << count_twoReSSA << endl;
        cout << "count_twoReSSA_ReDSA: " << count_twoReSSA_ReDSA << endl;
        cout << "count_twoReSSA_NonReDSA: " << count_twoReSSA_NonReDSA << "\n\n";
        cout << "undetectedDSA: " << undetectedDSA.size() << endl;
        //****************************
      }


      // ------------------get TSA---------------------
      // -- Two Faults One Paths --
      /*
      void findBlockDSADFS_helper(gate *curGate, set<set<int>> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAF, int oriFault) {
        // base case
        if (curGate->gateType == PI || curGate->gateType == constant) {
          return;
        }
        int outInverse = 1 - curGate->outValue;
        // if the change of one of gates' value will results in outStuckat value, then that value is the stuckat fault we want to find
        if (curGate->gateType == aig) {
          int in1Value = curGate->fanin1->outValue;
          int in1Inv = curGate->invIn1;
          int in1Inverse = 1 - in1Value;
          int in2Value = curGate->fanin2->outValue;
          int in2Inverse = 1 - in2Value;
          int in2Inv = curGate->invIn2;
          // find the DSA when 00 0 case happens.
          if (in1Value != in1Inv && in2Value != in2Inv) {
            // for the fault1 in input1, find the fault2 in the faninCons of input2.
            int blockFaultID1 = getFaultID(curGate->gateID, 1, in1Inverse);
            if (faultList.find(blockFaultID1) != faultList.end() || redundantSSAF.find(blockFaultID1) != redundantSSAF.end()) {
              set<int> possibleBlockFaults2;
              // use the function to find the undetected DSA.
              set<int> tp;
              sameFaultCurToPIDFS(curGate->fanin2, possibleBlockFaults2, faultList, redundantSSAF, tp, oriFault);
              for (auto faultID2 : possibleBlockFaults2) {
                set<int> tmp;
                tmp.insert(blockFaultID1);
                tmp.insert(faultID2);
                blockFaultsList.insert(tmp);
              }
            }
            // for the fault2 in input2, find the fault1 in the faninCons of input1.
            int blockFaultID2 = getFaultID(curGate->gateID, 2, in2Inverse);
            if (faultList.find(blockFaultID2) != faultList.end() || redundantSSAF.find(blockFaultID2) != redundantSSAF.end()) {
              set<int> possibleBlockFaults1;
              set<int> tp;
              sameFaultCurToPIDFS(curGate->fanin1, possibleBlockFaults1, faultList, redundantSSAF, tp, oriFault);
              for (auto faultID1 : possibleBlockFaults1) {
                set<int> tmp;
                tmp.insert(blockFaultID2);
                tmp.insert(faultID1);
                blockFaultsList.insert(tmp);
              }
            }
          }
          // go to previous gate
          // case1. if the inversing value will also make the output inversing, that inversing value is the stuck at fault we want to find
          if (curGate->getOutValue(in1Inverse, in2Value) == outInverse) {
            findBlockDSADFS_helper(curGate->fanin1, blockFaultsList, faultList, redundantSSAF, oriFault);
          }
          if (curGate->getOutValue(in1Value, in2Inverse) == outInverse) {
            findBlockDSADFS_helper(curGate->fanin2, blockFaultsList, faultList, redundantSSAF, oriFault);
          }
        // case2. inv, just go to the front gates.
        } else if (curGate->gateType == bufInv) {
          findBlockDSADFS_helper(curGate->fanin1, blockFaultsList, faultList, redundantSSAF, oriFault);
        }
      }
      */
      void findBlockDSADFS_helper(gate *curGate, set<set<int>> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAF, int oriFault) {
        // base case
        if (curGate->gateType == PI || curGate->gateType == constant) {
          return;
        }
        int outInverse = 1 - curGate->outValue;
        // if the change of one of gates' value will results in outStuckat value, then that value is the stuckat fault we want to find
        if (curGate->gateType == aig) {
          int in1Value = curGate->fanin1->outValue;
          int in1Inv = curGate->invIn1;
          int in1Inverse = 1 - in1Value;
          int in2Value = curGate->fanin2->outValue;
          int in2Inverse = 1 - in2Value;
          int in2Inv = curGate->invIn2;
          // find the DSA when 00 0 case happens.
          if (in1Value != in1Inv && in2Value != in2Inv) {
            int blockFaultID1 = getFaultID(curGate->gateID, 1, in1Inverse);
            int blockFaultID2 = getFaultID(curGate->gateID, 2, in2Inverse);
            set<int> possibleBlockFaults1;
            set<int> possibleBlockFaults2;
            if (faultList.find(blockFaultID1) != faultList.end() || redundantSSAF.find(blockFaultID1) != redundantSSAF.end()) {
              possibleBlockFaults1.insert(blockFaultID1);
            }
            if (faultList.find(blockFaultID2) != faultList.end() || redundantSSAF.find(blockFaultID2) != redundantSSAF.end()) {
              possibleBlockFaults2.insert(blockFaultID2);
            }
            set<int> tp;
            sameFaultCurToPIDFS(curGate->fanin1, possibleBlockFaults1, faultList, redundantSSAF, tp, oriFault);
            tp.clear();
            sameFaultCurToPIDFS(curGate->fanin2, possibleBlockFaults2, faultList, redundantSSAF, tp, oriFault);
            for (auto faultID1 : possibleBlockFaults1) {
              for (auto faultID2 : possibleBlockFaults2) {
                // set<set<int>> &blockFaultsList
                set<int> DSA;
                DSA.insert(faultID1);
                DSA.insert(faultID2);
                blockFaultsList.insert(DSA);
              }
            }
          }
          // go to previous gate
          // case1. if the inversing value will also make the output inversing, that inversing value is the stuck at fault we want to find
          if (curGate->getOutValue(in1Inverse, in2Value) == outInverse) {
            findBlockDSADFS_helper(curGate->fanin1, blockFaultsList, faultList, redundantSSAF, oriFault);
          }
          if (curGate->getOutValue(in1Value, in2Inverse) == outInverse) {
            findBlockDSADFS_helper(curGate->fanin2, blockFaultsList, faultList, redundantSSAF, oriFault);
          }
        // case2. inv, just go to the front gates.
        } else if (curGate->gateType == bufInv) {
          findBlockDSADFS_helper(curGate->fanin1, blockFaultsList, faultList, redundantSSAF, oriFault);
        }
      }

      // function: recursively find the SSA faults' path; also try to find the DSA faults that may block it.
      bool findBlockDSADFS(gate *curGate, gate *preGate, set<set<int>> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAFList, int oriFault) {
        // base case
        if (curGate->gateType == PO) {
            return true;
        }
        bool isPath = false;
        for (auto fanout : curGate->fanout) {
          if (fanout->different == true && findBlockDSADFS(fanout, curGate, blockFaultsList, faultList, redundantSSAFList, oriFault) == true) {
            isPath = true;
            if (curGate->gateType == aig) {
              // check side value.
              findBlockDSADFS_helper((curGate->fanin1 == preGate ? curGate->fanin2 : curGate->fanin1), blockFaultsList, faultList, redundantSSAFList, oriFault);
            }
          }
        }
        return isPath;
      }

      int findSSAFsBlocked2(vector<int> &newFaults, vector<int> &testVector, set<set<int>> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAF) {
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

        for (int i = 0; i < POSize; i++) {
          gate *POGate = theCircuit[PISize + gateSize + i];
          // if the faults can be detected in one of the PO, then it's detected.
          if (POGate->different == true) {
            for (auto faultID : newFaults) {
              int gateID = faultID >> 3;
              int port = (faultID >> 1) & 3;
              gate *curGate = theCircuit[gateID];
              gate *preGate = (port == 1) ? curGate->fanin1 : curGate->fanin2;
              findBlockDSADFS(curGate, preGate, blockFaultsList, faultList, redundantSSAF, faultID);
            }
            return 1;
          }
        }
        return 0;
      }

      void findSSABlockedByDSAInOnePath(map<int, set<set<int>>> &SSAToblockDSA) {
	      for (auto iter : SSAFToPatterns) {
          int faultID = iter.first;
	        vector<int> testVector = iter.second;
          vector<int> newFaults;
          newFaults.push_back(faultID);
          set<set<int>> blockFaultsList;
          findSSAFsBlocked2(newFaults, testVector, blockFaultsList, collapsedSSAFList, redundantSSAF);
          if (blockFaultsList.size() > 0) {
             SSAToblockDSA.insert(make_pair(faultID, blockFaultsList));
          }
        }
      }
      // -----

      // -- Two faults two path --
      // map<int, set<int>> potentiallyUndetected;
      // map<SSA, DSA that block the SSA> SSAToblockDSA
      void blockFaultInTwoPath_helper(gate* curGate, gate* preGate, int reconvergentGateID, set<int> &blockFaultList, set<int> &faultList, set<int> &redundantSSAF, int oriFault) {
        if (curGate->gateID == reconvergentGateID || curGate->gateType == PO) return;
        // map<fault, map<gateID, set<the fault affects the side value in that gate>>>
        set<int> tmp =  SSAFToFaultsBlockedInGate[oriFault][curGate->gateID];
        blockFaultList.insert(tmp.begin(), tmp.end());
        for (auto neiGate : curGate->fanout) {
          blockFaultInTwoPath_helper(neiGate, curGate, reconvergentGateID, blockFaultList, faultList, redundantSSAF, oriFault);
        }
      }

      void blockFaultInTwoPaths(int faultID, set<set<int>> &blockFaultsList, set<int> &faultList, set<int> &redundantSSAF) {
        for (auto fanoutGateID : SSAFToPathGates[faultID]) {
          if (twoFanoutGateToReconvGate.find(fanoutGateID) == twoFanoutGateToReconvGate.end()) continue;
          int reconvergentGateID = twoFanoutGateToReconvGate[fanoutGateID];
          gate *curGate = theCircuit[fanoutGateID];
          gate* fanoutGate1 = curGate->fanout[0];
          gate* fanoutGate2 = curGate->fanout[1];
          set<int> blockFaultInPath1;
          set<int> blockFaultInPath2;
          blockFaultInTwoPath_helper(fanoutGate1, curGate, reconvergentGateID, blockFaultInPath1, faultList, redundantSSAF, faultID);
          blockFaultInTwoPath_helper(fanoutGate2, curGate, reconvergentGateID, blockFaultInPath2, faultList, redundantSSAF, faultID);
          // merge two fault list
          for (auto faultID1 : blockFaultInPath1) {
            for (auto faultID2 : blockFaultInPath2) {
              set<int> tmp;
              tmp.insert(faultID1);
              tmp.insert(faultID2);
              blockFaultsList.insert(tmp);
            }
          }
        }
      }

      void findSSABlockedbyDSAInTwoPaths(map<int, set<set<int>>> &SSAToblockDSA) {
        for (auto iter : SSAFToPatterns) {
          int faultID = iter.first;
          vector<int> testVector = iter.second;
          set<set<int>> blockFaultsList;
          blockFaultInTwoPaths(faultID, blockFaultsList, collapsedSSAFList, redundantSSAF);
          SSAToblockDSA.insert(make_pair(faultID, blockFaultsList));
        }
      }
      // -------

      // -- One fault blocked by two faults --
      // map<DSA, blockedSSAList>
      void findDSABlockedbySSA(map<set<int>, set<int>> &DSAToblockSSA) {
        for (auto iter : DSAFToPatterns) {
          set<int> DSA = iter.first;
          vector<int> testVector = iter.second;
          vector<int> newFaults;
          newFaults.assign(DSA.begin(), DSA.end());
          set<int> blockFaultsList;
          // ???collaspsed???
          findSSAFsBlocked(newFaults, testVector, blockFaultsList, collapsedSSAFList, redundantSSAF, 1);
          // ???only check DSA + redundant SSAF.
          // ****may needs reverse order check
          set<int> temp;
          for (auto faultID : blockFaultsList) {
            if (redundantSSAF.find(faultID) != redundantSSAF.end() && DSA.find(faultID) == DSA.end()) {
              temp.insert(faultID);
            }
          }
          DSAToblockSSA.insert(make_pair(DSA, temp));
        }
      }
      // -----

      void findPotentialUndetectedTSAF(map<int, set<set<int>>> &SSAToblockDSA1, map<int, set<set<int>>> &SSAToblockDSA2, map<set<int>, set<int>> &DSAToblockSSA) {
        double curTime, preTime;
        preTime = clock();
        findSSABlockedByDSAInOnePath(SSAToblockDSA1);
        curTime = clock();
        cout << "   SSA + DSA One Path: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        preTime = clock();
        findSSABlockedbyDSAInTwoPaths(SSAToblockDSA2);
        curTime = clock();
        cout << "   SSA + DSA Two Path: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        preTime = clock();
        findDSABlockedbySSA(DSAToblockSSA);
        curTime = clock();
        cout << "   DSA + SSA: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        /*
        int count = 0;
        for (auto iter : SSAToblockDSA1) {
          cout << "falut1: " << iter.first << endl;
          for (auto DSA : iter.second) {
            cout << count++ << ": ";
            for (auto faultID : DSA) {
              cout << faultID << " ";
            }
            cout << endl;
          }
          cout << "\n\n";
        }
        */
        int count1 = 0;
        for (auto iter : SSAToblockDSA1) {
          count1 += iter.second.size();
        }
        int count2 = 0;
        for (auto iter : SSAToblockDSA2) {
          count2 += iter.second.size();
        }
        int count3 = 0;
        for (auto iter : DSAToblockSSA) {
          count3 += iter.second.size();
        }
        cout << "   SSAToblockDSA1.size(): " << count1 << endl;
        cout << "   SSAToblockDSA2.size(): " << SSAToblockDSA2.size() << endl;
        cout << "   DSAToblockSSA.size(): " << count3 << endl;

        /*
        for (auto iter : SSAToblockDSA) {
          int fault1 = iter.first;
          cout << "faultID: " << fault1 << endl;
          for (auto blockFault : iter.second) {
            cout << blockFault << " ";
          }
          cout << "\n\n";
        }
        */
        /*
        int count = 0;
        for (auto iter : DSAToblockSSA) {
          set<int> DSA = iter.first;
          for (auto blockFault : iter.second) {
            for (auto faultID : DSA) {
              cout << faultID << " ";
            }
            cout << "    ";
            cout << blockFault << endl;
            count++;
          }
        }
        cout << "count: " << count << endl;
        */
      }

      void pairPotentialUndetectedTSAF(map<int, set<set<int>>> &SSAToblockDSA1, map<int, set<set<int>>> &SSAToblockDSA2, map<set<int>, set<int>> &DSAToblockSSA, set<set<int>> &undetectedTSA) {
        // merge SSA to block DSA
        for (auto iter : SSAToblockDSA2) {
          int faultID = iter.first;
          set<set<int>> DSASet = iter.second;
          if (SSAToblockDSA1.find(faultID) != SSAToblockDSA1.end()) {
            SSAToblockDSA1[faultID].insert(DSASet.begin(), DSASet.end());
          } else {
            SSAToblockDSA1.insert(make_pair(faultID, DSASet));
          }
        }
        int testCount1 = 0, testCount2 = 0, testCount3 = 0;
        // first check the SSAToblockDSA1 itself
        // f1 -> f2, f3.("->" indicates blocked by)
        // if f2 is not redundant, f2 -> f1, f3.
        // if f3 is not redundant, f3 -> f1, f2.
        for (auto iter : SSAToblockDSA1) {
          int fault1 = iter.first;
          for (auto DSA : iter.second) {
            // 1. check with DSAToblockSSA
            if (DSAToblockSSA.find(DSA) != DSAToblockSSA.end() && DSAToblockSSA[DSA].find(fault1) != DSAToblockSSA[DSA].end()) {
              set<int> tmp;
              tmp.insert(fault1);
              tmp.insert(DSA.begin(), DSA.end());
              undetectedTSA.insert(tmp);
              //***************
              testCount1++;
              //**************
              continue;
            }
            // 2. check in SSAToblockDSA1 self
            int fault2 = 0;
            int fault3 = 0;
            int count = 0;
            for (auto faultID : DSA) {
              if (count == 0) fault2 = faultID;
              else fault3 = faultID;
              count++;
            }
            // check f2 -> f1, f3
            set<int> check;
            check.insert(fault1);
            check.insert(fault3);
            if (redundantSSAF.find(fault2) == redundantSSAF.end() &&
              (SSAToblockDSA1.find(fault2) == SSAToblockDSA1.end() || SSAToblockDSA1[fault2].find(check) == SSAToblockDSA1[fault2].end())) {
              continue;
            }
            // check f3 -> f1, f2
            check.clear();
            check.insert(fault1);
            check.insert(fault2);
            if (redundantSSAF.find(fault3) == redundantSSAF.end() &&
              (SSAToblockDSA1.find(fault3) == SSAToblockDSA1.end() || SSAToblockDSA1[fault3].find(check) == SSAToblockDSA1[fault3].end())) {
              continue;
            }
            set<int> tmp;
            tmp.insert(fault1);
            tmp.insert(DSA.begin(), DSA.end());
            undetectedTSA.insert(tmp);
            //***************
            testCount2++;
            //**************
          }
        }
        // check the DSAToblockSSA
        // {f1, f2} -> f3
        // only check f3 is redundant case
        // since f3 -> {f1, f2} is checked in the previous for loop.
        for (auto iter : DSAToblockSSA) {
          set<int> DSA = iter.first;
          for (auto faultID : iter.second) {
            if (redundantSSAF.find(faultID) != redundantSSAF.end()) {
              set<int> tmp;
              tmp.insert(faultID);
              tmp.insert(DSA.begin(), DSA.end());
              undetectedTSA.insert(tmp);
              //***************
              testCount3++;
              //**************
            }
          }
        }
        cout << "testCount1: " << testCount1 << endl;
        cout << "testCount2: " << testCount2 << endl;
        cout << "testCount3: " << testCount3 << endl;
      }

      void compressundetectedTSA(set<set<int>> &undetectedTSA, vector<vector<int>> &AllPatterns) {
        set<set<int>> finalUndetected;
        for (auto TSA : undetectedTSA) {
          vector<int> newFaults;
          newFaults.assign(TSA.begin(), TSA.end());
          int index = simulate->checkallPatterns(newFaults, AllPatterns);
          if (index < 0) {
            finalUndetected.insert(TSA);
          }
        }
        undetectedTSA.clear();
        undetectedTSA.insert(finalUndetected.begin(), finalUndetected.end());
      }

      void generateTSATest(set<set<int>> &undetectedTSA, vector<vector<int>> &TSAFPatterns, set<set<int>> &redundantTSAFList) {
        for (auto TSA : undetectedTSA) {
          vector<int> newFaults;
          newFaults.assign(TSA.begin(), TSA.end());
          int index = simulate->checkallPatterns(newFaults, TSAFPatterns);
          if (index >= 0) continue;
          vector<int> testVector;
          if (testBySAT->generateTestBySAT_1(newFaults, testVector) == 1) {
            TSAFPatterns.push_back(testVector);
          } else {
            redundantTSAFList.insert(TSA);
          }
        }
      }
      // ---------------------------------------------


      int TSAParser(char *TSAFile, set<set<int>> &TSAs) {
        ifstream file;
        file.open(TSAFile);
        if(!file){
          cout << "Cannot open the file" << endl;
          return 0;
        }
        string line;
        int count = 0;
        int count2 = 0;
        while(!file.eof()) {
          set<int> TSA;
          int count = 0;
          while (count < 3 && !file.eof()) {
            getline(file, line);
            if (line.size() < 20 || line.find("***") != string::npos) continue;
            int faultID = 0;
            for (int i = 0; '0' <= line[i] && line[i] <= '9'; i++) {
              int cur = line[i] - '0';
              faultID = faultID * 10 + cur;
            }
            TSA.insert(faultID);
            count++;
          }
          TSAs.insert(TSA);
        }
        file.close();
        cout << "TSAs.size(): " << TSAs.size() << endl;
        return 1;
      }

      void checkTSAAgain(set<set<int>> &TSAs) {
        vector<int> curFault;
        int count = 0;
        int count2 = 0;
        vector<int> pattern;
        for (auto TSA : TSAs) {
          curFault.clear();
          curFault.assign(TSA.begin(), TSA.end());
          count2++;
          if (simulate->checkallPatterns(curFault, AllPatterns) < 0) {
            count++;
            cout << "\n\ncount: " << count << " " << curFault.size() << endl;
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
        cout << "count2 " << count2 << endl;
      }



      void testDSA_pattern(set<int> &SSAFList, vector<vector<int>> &testVectors) {
          set<set<int>> DSAtmp;
          vector<int> list;
          vector<int> curFaults;
          for (auto elem : SSAFList) {
            list.push_back(elem);
          }
          vector<int> curFault;
          set<int> empty;
          int count = 0;
          vector<int> pattern;
          for (int i = 0; i < list.size() - 1; i++) {
              for (int j = i + 1; j < list.size(); j++) {
                  curFault.clear();
                  curFault.push_back(list[i]);
                  curFault.push_back(list[j]);
                  if(simulate->checkallPatterns(curFault, testVectors) < 0) {
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
                          cout << "   Redundant SSAF******" << endl;
                        }
                      }
                    }
                  }
              }
          }
          int DSARedundantNumber = 0;

          cout << "undetected DSA number: " << count << endl;
      }

      void testTSA_pattern(set<int> &SSAFList, vector<vector<int>> &testVectors) {
        set<set<int>> undetectedTSAbyDSAPT;
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
        for (auto faults : TSAtmp) {
          for (auto fault : faults) {
            cout << fault << " ";
          }
          cout << endl;
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

      void printCircuit2(vector <gate*> &curCircuit) {
        for (int i = 0; i < curCircuit.size(); i++) {
          cout << curCircuit[i]->gateID << " ";
          if (curCircuit[i]->gateType == null) {
            cout << "Big OR gate";
          } else if (curCircuit[i]->gateType == constant) {
            cout << curCircuit[i]->outName << "(inv" << curCircuit[i]->invOut << ") ";
          } else if (curCircuit[i]->gateType == PI) {
            cout << curCircuit[i]->outName;
          } else {
            cout << curCircuit[i]->fanin1->outName << "(inv" << curCircuit[i]->invIn1 << ") ";
            if (curCircuit[i]->gateType == aig || curCircuit[i]->gateType == OR || curCircuit[i]->gateType == XOR) {
              cout << curCircuit[i]->fanin2->outName << "(inv" << curCircuit[i]->invIn2 << ") ";
            }
            cout << curCircuit[i]->outName << "(inv" << curCircuit[i]->invOut <<  ") ";
          }
          // constant, bufInv, aig, PO, PI, OR, XOR
          cout << " "; printGateType(curCircuit[i]->gateType);
          cout << " diff" << curCircuit[i]->different;
          cout << endl;
        }
        cout << endl;
      }

      void printTestVector(vector<int> &testVector) {
        cout << "test pattern: ";
        for (auto i : testVector) {
          cout << i;
        }
        cout << endl;
      }

      void printUndetectedDSA(set<set<int>> &undetectedDSA) {
        cout << "\n\n\nUndetected DSA:" << endl;
        for (auto DSA : undetectedDSA) {
          for (auto faultID : DSA) {
            cout << faultID << " ";
            printFault2(faultID);
            vector<int> curFault;
            curFault.push_back(faultID);
            vector<int> pattern;
            if (testBySAT->generateTestBySAT_1(curFault, pattern) == 0) {
              cout << "******Redundant SSAF******" << endl;
            }
          }
          cout << "\n\n";
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
  };
}

#endif
