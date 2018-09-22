#include <vector>
#include <set>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

void getTestPattern(char *patternFile) {
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

int main(int argc, char **argv){
    if(argc < 1){
        cout << "Miss some input file" << endl;
        return 0;
    }

    cout << "printHere"  << endl;

    char *patternFile = argv[1];

    getTestPattern(patternFile);
}