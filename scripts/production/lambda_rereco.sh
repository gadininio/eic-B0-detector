#!/bin/bash
# lambda_rereco.sh: reconstruct an existing simulated Lambda sample again, with another EICrecon
# build or other options, and copy the output to EOS. Runs OUTSIDE eic-shell.
#
#   scripts/production/lambda_rereco.sh <GEO> <NAME> <RECO> <TAG> [EXTRA] [DEST]
#     GEO    geometry of the simulation (scripts/sim/ddsim_gun.sh): old = epic_43f427e, new = local
#     NAME   sample, e.g. lambda_new_s101: reads DEST/NAME.edm4hep.root
#     RECO   EICrecon build (scripts/reco/eicrecon.sh), e.g. EICrecon_pr3116
#     TAG    output DEST/NAME.TAG.edm4eic.root, log DEST/logs/NAME_TAG.log
#     EXTRA  more eicrecon options ("" for none)
#     DEST   default $EIC_DATA/lambda_1000ev
#
# Hit seeding (EICrecon#3116, 5 keV threshold by default since #3084):
#   for s in 101 102; do
#     scripts/production/lambda_rereco.sh epic_43f427e lambda_old_s$s EICrecon_pr3116 eicreconpr3116 &
#     scripts/production/lambda_rereco.sh local        lambda_new_s$s EICrecon_pr3116 eicreconpr3116 &
#   done; wait
# Stay on the same lxplus node until it is done: the job writes to that node's /tmp.
set -e
H=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$H/env.sh"
GEO=$1; NAME=$2; RECO=$3; TAG=$4; EXTRA=$5; DEST=${6:-$EIC_DATA/lambda_1000ev}
[ -n "$TAG" ] || { sed -n 2,19p "$0"; exit 1; }
D=$EIC_WORK/${NAME}_$TAG
mkdir -p "$D"
cp "$DEST/$NAME.edm4hep.root" "$D/"            # EOS is not visible inside eic-shell
"$H/in_eic_shell.sh" "$H/reco/eicrecon.sh" GEO=$GEO RECO=$RECO IN=$D/$NAME.edm4hep.root \
    OUT=$D/$NAME.$TAG.edm4eic.root "EXTRA=$EXTRA" > "$D/reco.log" 2>&1 || true
grep -q RECO_OK "$D/reco.log" || { echo "FAILED: $D/reco.log" >&2; exit 1; }
mkdir -p "$DEST/logs"
cp "$D/$NAME.$TAG.edm4eic.root" "$DEST/"
cp "$D/reco.log" "$DEST/logs/${NAME}_$TAG.log"
rm "$D/$NAME.edm4hep.root"
echo "copied $NAME.$TAG.edm4eic.root to $DEST"
