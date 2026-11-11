#!/bin/bash
# build_eicrecon.sh: build EICrecon (main, or a pull request) in $EIC_TOP/<dir>. Runs OUTSIDE
# eic-shell (builds inside it). build/ ~140 MB, install/ ~50 MB, ~20 min with -j8. Used by
# scripts/reco/eicrecon.sh with RECO=<dir> (RECO=main means dir EICrecon).
#
#   scripts/build/build_eicrecon.sh                       # main  -> $EIC_TOP/EICrecon
#   scripts/build/build_eicrecon.sh EICrecon_pr3116 3116  # PR    -> $EIC_TOP/EICrecon_pr3116
#
# EICrecon main at the time of this study: 8b44cae, which contains #2901 (dedicated B0 truth
# seeder B0TrackerTruthSeeds); the eic-shell nightly had a7bab3f, without it.
# PR #3116 (B0 hit seeding with the Acts triplet seeder, open) was built at 1ca147bc, which is
# main + 6 commits (contains #2901 and #3084, the 5 keV B0 threshold).
# A shallow clone has no tags, so "eicrecon --version" prints 0.0.0-unknown.
set -e
H=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$H/env.sh"
DIR=${1:-EICrecon}
PR=$2
cd "$EIC_TOP"
if [ ! -d "$DIR" ]; then
  git clone --depth 1 https://github.com/eic/EICrecon.git "$DIR"
  if [ -n "$PR" ]; then
    git -C "$DIR" fetch --depth 1 origin "pull/$PR/head"
    git -C "$DIR" checkout -q FETCH_HEAD
  fi
fi
git -C "$DIR" log -1 --format="$DIR %h %cd %s"
printf '%s\n' "set --" "cd $EIC_TOP/$DIR" \
  "cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install -DCMAKE_BUILD_TYPE=Release && cmake --build build -j8 && cmake --install build && echo BUILD_OK" \
  | "$EIC_TOP/eic-shell" > "$EIC_TOP/$DIR/build.log" 2>&1 || true
grep -q BUILD_OK "$EIC_TOP/$DIR/build.log" && echo "EICrecon built: source $EIC_TOP/$DIR/install/bin/eicrecon-this.sh" \
  || { echo "FAILED, see $EIC_TOP/$DIR/build.log" >&2; exit 1; }
