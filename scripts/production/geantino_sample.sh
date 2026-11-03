#!/bin/bash
# geantino_sample.sh: geantino gun sample for the geometric B0 acceptance, copied to EOS.
# Runs OUTSIDE eic-shell.
#
#   scripts/production/geantino_sample.sh <GEO> <NAME> <N> [DEST]
#     GEO   container | local (see scripts/sim/ddsim_gun.sh)
#     NAME  e.g. geantino_new_5000 -> NAME.edm4hep.root
#     DEST  default $EIC_DATA/geantino
# 5000 events take ~2 min.
set -e
H=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$H/env.sh"
GEO=$1; NAME=$2; N=$3; DEST=${4:-$EIC_DATA/geantino}
[ -n "$N" ] || { sed -n 2,10p "$0"; exit 1; }
D=$EIC_WORK/$NAME
mkdir -p "$D"
"$H/in_eic_shell.sh" "$H/sim/ddsim_gun.sh" GEO=$GEO PART=geantino N=$N SEED=7 PMIN=100 PMAX=100 OUT=$NAME.edm4hep.root \
    > "$D/sim.log" 2>&1 || true
grep -q SIM_OK "$D/sim.log" || { echo "FAILED: $D/sim.log" >&2; exit 1; }
mkdir -p "$DEST/logs"
cp "$D/$NAME.edm4hep.root" "$DEST/" && cp "$D/sim.log" "$DEST/logs/${NAME}_sim.log"
echo "copied to $DEST"
