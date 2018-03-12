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
using namespace Glucose;

namespace Circuit {
  class circuit {
    public:
      circuit(char *blifFile){
        double startTime = clock();
        PISize = 0;
        POSize = 0;
        gateSize = 0;
        copyCount = 1;
        if ( !blifParser(blifFile) )  exit(1);
        if ( !connectGates() )  return;
        if ( !generateFaultList() ) return;

        vector<gate*> oriAndFauCir;
        generateOriAndFau(oriAndFauCir);

        printCircuit(theCircuit);
        printCircuit(oriAndFauCir);

        generateCNF(oriAndFauCir);

        printCNF(CNFOriAndFauCir);


        double endTime = clock();
        cout << "The initialization of the Circuit takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds" << endl;

        vector<int> fault;
        fault.push_back(36);
        injectFaultsInCNF(fault);
        vector<int> result;
        if (SATCircuit(CNFOriAndFauCir, result)) {
          cout << "SAT" << endl;
          for (int i = 0; i < result.size(); i++) {
            cout << result[i] << " ";
          }
          cout << endl;
        } else {
          cout << "UNSAT" << endl;
        }
      }

      ~circuit(){}

      void addGate(gate *newGate){
        theCircuit.push_back(newGate);
          if(newGate->getType() == PI){
            PISize++;
          } else if(newGate->getType() == PO){
            POSize++;
          } else {
            gateSize++;
          }
      }

      unsigned int getInSize(){
        return PISize;
      }

      unsigned int getOutSize(){
        return POSize;
      }

      unsigned long getSize(){
        return theCircuit.size();
      }

      int assignPIs(string &inValues){
        if(inValues.size()!=PISize){
          cout << "\n***Input vector does not match the size of PI***\n" << endl;
          return 0;
        }
        for(int i=0;i<PISize;i++){
            char oneBit = inValues[i];
            theCircuit[i]->setPI(oneBit);
        }
        return 1;
      }

      void propagatePI(){
        for(unsigned long i=0;i<theCircuit.size();i++){
          theCircuit[i]->setOut();
        }
      }

      //in blif file, all items are seperated by space
      //get the items in one line and store it in elems.
      void splitName(string &line, vector<string> &elems){
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
        unsigned long lineNum = 0;
        unsigned long gateIndex = 0;
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
              MapNumWire.insert(pair<string, int>(newInput->getOutName(), gateIndex++));
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
              MapNumWire.insert(pair<string, int>(newAig->getOutName(), gateIndex++));
            }else if(elems.size() == 5){ // buffer or Inverter
              gate *newBufInv = new gate(elems[1], elems[2], elems[3], elems[4]);
              addGate(newBufInv);
              MapNumWire.insert(pair<string, int>(newBufInv->getOutName(), gateIndex++));
            }else if(elems.size() == 3){  // constant
              gate *newCons = new gate(elems[1], elems[2]);
              addGate(newCons);
              MapNumWire.insert(pair<string, int>(newCons->getOutName(), gateIndex++));
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
            MapNumWire.insert(pair<string, int>(newOutput->getOutName(), gateIndex++));
          }
        }
        file.close();
        return 1;
      }

      //assign gateID and search by fanin to connect the gates
      int connectGates(){
        if(theCircuit.size() == 0)  return 0;
        map<string, int>::iterator iter;
        for(unsigned long cur = 0;cur < theCircuit.size();cur++){
          theCircuit[cur]->setID(cur);
          Type gateType = theCircuit[cur]->getType();
          if(gateType == constant || gateType == PI) continue;

          iter = MapNumWire.find(theCircuit[cur]->getIn1Name());
          if (iter == MapNumWire.end()) {
            cout << "The gate is not connected: "; cout << theCircuit[cur]->getIn1Name() << endl;
            return 0;
          }
          theCircuit[iter->second]->addFanout(theCircuit[cur]);
          theCircuit[cur]->setFanin1(theCircuit[iter->second]);
          //If it's AIG, there will be two inputs.
          if(gateType != aig) continue;
          iter = MapNumWire.find(theCircuit[cur]->getIn2Name());
          if (iter == MapNumWire.end()) {
            cout << "The gate is not connected: "; cout << theCircuit[cur]->getIn2Name() << endl;
            return 0;
          }
          theCircuit[iter->second]->addFanout(theCircuit[cur]);
          theCircuit[cur]->setFanin2(theCircuit[iter->second]);
        }
	      cout << "The number of gates in the circuit is " << theCircuit.size() << endl;
        return 1;
      }

      //return 0 if nothing is inside the circuit
      //generate the faults that locate at aig input wire connecting to the PI or fanout.
      //fault number's meaning:
      // bit index:     2        1       0
      //                gateID   input   stuckat
      //Assume it's n, n/4=gateID, (n%4)/2=gate input ID, (n%4)%2=stuck-at fault,
      // also generate all faults(without collapsing)
      int generateFaultList(){
        if(theCircuit.size() == 0)  return 0;
        unsigned long newFault;
        for(unsigned long i = 0;i < theCircuit.size(); i++){
          if(theCircuit[i]->getType() != aig) continue;
          // the faults in the AIG input which are connected to PI or fanouts
          if(theCircuit[i]->getFanin1Type() == PI || theCircuit[i]->getFanin1OutNum() > 1){
            newFault = 4 * i + 0;   //stuck-at 0 at input 0 of gate i
            collapsedFaultList.insert(newFault);
            newFault = 4 * i + 1;   //stuck-at 1 at input 0 of gate i
            collapsedFaultList.insert(newFault);
          }
          if(theCircuit[i]->getFanin2Type() == PI || theCircuit[i]->getFanin2OutNum() > 1){
            newFault = 4 * i + 2;   //stuck-at 0 at input 1 of gate i
            collapsedFaultList.insert(newFault);
            newFault = 4 * i + 3;   //stuck-at 1 at input 1 of gate i
            collapsedFaultList.insert(newFault);
          }
          // all faults(in input)
          newFault = 4 * i + 0;
          allFaultList.insert(newFault);
          newFault = 4 * i + 1;
          allFaultList.insert(newFault);
          newFault = 4 * i + 2;
          allFaultList.insert(newFault);
          newFault = 4 * i + 3;
          allFaultList.insert(newFault);
        }
        return 1;
      }

      //void activateFault(int fault);
      //void activateFault(int fault, int );

      void resetAllVisited() {
        for (int i = 0; i < theCircuit.size(); i++) {
          theCircuit[i]->visited = false;
        }
      }

      void copyCircuit(vector <gate*> &copy) {
        for (int i = 0; i < theCircuit.size(); i++) {
          // new a gate, then change its information
          string name = "new";
          gate *newGate = new gate(PI, name);
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
          originalInput->fanin1 = newInput;
          originalInput->gateType = bufInv;
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
      }

      // insert stuck-at faults into CNF formula oriAndFauCir. Can be used for any number of faults
      // CNF formula:
      // original circuit | faulty circuit | new input | new XOR | new output | an "OR" gate for all outputs | constant wire(stuck at faults)
      // circuit size        circuit size    PI size      PO size   PO size           1
      int injectFaultsInCNF(vector<int> &newFaults) {
        int origialSize = theCircuit.size();
        if (origialSize == 0) return 0;
        int preSize = origialSize + origialSize + PISize + POSize + POSize + 1;
        // reset previous faults.
        for (int i = 0; i < curFaults.size(); i++) {
          int faultID = curFaults[i];
          int oriGateID = (faultID >> 2);
          int fauGateID = oriGateID + origialSize;
          for (int m = 0; m < CNFOriAndFauCir[fauGateID].size(); m++) {
            for (int n = 0; n < CNFOriAndFauCir[fauGateID][m].size(); n++) {
              int sign = (CNFOriAndFauCir[fauGateID][m][n] > 0) ? 1 : -1;
              CNFOriAndFauCir[fauGateID][m][n] = sign * (abs(CNFOriAndFauCir[oriGateID][m][n]) + origialSize);
            }
          }
          // clear stuck at constant wire locating at last
          CNFOriAndFauCir[preSize+i].clear();
        }
        curFaults.clear();
        curFaults.reserve(newFaults.size());
        curFaults.assign(newFaults.begin(), newFaults.end());
        vector<vector<int>> gateClause;
        // insert new faults
        for (int i = 0; i < newFaults.size(); i++) {
          // bit index:     2        1       0
          //                gateID   input   stuckat
          int faultID = newFaults[i];
          int oriGateID = (faultID >> 2);
          int fauGateID = oriGateID + origialSize;
          int input = (faultID >> 1) & 1;
          int stuckat = faultID & 1;
          // contant wire connected to the stuckat inputs
          // name by stuckat+faultID
          string name = "stuckat_"+to_string(faultID);
          string stuckatStr = to_string(stuckat);
          gate *stuckatCons = new gate(name, stuckatStr);
          stuckatCons->gateID = preSize + i;
          stuckatCons->generateClause(gateClause);
          // in the previous operation, we just clear stuckat constant wire's vector but not delete them (to reduce time complexity)
          // so here we can reuse these vector
          if (preSize + i < CNFOriAndFauCir.size()) {
            CNFOriAndFauCir[preSize + i].assign(gateClause.begin(), gateClause.end());
          } else {
            CNFOriAndFauCir.push_back(gateClause);
          }
          gateClause.clear();
          // change the fanin value of stuck at gates(connect to constant wire)
          // we dont care the origial fanin's output connection
          // directly change the value in clause of faulty gates
          gate *oriGate = theCircuit[oriGateID];
          int faultyFaninID = 0;
          if (oriGate->gateType == PI) {  // PI doesnt have fanin
            faultyFaninID = oriGate->gateID + origialSize;
          } else {
            faultyFaninID = ((input == 0) ? oriGate->fanin1->gateID : oriGate->fanin2->gateID) + origialSize;
          }
          for (int m = 0; m < CNFOriAndFauCir[fauGateID].size(); m++) {
            for (int n = 0; n < CNFOriAndFauCir[fauGateID][m].size(); n++) {
              // find the faulty fanin wire in CNF formula
              if (abs(CNFOriAndFauCir[fauGateID][m][n]) - 1 == faultyFaninID) {
                // connect it with stuckat wire
                CNFOriAndFauCir[fauGateID][m][n] = (CNFOriAndFauCir[fauGateID][m][n] > 0) ? (stuckatCons->gateID + 1) : (stuckatCons->gateID + 1)*(-1);
              }
            }
          }
        }
        return 1;
      }

      // send the entire CNFFormula to GLucose to do SAT.
      int SATCircuit(vector<vector<vector<int>>> &CNFOriAndFauCir, vector<int> &result) {
        if (CNFOriAndFauCir.size() == 0) return 0;
        glucose *SATSolver = new glucose();
        double startTime = clock();
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
        }
        else {
          //cout << "UNSAT" << endl;
        }
        double endTime = clock();
        cout << "The SAT takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds" << endl;
        return 1;
      }



      int printFault(int ID) {
        int faultID = ID;                         cout << "faultID: " << faultID << endl;
        int oriGateID = (faultID >> 2);           cout << "oriGateID: " << oriGateID << endl;
        int fauGateID = oriGateID + theCircuit.size();  cout << "fauGateID: " << fauGateID << endl;
        int input = (faultID >> 1) & 1;           cout << "input: " << input << endl;
        int stuckat = faultID & 1;                cout << "stuckat: " << stuckat << endl;
      }

      void printCircuit(vector <gate*> &curCircuit) {
        for (int i = 0; i < curCircuit.size(); i++) {
          cout << i << " " << curCircuit[i]->outName << " ";
          // constant, bufInv, aig, PO, PI, OR, XOR
          switch (curCircuit[i]->gateType){
            case constant:
              cout << "constant" << endl;
              break;
            case bufInv:
              cout << "bufInv" << endl;
              break;
            case aig:
              cout << "aig"<< endl;
              break;
            case PO:
              cout << "PO" << endl;
              break;
            case PI:
              cout << "PI"<< endl;
              break;
            case OR:
              cout << "OR"<< endl;
              break;
            case XOR:
              cout << "XOR"<< endl;
              break;
          }
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

      private:
        int copyCount;
        vector <gate*> theCircuit;
        unsigned int PISize,POSize, gateSize;
        set<unsigned long> collapsedFaultList;
        set<unsigned long> allFaultList;
        vector<vector<vector<int>>> CNFOriAndFauCir;
        vector<int> curFaults;
        // Key : wire name. Value : number in CNF formula
        map<string, int> MapNumWire;
  };
}

#endif
