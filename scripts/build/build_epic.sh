#!/bin/bash
# build_epic.sh: build epic (the ePIC detector geometry) in $EIC_TOP/<dir>, at main or at a
# given commit. Runs OUTSIDE eic-shell (builds inside it). ~100 MB, ~10 min.
# Use it in later jobs with GEO=local (dir epic) or GEO=<dir>.
#
#   scripts/build/build_epic.sh                          # main -> $EIC_TOP/epic
#   scripts/build/build_epic.sh epic_43f427e 43f427e736223b963d02adefcda44bfa225d577b
#                                                        # commit (full hash) -> $EIC_TOP/epic_43f427e
#
# Geometries of this study:
#   old  epic 43f427e: in the eic-shell nightly of 2026-11-02, the commit just before eic/epic#1194;
#        no release tag. From the nightly of 2026-11-07 on, the container has the new geometry,
#        so the old one needs this build (dir epic_43f427e).
#   new  epic ec774d8: merge of eic/epic#1194 (realistic B0) = release 26.11.0 (in the container
#        as /opt/detector/epic-26.11.0 since 2026-11-07; GEO=epic-26.11.0).
set -e
H=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$H/env.sh"
DIR=${1:-epic}
REF=$2
cd "$EIC_TOP"
if [ ! -d "$DIR" ]; then
  git clone --depth 1 https://github.com/eic/epic.git "$DIR"
  if [ -n "$REF" ]; then
    git -C "$DIR" fetch --depth 1 origin "$REF"
    git -C "$DIR" checkout -q FETCH_HEAD
  fi
fi
git -C "$DIR" log -1 --format="$DIR %h %cd %s"
printf '%s\n' "set --" "cd $EIC_TOP/$DIR" \
  "cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install && cmake --build build -j8 && cmake --install build && echo BUILD_OK" \
  | "$EIC_TOP/eic-shell" > "$EIC_TOP/$DIR/build.log" 2>&1 || true
grep -q BUILD_OK "$EIC_TOP/$DIR/build.log" && echo "epic built: source $EIC_TOP/$DIR/install/bin/thisepic.sh" \
  || { echo "FAILED, see $EIC_TOP/$DIR/build.log" >&2; exit 1; }
