#!/bin/bash
# lambda_sample.sh: one Lambda gun sample = simulation + reconstruction with EICrecon main at
# the default 10 keV and at 5 keV B0 threshold, copied to EOS. Runs OUTSIDE eic-shell.
#
#   scripts/production/lambda_sample.sh <GEO> <NAME> <SEED> <N> [DEST]
#     GEO   container | local (see scripts/sim/ddsim_gun.sh)
#     NAME  sample name, e.g. lambda_new_s101; files: NAME.edm4hep.root,
#           NAME.eicreconmain.edm4eic.root, NAME.eicreconmain_5keV.edm4eic.root
#     DEST  default $EIC_DATA/lambda_1000ev
#   Environment (optional):
#     PMIN, PMAX  Lambda momentum range in GeV (default 50, 100)
#     SIM_ONLY=1  only simulate (reconstruct afterwards with scripts/production/lambda_rereco.sh)
#
# ~5 min per 500 events per step on lxplus. To keep it running after logout, start it detached,
# and stay on the same lxplus node: the job writes to that node's /tmp.
#   nohup setsid scripts/production/lambda_sample.sh local lambda_new_s101 101 500 \
#       > /tmp/$USER/lambda_new_s101.log 2>&1 < /dev/null &
# The 1000-event samples of this study: container/local x seeds 101, 102, 500 events each
# (container = old geometry, epic 43f427e, as long as the nightly container has it).
# High-momentum samples (100-275 GeV, 2026-11-11), old = epic_43f427e, new = local, then hit seeding:
#   for s in 201 202; do for g in old new; do G=local; [ $g = old ] && G=epic_43f427e
#     PMIN=100 PMAX=275 SIM_ONLY=1 scripts/production/lambda_sample.sh $G lambda_${g}_s$s $s 500 $EIC_DATA/lambda_100to275 &
#   done; done; wait
#   ... then lambda_rereco.sh <G> lambda_<g>_s<s> EICrecon_pr3116 eicreconpr3116 "" $EIC_DATA/lambda_100to275
set -e
H=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$H/env.sh"
GEO=$1; NAME=$2; SEED=$3; N=$4; DEST=${5:-$EIC_DATA/lambda_1000ev}
[ -n "$N" ] || { sed -n 2,24p "$0"; exit 1; }
D=$EIC_WORK/$NAME
mkdir -p "$D"
step() {  # step <log> <OK marker> <script> <VAR=value...>
  local log=$1 ok=$2; shift 2
  "$H/in_eic_shell.sh" "$@" > "$D/$log" 2>&1 || true
  grep -q "$ok" "$D/$log" || { echo "FAILED: $D/$log" >&2; exit 1; }
  echo "done: $log"
}
step sim.log SIM_OK "$H/sim/ddsim_gun.sh" GEO=$GEO PART=lambda N=$N SEED=$SEED \
     PMIN=${PMIN:-50} PMAX=${PMAX:-100} OUT=$NAME.edm4hep.root
if [ -n "$SIM_ONLY" ]; then
  mkdir -p "$DEST/logs"; cp "$D/$NAME.edm4hep.root" "$DEST/"; cp "$D/sim.log" "$DEST/logs/${NAME}_sim.log"
  echo "copied to $DEST"; exit 0
fi
step reco.log RECO_OK "$H/reco/eicrecon.sh" GEO=$GEO IN=$D/$NAME.edm4hep.root OUT=$D/$NAME.eicreconmain.edm4eic.root
step reco_5keV.log RECO_OK "$H/reco/eicrecon.sh" GEO=$GEO IN=$D/$NAME.edm4hep.root OUT=$D/$NAME.eicreconmain_5keV.edm4eic.root \
     "EXTRA=-PB0TRK:B0TrackerRawHits:threshold=5e-6"
mkdir -p "$DEST/logs"
cp "$D"/$NAME*.root "$DEST/"
for l in sim reco reco_5keV; do cp "$D/$l.log" "$DEST/logs/${NAME}_$l.log"; done
echo "copied to $DEST"
