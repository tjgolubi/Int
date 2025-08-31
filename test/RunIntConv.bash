#!/usr/bin/env bash

# @file
# @copyright 2025 Terry Golubiewski, all rights reserved.
# @author Terry Golubiewski

set -uo pipefail

CXX="g++ -std=c++23"
CXXFLAGS="-Wall -Wextra -Werror -O2"
SRC="IntConv.cpp"

# Edit if Int.hpp is elsewhere; you can also add -I flags in CXXFLAGS.
INCLUDES="-I.."

let pass=0
let fail=0

run_case() {
  local macro="$1"
  echo -n "==> ${macro}"
  case "${macro}" in
    PASS_*) expect=pass;;
    FAIL_*) expect=fail;;
    *) echo
       echo "    ERROR: macro name must begin with 'PASS_' or 'FAIL_'"
       ((fail++))
       return;;
  esac
  echo " (expect ${expect})"

  if [ $(grep --count "#ifdef ${macro}" "${SRC}") -ne 1 ]
  then
    echo "    ERROR: ${macro} is not present in ${SRC}"
    ((fail++))
    return
  fi
  if ${CXX} -c ${CXXFLAGS} ${INCLUDES} "${SRC}" -D"${macro}" -o /dev/null
  then
    if [[ "${expect}" == "pass" ]]; then
      echo "    OK: compiled as expected"
      ((pass++))
    else
      echo "    ERROR: compiled but expected failure"
      ((fail++))
    fi
  else
    if [[ "${expect}" == "fail" ]]; then
      echo "    OK: failed as expected"
      ((pass++))
    else
      echo "    ERROR: failed to compile but expected success"
      ((fail++))
    fi
  fi
}

# Matrix
run_case PASS_INT_TO_INT_CONSTRUCT
run_case PASS_INT_TO_INT_NARROW_PAREN
run_case FAIL_INT_TO_INT_NARROW_BRACE
run_case FAIL_IMPLICIT_WIDEN_NATIVE
run_case FAIL_IMPLICIT_WIDEN_NONNATIVE
run_case PASS_SAME_T_IMPLICIT_NONNATIVE_EXPLICIT
run_case FAIL_SAME_T_IMPLICIT_NONNATIVE_IMPLICIT
run_case PASS_ASSIGN_WIDEN_FROM_INT
run_case FAIL_ASSIGN_NARROW_FROM_INT
run_case PASS_ASSIGN_NARROW_FROM_INT
run_case PASS_ASSIGN_WIDEN_FROM_SCALAR
run_case FAIL_ASSIGN_NARROW_FROM_SCALAR
run_case PASS_SCALAR_IN_WIDEN
run_case PASS_SCALAR_IN_NARROW_PAREN
run_case FAIL_SCALAR_IN_NARROW_BRACE
run_case PASS_SCALAR_OUT

echo
echo "Summary: ${pass} passed, ${fail} failed."
if [[ ${fail} -ne 0 ]]; then
  exit 1
fi
