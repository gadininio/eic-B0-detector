#!/bin/bash
# build_epic.sh: build epic (the ePIC detector geometry) main in $EIC_TOP/epic. Runs OUTSIDE
# eic-shell (builds inside it). ~100 MB. Use it in later jobs with GEO=local.
#   scripts/build/build_epic.sh            # fresh shallow clone of main, or rebuild if present
# epic main at the time of this study: ec774d8 (merge of eic/epic PR #1194, realistic B0 geometry).
set -e
H=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$H/env.sh"
cd "$EIC_TOP"
[ -d epic ] || git clone --depth 1 https://github.com/eic/epic.git epic
git -C epic log -1 --format='epic %h %cd'
printf '%s\n' "set --" "cd $EIC_TOP/epic" \
  "cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install && cmake --build build -j8 && cmake --install build && echo BUILD_OK" \
  | "$EIC_TOP/eic-shell" > "$EIC_TOP/epic/build.log" 2>&1 || true
grep -q BUILD_OK "$EIC_TOP/epic/build.log" && echo "epic built: source $EIC_TOP/epic/install/bin/thisepic.sh" \
  || { echo "FAILED, see $EIC_TOP/epic/build.log" >&2; exit 1; }
