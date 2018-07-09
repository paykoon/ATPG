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
      bool outValue;
      uint64_t invIn1_64,invIn2_64,invOut_64;
      uint64_t outValue_64;
      static const uint64_t one_64 = 0xffffffffffffffff;
      int gateID;
      vector <gate*> fanout;
      gate *fanin1,*fanin2;
      bool different;
      bool isPath;

      gate(){
        this->gateType = null;
      }
      // PI or PO (The name of In and out are same)
      gate(Type gateType, string &portName){
        this->gateType = gateType;
        this->isPath = false;
        this->different = false;
        this->in1Name = portName;
        this->outName = portName;
        this->invIn1 = 1;
        this->invOut = 1;
        this->invIn1_64 = one_64;
        this->invOut_64 = one_64;
      }
      // constant (The name of In and out are same)
      gate(string &wireName, string &value){
        this->gateType = constant;
        this->isPath = false;
        this->different = false;
        this->in1Name = wireName;
        this->outName = wireName;
        this->invIn1 = 1;
        this->invOut = 1;
        this->outValue = (value == "1") ? 1 : 0;
        this->invIn1_64 = one_64;
        this->invOut_64 = one_64;
        this->outValue_64 = (value == "1") ? one_64 : 0;
      }
      // buffer or Inverter
      gate(string &in1Name, string &outName, string &invIn1, string &invOut){
          this->gateType = bufInv;
          this->isPath = false;
          this->different = false;
          this->in1Name = in1Name;
          this->outName = outName;
          this->invIn1 = (invIn1 == "1");
          this->invOut = (invOut == "1");
          this->invIn1_64 = (invIn1 == "1") ? one_64 : 0;
          this->invOut_64 = (invOut == "1") ? one_64 : 0;
      }
      // AIG or OR gate or XOR gate (XOR and OR gate are just for CNF generation and SAT calculation)
      gate(Type gateType, string &in1Name, string &in2Name, string &outName, string &invIn1, string &invIn2, string &invOut){
          this->gateType = gateType;
          this->isPath = false;
          this->different = false;
          this->in1Name = in1Name;
          this->in2Name = in2Name;
          this->outName = outName;
          this->invIn1 = (invIn1 == "1");
          this->invIn2 = (invIn2 == "1");
          this->invOut = (invOut == "1");
          this->invIn1_64 = (invIn1 == "1") ? one_64 : 0;
          this->invIn2_64 = (invIn2 == "1") ? one_64 : 0;
          this->invOut_64 = (invOut == "1") ? one_64 : 0;
      }

      ~gate(){
        // delete this;
      }

      // given the input value, get the output value of current gate.
      int getOutValue(bool in1, bool in2){
        int outValue = 0;
        if (gateType == PO) {
          outValue = in1;
        } else if (gateType == bufInv){
          outValue = ~( ~(in1 ^ invIn1) ^ invOut);
        } else if (gateType == aig){
          outValue = ~( ( ~(in1 ^ invIn1) & ~(in2 ^ invIn2) ) ^ invOut);
        }
        return outValue;
      }

      // given the input value, get the output value of current gate.
      uint64_t getOutValue_64(uint64_t in1_64, uint64_t in2_64){
        uint64_t outValue_64 = 0;
        if (gateType == PO) {
          outValue_64 = in1_64;
        } else if (gateType == bufInv){
          outValue_64 = ~( ~(in1_64 ^ invIn1_64) ^ invOut_64);
        } else if (gateType == aig){
          outValue_64 = ~( ( ~(in1_64 ^ invIn1_64) & ~(in2_64 ^ invIn2_64) ) ^ invOut_64);
        }
        return outValue_64;
      }

      void setPI(bool inValues){
        if (gateType == PI) {
          outValue = inValues;
        }
      }

      void setPI_64(uint64_t inValues_64){
        if (gateType == PI) {
          outValue_64 = inValues_64;
        }
      }

      void setOut(){
        // the outValue of PI and constant will be assigned in setPI and initialization of constant.
        if (gateType == PO) {
          outValue = fanin1->outValue;
        } else if (gateType == bufInv){
          outValue = ~( ~(fanin1->outValue ^ invIn1) ^ invOut);
        } else if (gateType == aig){
          outValue = ~( ( ~(fanin1->outValue ^ invIn1) & ~(fanin2->outValue ^ invIn2) ) ^ invOut);
        }
      }

      void setOut_64(){
        if (gateType == PO) {
          outValue_64 = fanin1->outValue_64;
        } else if (gateType == bufInv){
          outValue_64 = ~( ~(fanin1->outValue_64 ^ invIn1_64) ^ invOut_64);
        } else if (gateType == aig){
          outValue_64 = ~( ( ~(fanin1->outValue_64 ^ invIn1_64) & ~(fanin2->outValue_64 ^ invIn2_64) ) ^ invOut_64);
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
        copy->invIn1_64 = this->invIn1_64;
        copy->invIn2_64 = this->invIn2_64;
        copy->invOut_64 = this->invOut_64;
        copy->outValue_64 = this->outValue_64;
        copy->fanout.clear();
        copy->fanout.assign(this->fanout.begin(), this->fanout.end());
        copy->fanin1 = nullptr;
        copy->fanin2 = nullptr;
        copy->fanin1 = this->fanin1;
        copy->fanin2 = this->fanin2;
      }
  };
}



#endif
