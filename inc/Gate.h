#ifndef _GATE_H
#define _GATE_H

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
using namespace std;

enum Type {PI, constant, bufInv, aig, PO, OR, XOR, null};

namespace Gate {
  class gate{
    public:
      Type gateType;
      string in1Name,in2Name,outName;
      bool invIn1,invIn2,invOut;
      bool isPath;
      int gateID;
      vector <gate*> fanout;
      gate *fanin1,*fanin2;
      bool outValue;
      //mark whether the fault is checked or not(both sa0 and sa1)
      //only for aig.
      bool in1Checked, in2Checked;
      bool visited;
      bool different;

      gate(){
        this->gateType = null;
      }
      // PI or PO (The name of In and out are same)
      gate(Type gateType, string &portName){
        this->gateType = gateType;
        this->isPath = false;
        this->in1Name = portName;
        this->outName = portName;
        this->invIn1 = 1;
        this->invOut = 1;
      }
      // constant (The name of In and out are same)
      gate(string &wireName, string &value){
        this->gateType = constant;
        this->isPath = false;
        this->in1Name = wireName;
        this->outName = wireName;
        this->invIn1 = 1;
        this->invOut = 1;
        this->outValue = (value == "1");
      }
      // buffer or Inverter
      gate(string &in1Name, string &outName, string &invIn1, string &invOut){
          this->gateType = bufInv;
          this->in1Name = in1Name;
          this->outName = outName;
          this->invIn1 = (invIn1 == "1");
          this->invOut = (invOut == "1");
          this->isPath = false;
      }
      // AIG or OR gate or XOR gate (XOR and OR gate are just for CNF generation and SAT calculation)
      gate(Type gateType, string &in1Name, string &in2Name, string &outName, string &invIn1, string &invIn2, string &invOut){
          this->gateType = gateType;
          this->in1Name = in1Name;
          this->in2Name = in2Name;
          this->outName = outName;
          this->invIn1 = (invIn1 == "1");
          this->invIn2 = (invIn2 == "1");
          this->invOut = (invOut == "1");
          this->isPath = false;
          in1Checked = false;
          in2Checked = false;
      }

      ~gate(){
        // delete this;
      }

      // given the input value, get the output value of current gate.
      int getOutValue(int in1, int in2){
        int outValue = 0;
        if (gateType == PO) {
          outValue = in1;
        } else if (gateType == bufInv){
          outValue = ( in1 == invIn1 ) == invOut;
        } else if (gateType == aig){
          outValue = ( (in1 == invIn1) & (in2== invIn2) ) == invOut;
        }
        return outValue;
      }

      void setPI(int inValues){
        if (gateType == PI) {
          outValue = inValues;
        }
      }

      void setOut(){
        // the outValue of PI and constant will be decided in other places
        if (gateType == PO) {
          outValue = fanin1->outValue;
        } else if (gateType == bufInv){
          outValue = ( fanin1->outValue == invIn1 ) == invOut;
        } else if (gateType == aig){
          outValue = ( (fanin1->outValue == invIn1) & (fanin2->outValue == invIn2) ) == invOut;
        }
      }

      void setID(int gateID){
        this->gateID = gateID;
      }

      void addFanout(gate* fanout){
        this->fanout.push_back(fanout);
      }

      void setFanin1(gate *fanin1){
        this->fanin1 = fanin1;
      }

      void setFanin2(gate *fanin2){
        this->fanin2 = fanin2;
      }

      void copyGate(gate *copy) {
        copy->gateType = this->gateType;
        copy->in1Name = this->in1Name;
        copy->in2Name = this->in2Name;
        copy->outName = this->outName;
        copy->invIn1 = this->invIn1;
        copy->invIn2 = this->invIn2;
        copy->invOut = this->invOut;
        copy->isPath = this->isPath;
        copy->gateID = this->gateID;
        copy->outValue = this->outValue;
        copy->fanout.clear();
        copy->fanout.assign(this->fanout.begin(), this->fanout.end());
        copy->fanin1 = nullptr;
        copy->fanin2 = nullptr;
        copy->fanin1 = this->fanin1;
        copy->fanin2 = this->fanin2;
      }

      void changeToBuf(){
          this->gateType = bufInv;
          this->invIn1 = 1;
          this->invOut = 1;
      }
  };
}



#endif
