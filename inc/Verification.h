#ifndef _VERIFICATION_H
#define _VERIFICATION_H

#include <vector>
#include <set>
#include <string>
#include "Circuit.h"

using namespace std;
using namespace Circuit;

namespace Verification {
  class verify {
  public:
    verify();
    ~verify();
    // 1. try to find the PI that can cover all input cases of the target gate
    //    Do it by add constraints of target gates' input value in CNF Formula
    // 2. for the input cases which are covered, we can also know its output value in SAT results.
    // 3. check whether the value can be propagated to PO by taking it as stuck at faults.
    //    but needs new constraints: the input value of the target cases should by the one we found just now.
    // should do some changes in Circuit.h --> not only AIG, but also all other wires can insert stuck-at faults
  };
}

#endif
