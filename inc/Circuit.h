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

      circuit(char *blifFile){
        PISize = 0;
        POSize = 0;
        gateSize = 0;
        CircuitInit(blifFile);
      }
      ~circuit(){
        delete this;
      }

      void CircuitInit(char *blifFile) {
        double startTime, endTime, preTime, curTime;
        startTime = clock();
        preTime = clock();
        cout << endl << "----------Initialization of circuit----------" << endl;

        cout << "1. Blif parsing is started. " << endl;
        if ( !blifParser(blifFile) )  exit(1);
        curTime = clock();
        cout << "   Blif parsing is completed. Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;
        cout << "   The number of gates in the circuit is " << theCircuit.size() << endl;
        
        cout << "2. Gate connecting is started." << endl;
        preTime = clock();
        if ( !connectGates() )  return;
        curTime = clock();
        cout << "   Gate connecting is completed. Time: " << (curTime - preTime)/CLOCKS_PER_SEC << " seconds." << endl;

        endTime = clock();
        cout << "----------The initialization of the Circuit takes " << (endTime - startTime)/CLOCKS_PER_SEC << " seconds----------" << endl << endl;
      }

      void resetAllVisited() {
        for (int i = 0; i < theCircuit.size(); i++) {
          theCircuit[i]->visited = false;
        }
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
      int connectGates(){
        if(theCircuit.size() == 0)  return 0;
        map<string, int>::iterator iter;
        for(int cur = 0;cur < theCircuit.size();cur++){
          theCircuit[cur]->setID(cur);
          Type gateType = theCircuit[cur]->gateType;
          if(gateType == constant || gateType == PI) continue;

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


  };
}

#endif
