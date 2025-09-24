#!/bin/bash

# @file
# @copyright 2025 Terry Golubiewski, all rights reserved.
# @author Terry Golubiewski

set -uo pipefail

CXX="g++ -std=gnu++23"
CXXFLAGS="-Wall -Wextra -Werror -O2"
src="IntConv.cpp"
log="${1:-/dev/stdout}"

reset=$'\e[0m'
red=$'\e[31m'
green=$'\e[32m'

# Edit if Int.hpp is elsewhere; you can also add -I flags in CXXFLAGS.
INCLUDES="-I.."

let pass=0
let fail=0

do_run_case() {
  local macro="$1"
  echo -n "==> ${macro}"
  case "${macro}" in
    PASS_*) expect=pass;;
    FAIL_*) expect=fail;;
    *) echo
       echo "    ERROR: macro name must begin with 'PASS_' or 'FAIL_'"
       ((fail++))
       return 1;;
  esac
  echo " (expect ${expect})"

  if [ $(grep --count "#ifdef ${macro}" "${src}") -ne 1 ]
  then
    echo "    ERROR: ${macro} is not present in ${src}"
    ((fail++))
    return 1
  fi
  if ${CXX} -c ${CXXFLAGS} ${INCLUDES} "${src}" -D"${macro}" -o /dev/null
  then
    if [[ "${expect}" == "pass" ]]; then
      echo "    OK: compiled as expected"
      ((pass++))
    else
      echo "    ERROR: compiled but expected failure"
      ((fail++))
      return 1
    fi
  else
    if [[ "${expect}" == "fail" ]]; then
      echo "    OK: failed as expected"
      ((pass++))
    else
      echo "    ERROR: failed to compile but expected success"
      ((fail++))
      return 1
    fi
  fi
  return 0
}

run_case() {
  echo -n "$@"
  if do_run_case "$@" > "${log}" 2>&1
  then echo
  else echo "${red} FAILED${reset}"
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

if ((fail == 0)); then
  fail_color="${green}"
else
  fail_color="${red}"
fi
echo "Summary: ${green}${pass} passed${reset}, ${fail_color}${fail} failed${reset}."
if ((fail != 0)); then
  exit ${fail}
fi
