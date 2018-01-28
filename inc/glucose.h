#ifndef _GLUCOSE_H
#define _GLUCOSE_H

#include "System.h"
#include "Dimacs.h"
#include "SimpSolver.h"
#include <vector>
#include <set>
#include <map>

using namespace std;

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

      int runGlucose(vector<vector<int>> &CNFFormula, vector<int> &result) {
        readClause(CNFFormula);
        runSAT();
        return SATResult(result);
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

      void readClause(vector<vector<int>> &CNFFormula) {
        int parsed_lit, var;
        vec<Lit> lits;
        for (int i = 0; i < CNFFormula.size(); i++) {//clauses
          for (int j = 0; j < CNFFormula[i].size(); j++) {//literals
            // add literal
            parsed_lit = CNFFormula[i][j];
            var = abs(parsed_lit)-1;
            while (var >= solver->nVars()) solver->newVar();
            lits.push( (parsed_lit > 0) ? mkLit(var) : ~mkLit(var) );
          }
          solver->addClause_(lits);
          lits.clear();
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
  };
}

#endif
