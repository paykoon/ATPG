#ifndef _GLUCOSE_H
#define _GLUCOSE_H

#include "System.h"
#include "Dimacs.h"
#include "SimpSolver.h"
#include <vector>
#include <set>
#include <map>
#include "Gate.h"

using namespace std;
using namespace Gate;

namespace Glucose {
  class glucose {
    public:
      glucose() {
        solver = new SimpSolver();
        initial();
      }
      ~glucose() {
        delete solver;
      }

      // send the entire CNFFormula to GLucose to do SAT.
      int SATCircuit(vector<vector<vector<int>>> &CNFOriAndFauCir, vector<int> &result, int theCircuitSize, int PISize) {
        if (CNFOriAndFauCir.size() == 0) return 0;
        vector<int> allValue;
        if (runGlucose(CNFOriAndFauCir, allValue) == 1) {
          // cout << "SAT" << endl;
          // only take the PI value in "allValue" as result
          // original circuit | faulty circuit | new input | new XOR | new output | an "OR" gate for all outputs | constant wire(stuck at faults)
          // circuit size        circuit size    PI size      PO size   PO size           1
          int start = theCircuitSize * 2;
          result.reserve(PISize);
          for (int i = 0; i < PISize; i++) {
            result.push_back(allValue[i+start]);
          }
          return 1;
        }
        else {
          //cout << "UNSAT" << endl;
          return 0;
        }
      }

    private:
      SimpSolver  *solver;
      lbool ret;

      void initial() {
        #if defined(__linux__)
        fpu_control_t oldcw, newcw;
        _FPU_GETCW(oldcw); newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE; _FPU_SETCW(newcw);
        #endif

        int cpu_lim = INT32_MAX;
        int mem_lim = INT32_MAX;

        if (cpu_lim != INT32_MAX){
          rlimit rl;
          getrlimit(RLIMIT_CPU, &rl);
          if (rl.rlim_max == RLIM_INFINITY || (rlim_t)cpu_lim < rl.rlim_max){
              rl.rlim_cur = cpu_lim;
              if (setrlimit(RLIMIT_CPU, &rl) == -1)
                  printf("c WARNING! Could not set resource limit: CPU-time.\n");
          }
        }
        // Set limit on virtual memory:
        if (mem_lim != INT32_MAX){
          rlim_t new_mem_lim = (rlim_t)mem_lim * 1024*1024;
          rlimit rl;
          getrlimit(RLIMIT_AS, &rl);
          if (rl.rlim_max == RLIM_INFINITY || new_mem_lim < rl.rlim_max){
              rl.rlim_cur = new_mem_lim;
              if (setrlimit(RLIMIT_AS, &rl) == -1)
                  printf("c WARNING! Could not set resource limit: Virtual memory.\n");
          }
        }
        solver->parsing = 1;
        solver->use_simplification = true;
        solver->verbosity = 1;
        solver->verbEveryConflicts = 10000;
        solver->showModel = false;
      }

      void readClause(vector<vector<vector<int>>> &CNFFormula) {
        int parsed_lit, var;
        vec<Lit> lits;
        for (int m = 0; m < CNFFormula.size(); m++) {  // gate
          for (int i = 0; i < CNFFormula[m].size(); i++) {//clauses
            for (int j = 0; j < CNFFormula[m][i].size(); j++) {//literals
              // add literal
              parsed_lit = CNFFormula[m][i][j];
              var = abs(parsed_lit)-1;
              while (var >= solver->nVars()) solver->newVar();
              lits.push( (parsed_lit > 0) ? mkLit(var) : ~mkLit(var) );
            }
            solver->addClause_(lits);
            lits.clear();
          }
        }
      }

      void runSAT() {
        solver->parsing = 0;
        vec<Lit> dummy;
        ret = solver->solveLimited(dummy);
      }

      // if sat return 1, else 0
      int SATResult(vector<int> &result) {
        if (ret == l_True){
          for (int i = 0; i < solver->nVars(); i++) {
            if (solver->model[i] != l_Undef) {
              int out = (solver->model[i]==l_True)? 1 : 0;  // result
              result.push_back(out);
            }
          }
          return 1;
        }
        else{
          return 0;
        }
      }

      int runGlucose(vector<vector<vector<int>>> &CNFFormula, vector<int> &result) {
        readClause(CNFFormula);
        runSAT();
        return SATResult(result);
      }

  };
}

#endif
