#ifndef _TMAXINTERFACE_H
#define _TMAXINTERFACE_H

#include "Circuit.h"
#include "Gate.h"

#include <vector>
#include <set>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;
using namespace Circuit;
using namespace Gate;

namespace TMAXINTERFACE {
  class tmax {
  public:
    vector <gate*> oriCircuit;
    vector <gate*> theCircuit;
    vector <vector<gate*>> faultyGatesSet;
    string fileName;
    int circuitSize;

    tmax(circuit *pCircuit, string fileName, set<int> &faultList) {
      this->oriCircuit = pCircuit->theCircuit;
      this->theCircuit.reserve(oriCircuit.size() * 10);
      copyCircuit(oriCircuit, this->theCircuit);
      circuitSize = theCircuit.size();
      this->fileName = fileName;
      //SSAFTestGeneration(faultList);
      test();
    }
    ~tmax(){}

    void copyCircuit(vector <gate*> &ori, vector <gate*> &copy) {
      for (int i = 0; i < ori.size(); i++) {
        // new a gate, then change its information
        gate *newGate = new gate();
        newGate->gateType = ori[i]->gateType;
        newGate->in1Name = ori[i]->in1Name;
        newGate->in2Name = ori[i]->in2Name;
        newGate->outName = ori[i]->outName;
        newGate->invIn1 = ori[i]->invIn1;
        newGate->invIn2 = ori[i]->invIn2;
        newGate->invOut = ori[i]->invOut;
        newGate->isPath = ori[i]->isPath;
        newGate->gateID = ori[i]->gateID;
        newGate->outValue = ori[i]->outValue;
        copy.push_back(newGate);
      }
      for (int i = 0; i < ori.size(); i++) {
        // PI and constant dont have fanin
        if (ori[i]->gateType != PI && ori[i]->gateType != constant) {
          int fanin1ID = ori[i]->fanin1->gateID;
          copy[i]->fanin1 = copy[fanin1ID];
        }
        // PO doesnt have fanout
        if (ori[i]->gateType != PO) {
          for (int j = 0; j < ori[i]->fanout.size(); j++) {
            int fanoutID = ori[i]->fanout[j]->gateID;
            copy[i]->fanout.push_back(copy[fanoutID]);
          }
        }
        if (ori[i]->gateType == aig || ori[i]->gateType == OR || ori[i]->gateType == XOR) {
          int fanin2ID = ori[i]->fanin2->gateID;
          copy[i]->fanin2 = copy[fanin2ID];
        }
      }
    }

    void SSAFTestGeneration(set<int> &faultList) {
      cout << "\n*****Generate the Verilog File and Fault File for Tetramax*****" << endl;
      int dot = fileName.find(".");
      fileName.erase(dot, fileName.size() - dot);
      map<int, string> gateIDVerilogString;
      string vFileName = "tmaxTest/dataFile/test.v";
      generateVerilogFile(gateIDVerilogString, vFileName);
      generateFaultListFile(faultList, gateIDVerilogString);
      cout << "\n\n";
    }

    void generateVerilogFile(map<int, string> &gateIDVerilogString, string &vFileName) {
      gateIDVerilogString.clear();
      vector <gate*> curCircuit = theCircuit;
      //string vFileName;
      //vFileName.append("tmaxTest/dataFile/test.v");

      ofstream myfile;
      myfile.open (vFileName);
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
              // sometimes ABC generated the node name [...], and it cannot be read by tmax..
              // myfile << curCircuit[i]->outName;
              myfile << getName(curCircuit[i]);
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
              //myfile << curCircuit[i]->outName;
              myfile << getName(curCircuit[i]);
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
              //myfile << curCircuit[i]->outName;
              myfile << getName(curCircuit[i]);
              if (i + 1 == curCircuit.size() || curCircuit[i+1]->gateType != PO) myfile << "; ";
              else myfile << ", ";
              //if (i + 1 < curCircuit.size()) myfile << ", ";
              //else myfile << "; ";
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
              //myfile << curCircuit[i]->outName;
              myfile << getName(curCircuit[i]);
              if (i + 1 < curCircuit.size()) myfile << ", ";
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

      for (auto curGate : curCircuit) {
          if (curGate->gateType == null)  continue;
          //if (curGate->in1Name.compare(curGate->outName) == 0)  continue;
          if (curGate->gateType == aig || curGate->gateType == bufInv || curGate->gateType == constant) {
            /*
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
            */
            if (curGate->gateType == aig) {
              int gateTypeID = aigCase(curGate);
              myfile << "N" << gateTypeID << " g" << curGate->gateID << "(";
              // myfile << curGate->outName << ", ";
              // myfile << curGate->fanin1->outName << ", ";
              // myfile << curGate->fanin2->outName << ");" << endl;
              myfile << getName(curGate) << ", ";
              myfile << getName(curGate->fanin1) << ", ";
              myfile << getName(curGate->fanin2) << ");" << endl;
            } else if (curGate->gateType == bufInv) {
              int gateTypeID = bufCase(curGate);
              myfile << "N" << gateTypeID << " g" << curGate->gateID << "(";
              //myfile << curGate->outName << ", ";
              //myfile << curGate->fanin1->outName << ");" << endl;
              myfile << getName(curGate) << ", ";
              myfile << getName(curGate->fanin1) << ");" << endl;
            } else if (curGate->gateType == constant) {
              int gateTypeID = constantCase(curGate);
              myfile << "N" << gateTypeID << " g" << curGate->gateID << "(";
              // myfile << curGate->outName << ");" << endl;
              myfile << getName(curGate) << ");";
            }
            string name = "g" + to_string(curGate->gateID);
            gateIDVerilogString.insert(make_pair(curGate->gateID, name));
          }
      }

      myfile << "\n\n\n\n\n";

      myfile << "endmodule" << endl;

      myfile.close();
    }

    string getName(gate *curGate) {
      string name = curGate->outName;
      if(name.find("[") != string::npos) {
        int start = name.find("[");
        int end = name.find("]");
        name = name.substr(start + 1, end - 1);
        string tmp = "tt_";
        tmp.append(name);
        name = tmp;
      }
      return name;
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

    int constantCase(gate *curGate) {
        if (curGate->gateType != constant) return 0;
        if (curGate->outValue == 1) return 11;
        else return 12;
    }

    /*
    void generateFaultListFile(vector <gate*> &curCircuit, string fileName, map<int, string> &gateIDVerilogString) {
      string circuitName = fileName;
      fileName.append("tmaxTest/dataFile/test.faults");
      ofstream myfile;
      myfile.open (fileName);
      // generate the faults in PI
      for (auto curGate : theCircuit) {
        if (curGate->gateType != PI) continue;
        myfile << " sa0   --   ";
        myfile << curGate->outName << endl;
        myfile << " sa1   --   ";
        myfile << curGate->outName << endl;
      }
      // generate all internal faults
      for (auto iter : gateIDVerilogString) {
        gate* curGate = theCircuit[iter.first];
        string moduleName = iter.second;
        myfile << " sa0   --   ";
        myfile << moduleName << "/x" << endl;
        myfile << " sa1   --   ";
        myfile << moduleName << "/x" << endl;
        if (curGate->gateType == aig) {
          myfile << " sa0   --   ";
          myfile << moduleName << "/y" << endl;
          myfile << " sa1   --   ";
          myfile << moduleName << "/y" << endl;
          myfile << " sa0   --   ";
          myfile << moduleName << "/z" << endl;
          myfile << " sa1   --   ";
          myfile << moduleName << "/z" << endl;
        }
      }
      myfile.close();
    }
    */

    void generateFaultListFile(set<int> faultList, map<int, string> &gateIDVerilogString) {
      string cFileName;
      cFileName.append("tmaxTest/dataFile/test.faults");
      ofstream myfile;
      myfile.open (cFileName);
      for (auto faultID : faultList) {
        int gateID = (faultID >> 3);
        int port = (faultID >> 1) & 3;
        int stuckat = faultID & 1;
        gate *curGate = theCircuit[gateID];
        if (curGate->gateType == PI) {
          myfile << " sa" << stuckat <<"   --   " << curGate->outName << endl;
        } else {
          if (gateIDVerilogString.find(gateID) == gateIDVerilogString.end()) continue;
          myfile << " sa" << stuckat <<"   --   ";
          myfile << gateIDVerilogString[gateID];
          if (curGate->gateType != aig) {
            myfile << "/x" << endl;
          } else {
            if (port == 1) {
              myfile << "/y" << endl;
            } else if (port == 2) {
              myfile << "/z" << endl;
            } else if (port == 3) {
              myfile << "/x" << endl;
            }
          }
        }
      }

      myfile.close();
    }

    void parseTestPattern(char *patternFile) {
      string fileName = "./dataFile/test.pattern";
      ofstream myfile;
      myfile.open (fileName);
      ifstream file;
      file.open(patternFile);
      if(!file){
        cout << "Cannot open the file" << endl;
        return;
      }

      string line;
      while(!file.eof()){
        getline(file, line);
        if (line.find("force_all_pis") != string::npos) {
          int start = line.find("=");
          if (start + 2 >= line.size()) {
            getline(file, line);
            while (line.find("Time 1: measure_all_pos") == string::npos) {
              for (int i = 0; i < line.size(); i++) {
                if (line[i] == '0' || line[i] == '1') myfile << line[i];
                else if (line[i] == 'z') myfile << "1";
              }
              getline(file, line);
            }
            myfile << endl;
            } else {
              for (int i = start; i < line.size(); i++) {
                if (line[i] == '0' || line[i] == '1') myfile << line[i];
                else if (line[i] == 'z') myfile << "1";
              }
              myfile << endl;
            }
          }
        }
        file.close();
        myfile.close();
    }

    // only parse the redundant fault.
    int parseFaultList(char *faultListFile) {
      set<int> redundantSSAF;
      ifstream file;
      file.open(faultListFile);
      if(!file){
        cout << "Cannot open the file" << endl;
        return 0;
      }
      string line;
      while(!file.eof()){
        getline(file, line);
        if (line.find("sa") == string::npos) continue;
        int stuckat = stoi(line.substr(line.find("sa")+2, line.find("sa")+2));
        int gateID = 0;
        int port = 0;
        if (line.find("/") != string::npos) {
          gateID = stoi(line.substr(line.find("g")+1, line.find("/")-1));
          string portName = line.substr(line.find("/")+1, line.find("/")+1);
          if (portName == "x") {
            port = 3;
          } else if (portName == "y") {
            port = 1;
          } else { // portName == 'z'
            port = 2;
          }
        } else {  // // fault in primary input
          string inputName = line.substr(line.find("G"), line.size()-1);
          port = 1;
          for (auto curGate : theCircuit) {
            if (curGate->outName == inputName) {
              gateID = curGate->gateID;
              break;
            }
          }
        }
        int faultID = getFaultID(gateID, port, stuckat);
        // if it's "UR", it must be redundant fault.
        // But for other faults, we still need to check again.
        redundantSSAF.insert(faultID);
      }
      file.close();
      return 1;
    }


    void test() {
      map<int, string> gateIDVerilogString;
      string vFileName = "tmaxTest/dataFile/test_noFault.v";
      generateVerilogFile(gateIDVerilogString, vFileName);

      set<set<int>> MSAs;
      set<int> MSA;
      set<int> SSAFsList;
      MSA.insert(63);
      MSA.insert(68);
      MSA.insert(82);
      MSAs.insert(MSA);
      transMSATOSSA(MSAs, SSAFsList);
      vFileName = "tmaxTest/dataFile/test_InsertFault.v";
      generateVerilogFile(gateIDVerilogString, vFileName);
      printCircuit(theCircuit);
      cout << "*****generated circuit's faults!!!*******" << endl;
      for (auto faultID : SSAFsList) {
        printFault2(faultID);
      }

      resetCircuit();
      vFileName = "tmaxTest/dataFile/test_reset.v";
      generateVerilogFile(gateIDVerilogString, vFileName);
    }


    // currently only for DSA and TSA
    void transMSATOSSA(set<set<int>> &MSAs, set<int> &SSAFsList) {
      int MSACount = 0;
      int gateID = 0;
      int port = 0;
      int stuckat = 0;
      for (auto MSA : MSAs) {
        // currently only for TSA and DSA.
        if (MSA.size() > 3) continue;
        // ---create MergeAND gate---
        vector<gate*> faultyGates;
        vector<gate*> MergeANDGate;
        string initName = "MergeAnd_0_MSA" + to_string(MSACount);
        string invIn1 = "1"; string invIn2 = "1"; string invOut = "1";
        vector<string> empty;
        // 2 and, fanin1,                   fanin2,                     fanout
        //     MergeANDGate[0]->fanin1,     MergeANDGate[0]->fanin2,    MergeANDGate[0]->fanout
        // 3 and, fanin1,                   fanin2                      fanin3                    fanout
        //     MergeANDGate[0]->fanin1,     MergeANDGate[0]->fanin2,    MergeANDGate[1]->fanin2       MergeANDGate[1]->fanout
        MergeANDGate.push_back(new gate(aig, initName, initName, initName, invIn1, invIn2, invOut));
        MergeANDGate[0]->gateID = theCircuit.size();
        gateID = MergeANDGate[0]->gateID;
        theCircuit.push_back(MergeANDGate[0]);
        faultyGates.push_back(MergeANDGate[0]);
        if (MSA.size() == 3) {
          string initName2 = "MergeAnd_1_MSA" + to_string(MSACount);
          MergeANDGate.push_back(new gate(aig, initName2, initName2, initName2, invIn1, invIn2, invOut));
          MergeANDGate[0]->fanout.push_back(MergeANDGate[1]);
          MergeANDGate[1]->gateID = theCircuit.size();
          MergeANDGate[1]->fanin1 = MergeANDGate[0];
          MergeANDGate[1]->in1Name = MergeANDGate[0]->outName;
          gateID = MergeANDGate[1]->gateID;
          theCircuit.push_back(MergeANDGate[1]);
          faultyGates.push_back(MergeANDGate[1]);
        }
        port = 3;
        stuckat = 1;
        cout << "11****gateID*****" << theCircuit.size() << endl;
        SSAFsList.insert(getFaultID(gateID, port, stuckat));
        // ---create inserted gate---
        int faultCount = 0;
        for (auto faultID : MSA) {
          gateID = (faultID >> 3);
          port = (faultID >> 1) & 3;
          stuckat = faultID & 1;
          gate *insertedGate;
          string insertedName = "insertedGate_" + to_string(faultCount) + "_MSA" + to_string(MSACount);
          if (stuckat == 1) { // 00 0, or gate
            invIn1 = "0"; invIn2 = "0"; invOut = "0";
          } else { // stuckat == 0, 10 1
            invIn1 = "1"; invIn2 = "0"; invOut = "1";
          }
          insertedGate = new gate(aig, insertedName, insertedName, insertedName, invIn1, invIn2, invOut);
          gate *faninGate;
          if (port == 1)      faninGate = theCircuit[gateID]->fanin1;
          else if (port == 2) faninGate = theCircuit[gateID]->fanin2;
          else                faninGate = theCircuit[gateID];
          vector<gate*> faninGateFanouts;
          // 1. disconnect faninGate with its original fanouts.
          if (port == 1 || port == 2) {
            int i = 0;
            for (i = 0; i < faninGate->fanout.size(); i++) {
              if (faninGate->fanout[i] == theCircuit[gateID]) break;
            }
            faninGateFanouts.push_back(faninGate->fanout[i]);
            faninGate->fanout[i] = faninGate->fanout[faninGate->fanout.size() - 1];
            faninGate->fanout.pop_back();
          } else {
            faninGateFanouts.insert(faninGateFanouts.end(), faninGate->fanout.begin(), faninGate->fanout.end());
            faninGate->fanout.clear();
          }
          // 2. connect the insertedGate to faninGate
          insertedGate->fanin2 = faninGate;
          insertedGate->in2Name = faninGate->outName;
          faninGate->fanout.push_back(insertedGate);
          // 3. connect the insertedGate to fanoutGate
          for (auto fanoutGate : faninGateFanouts) {
            insertedGate->fanout.push_back(fanoutGate);
            if (fanoutGate->fanin1 == faninGate) {
              fanoutGate->fanin1 = insertedGate;
              fanoutGate->in1Name =insertedGate->outName;
            }
            if (fanoutGate->gateType == aig && fanoutGate->fanin2 == faninGate) {
              fanoutGate->fanin2 = insertedGate;
              fanoutGate->in2Name = insertedGate->outName;
            }
          }

          // ---connect faninGate to MergeAND---
          // 2 and, fanin1,                   fanin2,                     fanout
          //     MergeANDGate[0]->fanin1,     MergeANDGate[0]->fanin2,    MergeANDGate[0]->fanout
          // 3 and, fanin1,                   fanin2                      fanin3                    fanout
          //     MergeANDGate[0]->fanin1,     MergeANDGate[0]->fanin2,    MergeANDGate[1]->fanin2       MergeANDGate[1]->fanout
          switch (faultCount) {
            case 0: {
              MergeANDGate[0]->fanin1 = faninGate;
              MergeANDGate[0]->in1Name = faninGate->outName;
              MergeANDGate[0]->invIn1 = stuckat;
            }
            case 1: {
              MergeANDGate[0]->fanin2 = faninGate;
              MergeANDGate[0]->in2Name = faninGate->outName;
              MergeANDGate[0]->invIn2 = stuckat;
            }
            case 2: {
              MergeANDGate[1]->fanin2 = faninGate;
              MergeANDGate[1]->in2Name = faninGate->outName;
              MergeANDGate[1]->invIn2 = stuckat;
            }
          }
          if (faultCount <= 2)  faninGate->fanout.push_back(MergeANDGate[0]);
          else                  faninGate->fanout.push_back(MergeANDGate[1]);

          // ---connect MergeAND to insertedGate---
          if (MSA.size() == 2) {
            MergeANDGate[0]->fanout.push_back(insertedGate);
            insertedGate->fanin1 = MergeANDGate[0];
            insertedGate->in1Name = MergeANDGate[0]->outName;
          } else {
            MergeANDGate[1]->fanout.push_back(insertedGate);
            insertedGate->fanin1 = MergeANDGate[1];
            insertedGate->in1Name = MergeANDGate[1]->outName;
          }
          insertedGate->gateID = theCircuit.size();
          faultyGates.push_back(insertedGate);
          faultyGatesSet.push_back(faultyGates);
          theCircuit.push_back(insertedGate);
          faultCount++;
        }
        MSACount++;
      }
    }

    /*
    // still has bugs..
    void resetCircuit() {
      for (auto faultyGates : faultyGatesSet) {
        for (auto curGate : faultyGates) {
          if (curGate->outName.find("MergeAnd") != string::npos) {
            gate *MergeANDGate = curGate;
            deleteInFanout(MergeANDGate->fanin1, MergeANDGate);
            //deleteInFanout(MergeANDGate->fanin2, MergeANDGate);
          } else {
            gate *insertedGate = curGate;
            gate *faninGate = insertedGate->fanin2;
            // only fanin2 connected to original circuit's gate.
            // delete fanin2's fanout
            deleteInFanout(faninGate, insertedGate);
            faninGate->fanout.insert(faninGate->fanout.end(), insertedGate->fanout.begin(), insertedGate->fanout.end());
            // change fanout's fanin
            for (auto fanoutGate : curGate->fanout) {
              if (fanoutGate->fanin1 == curGate) {
                fanoutGate->fanin1 = faninGate;
                fanoutGate->in1Name = faninGate->outName;
              }
              if (fanoutGate->gateType == aig && fanoutGate->fanin2 == curGate) {
                fanoutGate->fanin2 = faninGate;
                fanoutGate->in2Name = faninGate->outName;
              }
            }
          }
          //curGate->fanin1 = nullptr;
          //curGate->fanin2 = nullptr;
          curGate->fanout.clear();
        }
      }
      for (auto faultyGates : faultyGatesSet) {
        for (auto curGate : faultyGates) {
          delete curGate;
          theCircuit.pop_back();
        }
      }
      faultyGatesSet.clear();
    }
    // delete the "deletedGate" in preGate's fanout.
    void deleteInFanout(gate *preGate, gate *deletedGate) {
      int i = 0;
      for (i = 0; i < preGate->fanout.size(); i++) {
        if (preGate->fanout[i] == deletedGate) break;
      }
      preGate->fanout[i] = preGate->fanout[preGate->fanout.size() - 1];
      preGate->fanout.pop_back();
    }
    */

    int getFaultID(int gateID, int port, int stuckat) {
      return (gateID << 3) + (port << 1) + stuckat;
    }

    void resetCircuit() {
      /*
      for (auto faultyGates : faultyGatesSet) {
        for (auto curGate : faultyGates) {
          delete curGate;
          theCircuit.pop_back();
        }
      }
      */
      //oriCircuit
      for (auto curGate : theCircuit) {
        delete curGate;
      }
      theCircuit.clear();
      faultyGatesSet.clear();
      theCircuit.reserve(oriCircuit.size() * 10);
      copyCircuit(oriCircuit, theCircuit);
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
  };
}


#endif
