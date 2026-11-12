#!/bin/bash
# dis_lambda_sample.sh: one Lambda-tagged DIS sample = Pythia8 generation (once), then simulation
# and reconstruction (EICrecon#3116 build, truth + hit seeding) in the old and new geometry, copied
# to EOS. Runs OUTSIDE eic-shell.
#
#   scripts/production/dis_lambda_sample.sh <SEED> <N> [DEST]
#     N     number of kept events (Lambda at theta < 25 mrad, scripts/gen/gen_dis_lambda.sh)
#     DEST  default $EIC_DATA/lambda_dis (18x275) or $EIC_DATA/lambda_dis_<BEAM>
#   Files (P = dis_lambda for 18x275, dis_lambda_<BEAM> otherwise): P_sSEED.hepmc,
#          P_{old,new}_sSEED.edm4hep.root, P_{old,new}_sSEED.eicreconpr3116.edm4eic.root, logs/
#   Environment (optional):
#     BEAM   beam configuration (default 18x275, e.g. 9x130): generator energies and
#            epic_craterlake_<BEAM>.xml in simulation and reconstruction
#     Q2MIN, THMAX as in gen_dis_lambda.sh (EE, EP override the energies taken from BEAM)
#     OLD, NEW geometries (default epic_43f427e, local); RECO (default EICrecon_pr3116)
#
# Full DIS events: ~15 s/event in ddsim on lxplus (500 events ~2 h), both geometries in parallel.
# Stay on the same lxplus node (the job writes to its /tmp). Started 2026-11-12 with
#   for s in 301 302; do nohup setsid scripts/production/dis_lambda_sample.sh $s 500 \
#       > /tmp/$USER/dis_lambda_s$s.log 2>&1 < /dev/null & done
#   for s in 401 402; do BEAM=9x130 nohup setsid scripts/production/dis_lambda_sample.sh $s 500 \
#       > /tmp/$USER/dis_lambda_9x130_s$s.log 2>&1 < /dev/null & done
set -e
H=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$H/env.sh"
BEAM=${BEAM:-18x275}
P=dis_lambda; [ "$BEAM" = 18x275 ] || P=dis_lambda_$BEAM
SEED=$1; N=$2; DEST=${3:-$EIC_DATA/lambda_dis}
[ "$BEAM" = 18x275 ] || DEST=${3:-$EIC_DATA/lambda_dis_$BEAM}
[ -n "$N" ] || { sed -n 2,23p "$0"; exit 1; }
OLD=${OLD:-epic_43f427e}; NEW=${NEW:-local}; RECO=${RECO:-EICrecon_pr3116}
EE=${EE:-${BEAM%x*}}; EP=${EP:-${BEAM#*x}}
G=${P}_s$SEED
D=$EIC_WORK/$G
mkdir -p "$D" "$DEST/logs"
step() {  # step <log> <OK marker> <script> <VAR=value...>
  local log=$1 ok=$2; shift 2
  "$H/in_eic_shell.sh" "$@" > "$D/$log" 2>&1 || true
  grep -q "$ok" "$D/$log" || { echo "FAILED: $D/$log" >&2; return 1; }
  echo "done: $log"
}
gen_vars=(N=$N SEED=$SEED OUT=$G.hepmc EE=$EE EP=$EP)
for v in Q2MIN THMAX; do [ -n "${!v}" ] && gen_vars+=("$v=${!v}"); done
step gen.log GEN_OK "$H/gen/gen_dis_lambda.sh" "${gen_vars[@]}"
grep GEN_SUMMARY "$D/gen.log"
cp "$D/$G.hepmc" "$DEST/"; cp "$D/gen.log" "$DEST/logs/${G}_gen.log"
one() {  # one <old|new> <GEO>: simulate and reconstruct in one geometry
  local g=$1 geo=$2 name=${P}_$1_s$SEED
  step ${g}_sim.log SIM_OK "$H/sim/ddsim_hepmc.sh" GEO=$geo BEAM=$BEAM IN=$D/$G.hepmc OUT=$name.edm4hep.root || return 1
  local sim=$EIC_WORK/$name/$name.edm4hep.root
  step ${g}_reco.log RECO_OK "$H/reco/eicrecon.sh" GEO=$geo BEAM=$BEAM RECO=$RECO IN=$sim \
       OUT=$EIC_WORK/$name/$name.eicreconpr3116.edm4eic.root || return 1
  cp "$sim" "$EIC_WORK/$name/$name.eicreconpr3116.edm4eic.root" "$DEST/"
  for l in sim reco; do cp "$D/${g}_$l.log" "$DEST/logs/${name}_$l.log"; done
  echo "copied $name to $DEST"
}
one old "$OLD" & p1=$!
one new "$NEW" & p2=$!
wait $p1 && wait $p2 && echo ALL_OK
