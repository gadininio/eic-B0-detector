#!/bin/bash
# campaign_fetch.sh: copy the first N files of an ePIC campaign folder from the BNL xrootd server
# to EOS. Runs OUTSIDE eic-shell (xrdcp from LCG). Files already copied are skipped.
#
#   scripts/production/campaign_fetch.sh <DIR> <N> <DEST>
#     DIR   folder on root://epicxrd1.sdcc.bnl.gov:1095, e.g.
#           /eic/EPIC/RECO/26.07.3/epic_craterlake/SIDIS/pythia6-lhapdf/1.1.0/ep_noradcor/9x130/x_1e-3to1e-2
#     DEST  e.g. $EIC_DATA/campaign/sidis_9x130
# The JLab server (root://dtn-eic.jlab.org) is not reachable from lxplus (port 1094 closed).
# Used 2026-11-12 for the 9x130 SIDIS cross-check (100 files of 199 events, ~75 MB each):
#   scripts/production/campaign_fetch.sh /eic/EPIC/RECO/26.07.3/epic_craterlake/SIDIS/pythia6-lhapdf/1.1.0/ep_noradcor/9x130/x_1e-3to1e-2 100 $EIC_DATA/campaign/sidis_9x130
set -e
H=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$H/env.sh"
source "$EIC_ROOT_SETUP" > /dev/null 2>&1
X=root://epicxrd1.sdcc.bnl.gov:1095
DIR=$1; N=$2; DEST=$3
[ -n "$DEST" ] || { sed -n 2,12p "$0"; exit 1; }
mkdir -p "$DEST"
xrdfs $X ls "$DIR" | grep "\.root$" | sort | head -n "$N" > "$DEST/files.txt"
n=0
while read -r f; do
  b=$(basename "$f")
  [ -s "$DEST/$b" ] || xrdcp -s "$X/$f" "$DEST/$b"
  n=$((n + 1))
done < "$DEST/files.txt"
echo "FETCH_OK $n files in $DEST"
