#include <errno.h>
#include <signal.h>
#include <zlib.h>
#include <sys/resource.h>
#include "utils/System.h"
#include "utils/ParseUtils.h"
#include "utils/Options.h"
#include "core/Dimacs.h"
#include "simp/SimpSolver.h"
#include <stdio.h>
#include <stdlib.h>

using namespace Glucose;

void glucoseInitial(SimpSolver  &S) {
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

  S.parsing = 1;
  S.use_simplification = true;
  S.verbosity = 1;
  S.verbEveryConflicts = 10000;
  S.showModel = false;
}

// the type of input data*******
void glucoseReadClause(int *test, Solver& S) {
  int clauseNum = 3;//*******
  int litNum = 1;//*******

  int parsed_lit, var;
  vec<Lit> lits;
  for (int i = 0; i < clauseNum; i++) {//*******
    for (int j = 0; j < litNum; j++) {//*******
      // add literal******
      parsed_lit = test[i]; //*******

      var = abs(parsed_lit)-1;
      while (var >= S.nVars()) S.newVar();
      lits.push( (parsed_lit > 0) ? mkLit(var) : ~mkLit(var) );
    }
    S.addClause_(lits);
    lits.clear();
  }
}

// if sat return 1, else 0
bool glucoseSATResult(lbool ret, Solver& S) {
  if (ret == l_True){
    printf("***************SAT Solutin**************\n");
    for (int i = 0; i < S.nVars(); i++) {
      if (S.model[i] != l_Undef) {
        int out = (S.model[i]==l_True)? 1 : 0;  // result
        printf("%d ",out);
      }
    }
    printf("\n");
    return 1;
  }
  else if (ret == l_False){
      printf("***************UNSAT***************\n");
      return 0;
  }
}

int main(int argc, char** argv)
{
  SimpSolver  S;
  glucoseInitial(S);

  //********
  int test[3] = {1, 2, -3};
  glucoseReadClause(test, S);

  S.parsing = 0;
  vec<Lit> dummy;
  lbool ret = S.solveLimited(dummy);

  //******
  glucoseSATResult(ret, S);

  return (ret == l_True ? 10 : ret == l_False ? 20 : 0);
}
