#ifndef _CNFGENERATION_H
#define _CNFGENERATION_H

#include <vector>
#include <set>
#include <string>
#include "Circuit.h"
#include "Gate.h"

using namespace std;
using namespace Gate;
using namespace Circuit;

namespace CNFGeneration {
  class cnfgeneration {
    public:
      gate* curGate;

      cnfgeneration() {}
      ~cnfgeneration() {}
      // generate current clause in CNF format
      // ID in cnf formula is gate ID + 1. And it will be -1 in glucose.h
      void generateClause(vector<vector<int>> &gateClause, gate* curGate) {
        this->curGate = curGate;
        Type gateType = curGate->gateType;
        bool invIn1 = curGate->invIn1;
        bool invIn2 = curGate->invIn2;
        bool invOut = curGate->invOut;
        int gateID = curGate->gateID;
        gate *fanin1 = curGate->fanin1;
        gate *fanin2 = curGate->fanin2;
        bool outValue = curGate->outValue;

        gateClause.clear();
        vector<int>clause;
        // Generally CNF formula's ID starts from 1, but our circuits's gate ID start from 0
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
          int varOut = invOut ? (gateID+1) : (gateID+1)*(-1);
          clause.push_back(varIn1);
          clause.push_back(varOut*(-1));
          gateClause.push_back(clause);
          clause.clear();
          clause.push_back(varIn1*(-1));
          clause.push_back(varOut);
          gateClause.push_back(clause);
        } else if (gateType == constant) {
          // varOut * constant value
          int varOut = (gateID+1)*(outValue > 0 ? 1 : -1);
          clause.push_back(varOut);
          gateClause.push_back(clause);
        } else if (gateType == aig) { //AIG
          //(varIn1+~varOut)(varIn2+~varOut)(~varIn1+~varIn2+varOut)
          int varIn1 = invIn1 ? (fanin1->gateID+1) : (fanin1->gateID+1)*(-1);
          int varIn2 = invIn2 ? (fanin2->gateID+1) : (fanin2->gateID+1)*(-1);
          int varOut = invOut ? (gateID+1) : (gateID+1) * (-1);
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
          int varOut = invOut ? (gateID+1) : (gateID+1) * (-1);
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
          int varOut = invOut ? (gateID+1) : (gateID+1) * (-1);
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
  };
}

#endif
