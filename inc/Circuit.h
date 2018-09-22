#ifndef _CIRCUIT_H
#define _CIRCUIT_H

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
using namespace std;
using namespace Gate;

// Cirucit class. The initialization process when a circuit is built is included.
namespace Circuit {
  class circuit {
    public:
      int PISize, POSize, gateSize;
      vector <gate*> theCircuit;
      // Key : signal name. Value : corresponding ID
      map<string, int> MapNumWire;
      map<int, vector<uint64_t>> gateToRelatedGates;
      static const int WSIZE = 64;
      vector<gate*> PIGates; // help to assign the level number of each gate.
      vector<gate*> POGates;

      // map<gateID with 2 fanouts, related reconvergent gatesID(-1 if no reconvergent)>
      map<int, int> twoFanoutGateToReconvGate;

      circuit(char *blifFile){
        PISize = 0;
        POSize = 0;
        gateSize = 0;
        CircuitInit(blifFile);
      }
      ~circuit(){}

      void CircuitInit(char *blifFile) {
        double startTime, endTime, preTime, curTime;
        startTime = clock();
        preTime = clock();
        cout << endl << "----------Initialization of circuit----------" << endl;
        if ( !blifParser(blifFile) )  exit(1);
        cout << "   The number of gates in the circuit is " << theCircuit.size() << endl;
        if ( !connectGates() )  return;


        //*********************
        printCircuitVerilog(theCircuit, blifFile);
        //*********************


        cout << "   Pair each gate with it's related gates." << endl;
        preTime = clock();
        pairGateWithRelatedGates();
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        endTime = clock();
        cout << "   Pair gates that has two fanouts with its related reconvergent gate" << endl;
        preTime = clock();
        findTwoFanouts();
        curTime = clock();
        cout << "   Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        cout << "----------Time " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------" << endl << endl;

      }

    private:

      void addGate(gate *newGate){
        theCircuit.push_back(newGate);
          if(newGate->gateType == PI){
            PISize++;
          } else if(newGate->gateType == PO){
            POSize++;
          } else {
            gateSize++;
          }
      }

      //in blif file, all items are seperated by space
      //get the items in one line and store it in elems.
      void splitName(string &line, vector<string> &elems){
        int spaceLength = 0;
        // skip the space in the head of line
        for (auto ch : line) {
            if(ch == ' ') {
                spaceLength++;
            } else {
                break;
            }
        }
        line.erase(0, spaceLength);
        stringstream lineStream(line);//transfer string to stream
        string item;
        while( getline(lineStream, item, ' ') ){
          elems.push_back(item);
        }
      }

      void splitInverter(string &line, vector<string> &elems){
          for(int i = 0;i < line.size();i++){
              if(line[i] != ' '){
                  elems.push_back(line.substr(i,1));
              }
          }
      }

      //can only read the AIG circuit
      //return 0 if there are some error;
      int blifParser(char *blifFile){
        ifstream file;
        file.open(blifFile);
        if(!file){
          cout << "Cannot open the file" << endl;
          return 0;
        }
        int lineNum = 0;
        int gateIndex = 0;
        string line;
        bool stillIn = false, stillOut = false;
        vector<string>POTemp;
        while(!file.eof()){
          getline(file, line);
          lineNum++;
          vector<string> elems;
          if(line.find(".inputs") != string::npos || stillIn){ // PI
            //the first "\" is the escape charater
            if(line.find("\\") != string::npos){ //not the last line
              stillIn = true;
            }else{
              stillIn = false;
            }
            splitName(line,elems);
            for(unsigned int i = 0;i < elems.size();i++){
                if(elems[i] == ".inputs" || elems[i] == "\\") continue;
              gate *newInput = new gate(PI, elems[i]);
              addGate(newInput);
              MapNumWire.insert(pair<string, int>(newInput->outName, gateIndex++));
            }
          } else if(line.find(".outputs") !=string::npos || stillOut){
            if(line.find("\\") != string::npos){
              stillOut = true;
            }else{
              stillOut = false;
            }
            //store the output line and process them later
            POTemp.push_back(line);
          } else if(line.find(".names") != string::npos){
            splitName(line, elems);
            getline(file, line);
            lineNum++;
            splitInverter(line, elems);
            //elems stores the gate name and its inverter
            //the elems[0]== .names
            if(elems.size() == 7){ //AIG
              gate *newAig = new gate(aig, elems[1], elems[2], elems[3], elems[4], elems[5], elems[6]);
              addGate(newAig);
              MapNumWire.insert(pair<string, int>(newAig->outName, gateIndex++));
            }else if(elems.size() == 5){ // buffer or Inverter
              gate *newBufInv = new gate(elems[1], elems[2], elems[3], elems[4]);
              addGate(newBufInv);
              MapNumWire.insert(pair<string, int>(newBufInv->outName, gateIndex++));
            }else if(elems.size() == 3){  // constant
              gate *newCons = new gate(elems[1], elems[2]);
              addGate(newCons);
              MapNumWire.insert(pair<string, int>(newCons->outName, gateIndex++));
            }else{
                cout << "\n***Line "; cout << lineNum; cout << " in blif file has errors***\n" << endl;
              return 0;
            }
          }
        }
        //insert the output node at last
        for(unsigned int i = 0;i < POTemp.size();i++){
          vector <string> elems;
          splitName(POTemp[i],elems);
          for(unsigned int m = 0;m < elems.size(); m++){
              if(elems[m] == ".outputs" || elems[m] == "\\") continue;
            gate *newOutput = new gate(PO, elems[m]);
            addGate(newOutput);
            MapNumWire.insert(pair<string, int>(newOutput->outName, gateIndex++));
          }
        }
        file.close();
        return 1;
      }

      //assign gateID and search by fanin to connect the gates
      int connectGates() {
        if(theCircuit.size() == 0)  return 0;
        map<string, int>::iterator iter;
        for(int cur = 0;cur < theCircuit.size();cur++){
          theCircuit[cur]->setID(cur);
          Type gateType = theCircuit[cur]->gateType;
          if (gateType == PI) PIGates.push_back(theCircuit[cur]);
          else if (gateType == PO) POGates.push_back(theCircuit[cur]);
          if (gateType == constant || gateType == PI) continue;
          iter = MapNumWire.find(theCircuit[cur]->in1Name);
          if (iter == MapNumWire.end()) {
            cout << "The gate is not connected: "; cout << theCircuit[cur]->in1Name << endl;
            return 0;
          }
          theCircuit[iter->second]->addFanout(theCircuit[cur]);
          theCircuit[cur]->setFanin1(theCircuit[iter->second]);
          //If it's AIG, there will be two inputs.
          if(gateType != aig) continue;
          iter = MapNumWire.find(theCircuit[cur]->in2Name);
          if (iter == MapNumWire.end()) {
            cout << "The gate is not connected: "; cout << theCircuit[cur]->in2Name << endl;
            return 0;
          }
          theCircuit[iter->second]->addFanout(theCircuit[cur]);
          theCircuit[cur]->setFanin2(theCircuit[iter->second]);
        }
        return 1;
      }

      // --------pair each gate with its related gates------------
      /*
      void pairGateWithRelatedGates() {
        int size = theCircuit.size();
        for (auto curGate : theCircuit) {
          vector<uint64_t> relatedGates((size - 1) / WSIZE + 1, 0);
          setBit(curGate->gateID, relatedGates);  // the gate itself is also the related gate
          // relatedGates.insert(curGate->gateID);
          findrelatedGatesDFS(curGate, relatedGates);
          gateToRelatedGates.insert(make_pair(curGate->gateID, relatedGates));
          // ********************
          cout << "gateID: " << curGate->gateID << endl;
          for (auto item : relatedGates) printBit(item);
          cout << endl;
          // ********************
        }
      }

      void findrelatedGatesDFS(gate *curGate, vector<uint64_t> &relatedGates) {
        if (curGate->gateType == PO) {
          findrelatedGates_helperDFS(curGate, relatedGates);
          return;
        }
        for (auto nextGate: curGate->fanout) {
          findrelatedGatesDFS(nextGate, relatedGates);
        }
      }

      void findrelatedGates_helperDFS(gate *curGate, vector<uint64_t> &relatedGates) {
        // relatedGates.insert(curGate->gateID);
        setBit(curGate->gateID, relatedGates);
        if (curGate->gateType == PI || curGate->gateType == constant) {
          return;
        }
        findrelatedGates_helperDFS(curGate->fanin1, relatedGates);
        if (curGate->gateType == aig) findrelatedGates_helperDFS(curGate->fanin2, relatedGates);
      }
      */

      void pairGateWithRelatedGates() {
        int size = theCircuit.size();
        map<int, set<int>> levelToGate;
        map<int, set<int>> reverseLevelToGate;
        sortGateByLevel(levelToGate, reverseLevelToGate);
        findInFanins(levelToGate);
        findInPropagationPath(reverseLevelToGate);
        // ********************
        /*
        for (auto iter : gateToRelatedGates) {
          cout << "gateID: " << iter.first << endl;
          for (auto item : iter.second) printBit(item);
          cout << endl;
        }
        */
        // ********************
      }

      // find each level of gate:
      // levelToGate <level, set<gates ID in same level>>
      // reverseLevelToGate <reverseLevel, set<gates Id in same reverseLevel>>
      void sortGateByLevel(map<int, set<int>> &levelToGate, map<int, set<int>> &reverseLevelToGate) {
        for (auto POGate : POGates) {
          assignLevelToGate(POGate, levelToGate);
        }
        for (auto PIGate : PIGates) {
          assignReverseLevelToGate(PIGate, reverseLevelToGate);
        }
      }

      // PI, constant, bufInv, aig, PO
      // search from PO to PI, top down.
      // return the gateLevel (from PI to curGate)
      int assignLevelToGate(gate *curGate, map<int, set<int>> &levelToGate) {
        if (curGate->gateType == PI || curGate->gateType == constant) {
          curGate->level = 0;
          if (levelToGate.find(curGate->level) == levelToGate.end()) {
            set<int> tmp;
            levelToGate.insert(make_pair(curGate->level, tmp));
          }
          levelToGate[curGate->level].insert(curGate->gateID);
          return curGate->level;
        } else if (curGate->level != -1) { // purning
          return curGate->level;
        }

        if (curGate->gateType == bufInv || curGate->gateType == PO) {
          curGate->level = assignLevelToGate(curGate->fanin1, levelToGate) + 1;
        } else { // aig
          curGate->level = max(assignLevelToGate(curGate->fanin1, levelToGate), assignLevelToGate(curGate->fanin2, levelToGate)) + 1;
        }
        if (levelToGate.find(curGate->level) == levelToGate.end()) {
          set<int> tmp;
          levelToGate.insert(make_pair(curGate->level, tmp));
        }
        levelToGate[curGate->level].insert(curGate->gateID);
        return curGate->level;
      }

      // search from PI to PO, top down.
      // return reverseLevel (from PO to curGate)
      int assignReverseLevelToGate(gate *curGate, map<int, set<int>> &reverseLevelToGate) {
        if (curGate->gateType == PO) {
          curGate->reverseLevel = 0;
          if (reverseLevelToGate.find(curGate->reverseLevel) == reverseLevelToGate.end()) {
            set<int> tmp;
            reverseLevelToGate.insert(make_pair(curGate->reverseLevel, tmp));
          }
          reverseLevelToGate[curGate->reverseLevel].insert(curGate->gateID);
          return curGate->reverseLevel;
        } else if (curGate->reverseLevel != -1) {  // pruning.
          return curGate->reverseLevel;
        }

        int maxReverseLevel = -1;
        for (auto nei : curGate->fanout) {
          maxReverseLevel = max(maxReverseLevel, assignReverseLevelToGate(nei, reverseLevelToGate));
        }
        curGate->reverseLevel = maxReverseLevel + 1;
        if (reverseLevelToGate.find(curGate->reverseLevel) == reverseLevelToGate.end()) {
          set<int> tmp;
          reverseLevelToGate.insert(make_pair(curGate->reverseLevel, tmp));
        }
        reverseLevelToGate[curGate->reverseLevel].insert(curGate->gateID);
        return curGate->reverseLevel;
      }

      // get faninCons of each gate level by level.
      // start from PI and constant
      void findInFanins(map<int, set<int>> &levelToGate) {
        int size = theCircuit.size();
        for (auto iter : levelToGate) {
          for (auto gateID : iter.second) {
            gate *curGate = theCircuit[gateID];
            if (curGate->gateType == PI || curGate->gateType == constant) {
              vector<uint64_t> faninCons((size - 1) / WSIZE + 1, 0);
              setBit(curGate->gateID, faninCons);  // the gate itself is also the related gate
              gateToRelatedGates.insert(make_pair(curGate->gateID, faninCons));
            } else {
              // merge the fanin1's faninCons to the current one
              vector<uint64_t> fanin1Cons = gateToRelatedGates[curGate->fanin1->gateID];
              setBit(curGate->gateID, fanin1Cons);
              // if it's aig, merge the fanin2's faninCons to the current one
              if (curGate->gateType == aig) {
                vector<uint64_t> fanin2Cons = gateToRelatedGates[curGate->fanin2->gateID];
                for (int i = 0; i < fanin1Cons.size(); i++) {
                  fanin1Cons[i] |= fanin2Cons[i];
                }
              }
              gateToRelatedGates.insert(make_pair(curGate->gateID, fanin1Cons));
            }
          }
        }
      }

      // Get all the related gates in the propagation paths
      // start from PO, the gate in the left side can merge the results from the fanout in right side.
      void findInPropagationPath(map<int, set<int>> &reverseLevelToGate) {
        int size = theCircuit.size();
        for (auto iter : reverseLevelToGate) {
          for (auto gateID : iter.second) {
            gate *curGate = theCircuit[gateID];
            // if it's PO, related gates is find in findInFanins process.
            if (curGate->gateType != PO) {
              for (auto nei : curGate->fanout) {
                for (int i = 0; i < gateToRelatedGates[0].size(); i++) {
                  gateToRelatedGates[curGate->gateID][i] |= gateToRelatedGates[nei->gateID][i];
                }
              }
            }
          }
        }
      }

      void printBit(uint64_t number) {
        for (uint64_t i = 0; i < 64; i++) {
          if (((1ULL << i) & number) > 0) cout << i << " ";
        }
      }

      void setBit(int pos, vector<uint64_t> &vec) {
        int w = pos / WSIZE;
        int b = pos % WSIZE;
        uint64_t mask = 1ULL << b;
        vec[w] |= mask;
      }
      // ----------------------------------------------------------

      // --------find the gate that has two fanouts, and pair it with the related reconvergent gate------------
      void findTwoFanouts() {
        for (auto curGate : theCircuit) {
          if (curGate->fanout.size() == 2) {
            int reconvergentID = -1;
            gate *fanoutGate1 = curGate->fanout[0];
            gate *fanoutGate2 = curGate->fanout[1];
            for (auto nei2Gate : fanoutGate1->fanout) {
              reconvergentID = checkInPath1(nei2Gate, fanoutGate2);
              if (reconvergentID >= 0) break;
            }
            twoFanoutGateToReconvGate.insert(make_pair(curGate->gateID, reconvergentID));
          }
        }
      }

      // return the reconvergent gateID
      // return -1 if on reconvergence
      int checkInPath1(gate* curGate, gate* fanoutGate2) {
        if (curGate->gateType == PO) return -1;
        int reconvergentID = checkInPath2(fanoutGate2, curGate);
        if (reconvergentID >= 0) return reconvergentID;
        for (auto neiGate : curGate->fanout) {
          reconvergentID = checkInPath1(neiGate, fanoutGate2);
          if (reconvergentID >= 0) break;
        }
        return reconvergentID;
      }

      int checkInPath2(gate* curGate, gate* waitCheckGate) {
        if (curGate->gateType == PO) return -1;
        if (curGate == waitCheckGate) return curGate->gateID;
        int reconvergentID = -1;
        for (auto neiGate: curGate->fanout) {
          reconvergentID = checkInPath2(neiGate, waitCheckGate);
          if (reconvergentID >= 0) break;
        }
        return reconvergentID;
      }
      //------------------------------------------------------------------

      //-------------------print the file for tetramax-----------------------
      void printCircuitVerilog(vector <gate*> &curCircuit, char *blifFile) {
        string fileName = blifFile;
        int dot = fileName.find(".");
        fileName.erase(dot, fileName.size() - dot);
        string circuitName = fileName;
        fileName.append("tmaxTest/dataFile/test.v");

        ofstream myfile;
        myfile.open (fileName);
        int count = 0;
        myfile << "module " << "test" << "(";
        for (int i = 0; i < curCircuit.size(); i++) {
            if (curCircuit[i]->gateType == PI || curCircuit[i]->gateType == PO) {

                int flag = 0;
                if (curCircuit[i]->gateType == PO) {
                    for (int k = 0; k < curCircuit.size(); k++) {
                        if (curCircuit[k]->gateType == PI && curCircuit[k]->outName == curCircuit[i]->outName)
                            flag = 1;
                    }
                }
                if(flag == 1) continue;
                myfile << curCircuit[i]->outName;
                if (i + 1 >= curCircuit.size()) continue;
                myfile << ", ";

                if (count == 10) {
                  count = 0;
                  myfile << endl;
                } else {
                  count++;
                }

            }
        }
        myfile << ");";

        myfile << "\n\n\n\n\n";

        count = 0;
        myfile << "input ";
        for (int i = 0; i < curCircuit.size(); i++) {
            if (curCircuit[i]->gateType == PI) {
                myfile << curCircuit[i]->outName;
                if (i + 1 < curCircuit.size() && curCircuit[i + 1]->gateType == PI) myfile << ", ";
                else myfile << ";";

                if (count == 10) {
                  count = 0;
                  myfile << endl;
                } else {
                  count++;
                }

            }
        }

        myfile << "\n\n\n\n\n";

        count = 0;
        myfile << "output ";
        for (int i = 0; i < curCircuit.size(); i++) {
            if (curCircuit[i]->gateType == PO) {
                int flag = 0;
                for (int k = 0; k < curCircuit.size(); k++) {
                    if (curCircuit[k]->gateType == PI && curCircuit[i]->outName == curCircuit[k]->outName)
                        flag = 1;
                }
                if (flag == 1) continue;
                myfile << curCircuit[i]->outName;
                if (i + 1 < curCircuit.size()) myfile << ", ";
                else myfile << "; ";

                if (count == 10) {
                  count = 0;
                  myfile << endl;
                } else {
                  count++;
                }

            }
        }

        myfile << "\n\n\n\n\n";

        count = 0;
        myfile << "wire ";
        for (int i = 0; i < curCircuit.size(); i++) {
            if (curCircuit[i]->gateType != PO && curCircuit[i]->gateType != PI) {
                myfile << curCircuit[i]->outName;
                if (i + 1 < curCircuit.size() && curCircuit[i + 1]->gateType != PO) myfile << ", ";
                else myfile << ";";

                if (count == 10) {
                  count = 0;
                  myfile << endl;
                } else {
                  count++;
                }

            }
        }

        myfile << "\n\n\n\n\n";

        int num = 1;

        //***************
        //myfile << "wire high, low;" << endl;
        //myfile << "assign high = 1;" << endl;
        //myfile << "assign low = 0;" << endl;
        //*****************

        for (auto curGate : curCircuit) {
            if (curGate->gateType == null)  continue;
            //if (curGate->in1Name.compare(curGate->outName) == 0)  continue;

            if (curGate->gateType == aig) {
                myfile << "N1 g" << num << "(";
                if (curGate->invOut == 0) myfile << "~";
                myfile << curGate->outName << ", ";
                if (curGate->invIn1 == 0) myfile << "~";
                myfile << curGate->fanin1->outName << ", ";
                if (curGate->invIn2 == 0) myfile << "~";
                myfile << curGate->fanin2->outName << ");" << endl;
            } else if (curGate->gateType == bufInv) {
                myfile << "N1 g" << num << "(";
                if (curGate->invOut == 0) myfile << "~";
                myfile << curGate->outName << ", ";
                if (curGate->invIn1 == 0) myfile << "~";
                myfile << curGate->fanin1->outName << ", ";
                myfile << "1);" << endl;
            } else if (curGate->gateType == constant) {
                myfile << "N1 g" << num << "(";
                myfile << curGate->outName << ", ";
                myfile << curGate->outValue << ", ";
                myfile << "1);" << endl;
            }
            num++;

            /*
            if (curGate->gateType == aig) {
                myfile << "assign ";
                myfile << curGate->outName << " = ";
                if (curGate->invOut == 0) myfile << "~";
                myfile << "(";
                if (curGate->invIn1 == 0) myfile << "~";
                myfile << curGate->fanin1->outName << " & ";
                if (curGate->invIn2 == 0) myfile << "~";
                myfile << curGate->fanin2->outName;
                myfile << ");" << endl;
            } else if (curGate->gateType == bufInv) {
                myfile << "assign ";
                myfile << curGate->outName << " = ";
                if (curGate->invOut == 0) myfile << "~";
                myfile << "(";
                if (curGate->invIn1 == 0) myfile << "~";
                myfile << curGate->fanin1->outName << " & ";
                myfile << "high";
                myfile << ");"  << endl;
            } else if (curGate->gateType == constant) {
                myfile << "assign ";
                myfile << curGate->outName << " = ";
                myfile << "(";
                if(curGate->outValue) myfile << "high";
                else myfile << "low";
                myfile << " & ";
                myfile << "high";
                myfile << ");" << endl;
            }
            */
        }

        myfile << "\n\n\n\n\n";

        myfile << "endmodule" << endl;
        myfile.close();
      }

      int aigCase(gate *curGate) {
          if (curGate->gateType != aig) return 0;
          if (curGate->invIn1 == 1 && curGate->invIn2 == 1 && curGate->invOut == 1) {
              return 1;
          } else if (curGate->invIn1 == 1 && curGate->invIn2 == 0 && curGate->invOut == 1) {
              return 2;
          } else if (curGate->invIn1 == 0 && curGate->invIn2 == 1 && curGate->invOut == 1) {
              return 3;
          } else if (curGate->invIn1 == 1 && curGate->invIn2 == 1 && curGate->invOut == 0) {
              return 4;
          } else if (curGate->invIn1 == 1 && curGate->invIn2 == 0 && curGate->invOut == 0) {
              return 5;
          } else if (curGate->invIn1 == 0 && curGate->invIn2 == 1 && curGate->invOut == 0) {
              return 6;
          } else if (curGate->invIn1 == 0 && curGate->invIn2 == 0 && curGate->invOut == 1) {
              return 7;
          } else { //if (curGate->invIn1 == 0 && curGate->invIn2 == 0 && curGate->invOut == 0) {
              return 8;
          }
      }

      int bufCase(gate *curGate) {
          if (curGate->gateType != bufInv) return 0;
          if (curGate->invIn1 == 1 && curGate->invOut == 1) {
              return 9;
          } else if (curGate->invIn1 == 1 && curGate->invOut == 0) {
              return 10;
          } else if (curGate->invIn1 == 0 && curGate->invOut == 1) {
              return 10;
          } else { // if (curGate->invIn1 == 1 && curGate->invOut == 0) {
              return 9;
          }
      }
  };
}

#endif
