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


//*******
for (int i = 0; i < theCircuit.size(); i++) {
    cout << i << " " << theCircuit[i]->outName << " " << theCircuit[i]->gateType << endl;
    }
    cout << endl;

    // check cnf formula
    for (int i =0; i < CNFFormula.size(); i++) {
        for (int j = 0; j < CNFFormula[i].size(); j++) {
            if (CNFFormula[i][j] > 0) {
                cout << CNFFormula[i][j] - 1 << " ";
            } else {
                cout << "-" << (-1)*CNFFormula[i][j] - 1 << " ";
            }
        }
        cout << endl;
    }
