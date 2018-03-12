#ifndef _GATE_H
#define _GATE_H

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
using namespace std;




enum Type {constant, bufInv, aig, PO, PI, OR, XOR};

namespace Gate {
  class gate{
    public:
      // PI or PO (The name of In and out are same)
      gate(Type gateType, string &portName){
        this->gateType = gateType;
        this->faultyOut = false;
        this->in1Name = portName;
        this->outName = portName;
        this->invIn1 = 1;
        this->invOut = 1;
      }
      // constant (The name of In and out are same)
      gate(string &wireName, string &value){
        this->gateType = constant;
        this->faultyOut = false;
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
          this->faultyOut = false;
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
          this->faultyOut = false;
          in1Checked = false;
          in2Checked = false;
      }

      ~gate(){
        delete this;
      }

      void setPI(char inValues){
        outValue = (inValues == '1');
      }

      void setOut(){
        // the outValue of PI and constant will be decided in other places
        if (gateType == PO) {
          outValue = fanin1->outValue;
        } else if(gateType == bufInv){
          outValue = ( fanin1->outValue == invIn1 ) == invOut;
        } else if(gateType == aig){
          outValue = ( (fanin1->outValue == invIn1) & (fanin2->outValue == invIn2) ) == invOut;
        }
      }

      void setID(int gateID){
        this->gateID = gateID;
      }

      Type getType(){
        return gateType;
      }

      Type getFanin1Type(){
        return fanin1->getType();
      }

      Type getFanin2Type(){
        return fanin2->getType();
      }

      int getFanoutNum(){
        return fanout.size();
      }

      int getFanin1OutNum(){
        return fanin1->getFanoutNum();
      }

      int getFanin2OutNum(){
        return fanin2->getFanoutNum();
      }

      string getIn1Name(){
        return in1Name;
      }

      string getIn2Name(){
        return in2Name;
      }

      string getOutName(){
        return outName;
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

      // generate current clause in CNF format
      // ID in cnf formula is gate ID + 1. And it will be -1 in glucose.h
      void generateClause(vector<vector<int>> &gateClause) {
        gateClause.clear();
        vector<int>clause;
      	// general CNF formula's ID starts from 1, but our circuits's gate ID start from 0
        // so we should do gateID + 1
        if (gateType == PI) {
          //(varOut + ~varOut). Means dont care.
          int varIn1 = (gateID+1)*(-1);
          int varOut = gateID+1;
          clause.push_back(varIn1);
          clause.push_back(varOut);
          gateClause.push_back(clause);
          clause.clear();
        } else if (gateType == PO || gateType == bufInv) {
          //(varIn1 + ~varOut)(~varIn1+varOut)
          int varIn1 = invIn1 ? (fanin1->gateID+1) : (fanin1->gateID+1)*(-1);
      	  int varOut = gateID+1;
      	  clause.push_back(varIn1);
      	  clause.push_back(varOut*(-1));
      	  gateClause.push_back(clause);
      	  clause.clear();
      	  clause.push_back(varIn1*(-1));
      	  clause.push_back(varOut);
      	  gateClause.push_back(clause);
      	} else if (gateType == constant) {
          int varOut = (gateID+1)*(outValue > 0 ? 1 : -1);
      	  clause.push_back(varOut);
      	  gateClause.push_back(clause);
        } else if (gateType == aig) { //AIG
          //(varIn1+~varOut)(varIn2+~varOut)(~varIn1+~varIn2+varOut)
          int varIn1 = invIn1 ? (fanin1->gateID+1) : (fanin1->gateID+1)*(-1);
          int varIn2 = invIn2 ? (fanin2->gateID+1) : (fanin2->gateID+1)*(-1);
          int varOut = gateID+1;
      	  // (varIn1+~varOut)
      	  clause.push_back(varIn1);
      	  clause.push_back(varOut*(-1));
      	  gateClause.push_back(clause);
      	  clause.clear();
          // (varIn2+~varOut)
      	  clause.push_back(varIn2);
      	  clause.push_back(varOut*(-1));
      	  gateClause.push_back(clause);
      	  clause.clear();
          // (~varIn1+~varIn2+varOut)
          clause.push_back(varIn1*(-1));
      	  clause.push_back(varIn2*(-1));
      	  clause.push_back(varOut);
      	  gateClause.push_back(clause);
        } else if (gateType == OR) {
          //(~varIn1+varOut)(~varIn2+varOut)(varIn1+varIn2+~varOut)
          int varIn1 = invIn1 ? (fanin1->gateID+1) : (fanin1->gateID+1)*(-1);
          int varIn2 = invIn2 ? (fanin2->gateID+1) : (fanin2->gateID+1)*(-1);
          int varOut = gateID+1;
      	  // (~varIn1+varOut)
      	  clause.push_back(varIn1*(-1));
      	  clause.push_back(varOut);
      	  gateClause.push_back(clause);
      	  clause.clear();
          // (~varIn2+varOut)
      	  clause.push_back(varIn2*(-1));
      	  clause.push_back(varOut);
      	  gateClause.push_back(clause);
      	  clause.clear();
          // (varIn1+varIn2+~varOut)
          clause.push_back(varIn1);
      	  clause.push_back(varIn2);
      	  clause.push_back(varOut*(-1));
      	  gateClause.push_back(clause);
        } else if (gateType == XOR) {
          //(varIn1+varIn2+~varOut)(varIn1+~varIn2+varOut)(~varIn1+~varIn2+~varOut)(~varIn1+varIn2+varOut)
          int varIn1 = invIn1 ? (fanin1->gateID+1) : (fanin1->gateID+1)*(-1);
          int varIn2 = invIn2 ? (fanin2->gateID+1) : (fanin2->gateID+1)*(-1);
          int varOut = gateID+1;
          // (varIn1+varIn2+~varOut)
          clause.push_back(varIn1);
      	  clause.push_back(varIn2);
      	  clause.push_back(varOut*(-1));
      	  gateClause.push_back(clause);
          clause.clear();
          // (varIn1+~varIn2+varOut)
          clause.push_back(varIn1);
          clause.push_back(varIn2*(-1));
          clause.push_back(varOut);
          gateClause.push_back(clause);
          clause.clear();
          // (~varIn1+~varIn2+~varOut)
          clause.push_back(varIn1*(-1));
          clause.push_back(varIn2*(-1));
          clause.push_back(varOut*(-1));
          gateClause.push_back(clause);
          clause.clear();
          // (~varIn1+varIn2+varOut)
          clause.push_back(varIn1*(-1));
          clause.push_back(varIn2);
          clause.push_back(varOut);
          gateClause.push_back(clause);
          clause.clear();
        }
      }

      Type gateType;
      string in1Name,in2Name,outName;
      bool invIn1,invIn2,invOut;
      bool faultyOut;
      int gateID;
      vector <gate*> fanout;
      gate *fanin1,*fanin2;
      bool outValue;
      //mark whether the fault is checked or not(both sa0 and sa1)
      //only for aig.
      bool in1Checked, in2Checked;
      bool visited;
  };
}



#endif