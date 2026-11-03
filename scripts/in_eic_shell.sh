#!/bin/bash
# in_eic_shell.sh: run a bash script inside eic-shell, passing variables to it.
#
#   scripts/in_eic_shell.sh <script> [VAR=value ...]
#
# eic-shell runs its arguments with "bash -c", so "eic-shell -- bash -c ..." fails ("Illegal
# option --"). Instead the variable assignments and the script are piped into eic-shell on
# stdin. The script must run "set --" before sourcing thisepic.sh / eicrecon-this.sh (in piped
# mode $1 would otherwise be picked up as DETECTOR_CONFIG).
# Example:
#   scripts/in_eic_shell.sh scripts/sim/ddsim_gun.sh GEO=local PART=lambda N=500 SEED=101 OUT=lambda_new_s101.edm4hep.root
set -e
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
script=$1; shift
[ -f "$script" ] || { echo "no such script: $script" >&2; exit 1; }
{
  echo "export EIC_TOP=$EIC_TOP EIC_WORK=$EIC_WORK"
  for kv in "$@"; do echo "export $kv"; done
  cat "$script"
} | "$EIC_TOP/eic-shell"
