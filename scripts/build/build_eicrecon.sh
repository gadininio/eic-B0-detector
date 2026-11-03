#!/bin/bash
# build_eicrecon.sh: build EICrecon main in $EIC_TOP/EICrecon. Runs OUTSIDE eic-shell (builds
# inside it). build/ ~140 MB, install/ ~50 MB, ~20 min with -j8. Used by scripts/reco/eicrecon.sh
# with RECO=main.
# EICrecon main at the time of this study: 8b44cae, which contains #2901 (dedicated B0 truth
# seeder B0TrackerTruthSeeds); the eic-shell nightly had a7bab3f, without it.
# A shallow clone has no tags, so "eicrecon --version" prints 0.0.0-unknown.
set -e
H=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$H/env.sh"
cd "$EIC_TOP"
[ -d EICrecon ] || git clone --depth 1 https://github.com/eic/EICrecon.git EICrecon
git -C EICrecon log -1 --format='EICrecon %h %cd'
printf '%s\n' "set --" "cd $EIC_TOP/EICrecon" \
  "cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install -DCMAKE_BUILD_TYPE=Release && cmake --build build -j8 && cmake --install build && echo BUILD_OK" \
  | "$EIC_TOP/eic-shell" > "$EIC_TOP/EICrecon/build.log" 2>&1 || true
grep -q BUILD_OK "$EIC_TOP/EICrecon/build.log" && echo "EICrecon built: source $EIC_TOP/EICrecon/install/bin/eicrecon-this.sh" \
  || { echo "FAILED, see $EIC_TOP/EICrecon/build.log" >&2; exit 1; }
