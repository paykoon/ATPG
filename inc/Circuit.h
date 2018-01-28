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
        if ( !blifParser(blifFile) )  exit(1);
        if ( !connectGates() )  return;
        if ( !generateFaultList() ) return;
  //      if ( !assignWireNum()) return;
        double endTime = clock();
        cout << "The initialization of the Circuit takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds" << endl;
       // printEntireCircuit();
      }
      ~circuit(){}

      void addGate(gate *newGate){
        theCircuit.push_back(newGate);
          if(newGate->getType() == PI){
              PISize++;
          }
          else if(newGate->getType() == PO){
              POSize++;
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

          if(line.find(".inputs") != string::npos || stillIn){
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
          }

          else if(line.find(".outputs") !=string::npos || stillOut){
            if(line.find("\\") != string::npos){
              stillOut = true;
            }else{
              stillOut = false;
            }
            //store the output line and process them later
            POTemp.push_back(line);
          }

          else if(line.find(".names") != string::npos){
            splitName(line, elems);
            getline(file, line);
            lineNum++;
            splitInverter(line, elems);
            //elems stores the gate name and its inverter
            //the elems[0]== .names
            if(elems.size() == 7){
              gate *newAig = new gate(elems[1], elems[2], elems[3], elems[4], elems[5], elems[6]);
              addGate(newAig);
              MapNumWire.insert(pair<string, int>(newAig->getOutName(), gateIndex++));
            }else if(elems.size() == 5){
              gate *newBufInv = new gate(elems[1], elems[2], elems[3], elems[4]);
              addGate(newBufInv);
              MapNumWire.insert(pair<string, int>(newBufInv->getOutName(), gateIndex++));
            }else if(elems.size() == 3){
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
      //Assume it's n, n/4=gateID, (n%4)/2=gate input ID, (n%4)%2=stuck-at fault,
      int generateFaultList(){
        if(theCircuit.size() == 0)  return 0;

        unsigned long newFault;
        for(unsigned long i = 0;i < theCircuit.size(); i++){
          if(theCircuit[i]->getType() != aig) continue;
          if(theCircuit[i]->getFanin1Type() == PI || theCircuit[i]->getFanin1OutNum() > 1){
            newFault = 4 * i + 0;   //stuck-at 0 at input 0 of gate i
            faultList.insert(newFault);
            newFault = 4 * i + 1;   //stuck-at 1 at input 0 of gate i
            faultList.insert(newFault);
          }
          if(theCircuit[i]->getFanin2Type() == PI || theCircuit[i]->getFanin2OutNum() > 1){
            newFault = 4 * i + 2;   //stuck-at 0 at input 1 of gate i
            faultList.insert(newFault);
            newFault = 4 * i + 3;   //stuck-at 1 at input 1 of gate i
            faultList.insert(newFault);
          }
        }
        return 1;
      }

      //To test-------
      void printFaultList(){
        for(set<unsigned long>::iterator iter = faultList.begin();iter != faultList.end();iter++){
          int gate = *iter >> 2;
          cout << theCircuit[gate]->getIn1Name() << " " << theCircuit[gate]->getIn2Name() << " " << theCircuit[gate]->getOutName() << endl;
          int inputPort = (*iter >> 1) & 0x0001;
          if(inputPort == 0)
            cout << theCircuit[gate]->getIn1Name();
          else
            cout << theCircuit[gate]->getIn2Name();
          int stuckat = *iter & 0x0001;
          cout << " sa" << stuckat << endl; cout << endl;
        }
        cout << "Total possible single fault number is: " << (getSize() - getInSize() - getOutSize())*3 << endl;
        cout << "Total selected single fault number is: " << faultList.size() << endl;
      }

      void printEntireCircuit() {
        for (int i = 0; i < theCircuit.size(); i++) {
          cout << "ID: " << i << " Gate Name: " << theCircuit[i]->outName << endl;
          cout << "In1: " << theCircuit[i]->in1Name << " In2: " << theCircuit[i]->in2Name << endl;
        }
      }

      //void activateFault(int fault);
      //void activateFault(int fault, int );

      void resetAllVisited() {
        for (int i = 0; i < theCircuit.size(); i++) {
          theCircuit[i]->visited = false;
        }
      }

      /*
      void BFSCircuit() {
          resetAllVisited();
          for (int n = 0; n < theCircuit.size(); n++) {
              gate *curGate = theCircuit[n];
              if (curGate->visited == false && curGate->gateType == aig) {
                  queue <gate*> q;
                  q.push(curGate);
                  curGate->visited = true;
                  while (!q.empty()) {
                      unsigned long size = q.size();
                      for (int i = 0; i < size; i++) {
                          curGate = q.front();
                          q.pop();
                          //******process the node here*********
                          cout << curGate->outName << " " << endl;
                          for (int j = 0; j < curGate->fanout.size(); j++) {
                              gate *nextGate = curGate->fanout[j];
                              if (nextGate->visited == false && nextGate->gateType == aig) {
                                  nextGate->visited = true;
                                  q.push(nextGate);
                              }
                          }
                      }
                  }
              }
          }
      }
      */

      /*
      // assign a number to each AIG for the CNF generation
      int assignWireNum() {
        if(theCircuit.size() == 0)  return 0;
        int wireIndex = 0;
        map<string, int>::iterator iter;
        // assign number to PI first in the sequence of blif file's input
        // then assign number to AIG signal
        for (int i = 0; i < theCircuit.size(); i++) {
          gate *curGate = theCircuit[i];
          // PI
          if (curGate->getType() == PI) {
            MapNumWire.insert(pair<string, int>(curGate->getIn1Name(), wireIndex++));
          } else if (curGate->getType() == aig) {
            // check input1
            iter = MapNumWire.find(curGate->getIn1Name());
            if (iter == MapNumWire.end()) {
              MapNumWire.insert(pair<string, int>(curGate->getIn1Name(), wireIndex++));
            }
            // check input2
            iter = MapNumWire.find(curGate->getIn2Name());
            if (iter == MapNumWire.end()) {
              MapNumWire.insert(pair<string, int>(curGate->getIn2Name(), wireIndex++));
            }
            // check output
            iter = MapNumWire.find(curGate->getOutName());
            if (iter == MapNumWire.end()) {
              MapNumWire.insert(pair<string, int>(curGate->getOutName(), wireIndex++));
            }
          }
        }
        //for (iter = MapNumWire.begin(); iter != MapNumWire.end(); iter++) {
        //    int count = iter->second;
        //    string name = iter->first;
        //    cout << "Count: " << count << " Name: " << name << endl;
        //}
        return 1;
      }
      */

      // generate CNF formula and put into glucose container
      int CNFGenerator() {
        if(theCircuit.size() == 0)  return 0;
        vector<vector<int>> CNFFormula;
	      CNFFormula.reserve(theCircuit.size() *3);
        vector<vector<int>> gateClause;
        for (int i = 0; i < theCircuit.size(); i++) {
          gateClause.clear();
          gate *curGate = theCircuit[i];
          if (curGate->gateType == PI) continue;
	        curGate->generateClause(gateClause);
          CNFFormula.insert(CNFFormula.end(), gateClause.begin(), gateClause.end());
       	}
        // decide the output that want to be SAT*****
      	vector<int> output;
      	output.push_back(theCircuit.size() - 1);
      	CNFFormula.push_back(output);
	//
	/*
	for (int i =0; i < CNFFormula.size(); i++) {
          for (int j = 0; j < CNFFormula[i].size(); j++) {
	   cout << CNFFormula[i][j] << " ";
	  }
	  cout << endl;
	}*/

      	glucose *SATSolver = new glucose();
      	vector<int> result;
      	double startTime = clock();
      	if (SATSolver->runGlucose(CNFFormula, result)) {
      	  cout << "SAT" << endl;
      	  for (int i = 0; i < result.size(); i++) {
      	  	cout << result[i] << endl;
      	  }
      	}
      	else {
      	  cout << "UNSAT" << endl;
      	}
        double endTime = clock();
        cout << "The SAT takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds" << endl;
        return 1;
      }


      int SATCircuit() {

      }

      int gitTest() {

      }

      private:
        vector <gate*> theCircuit;
        set<unsigned long> faultList;
        unsigned int PISize,POSize;
        // Key : wire name. Value : number in CNF formula
        map<string, int> MapNumWire;
  };
}

#endif
