#!/bin/bash
# make_results.sh: run all analysis macros on the samples in $EIC_DATA and write every plot
# (png) and printout (txt) into results/. Runs OUTSIDE eic-shell, with ROOT from LCG.
#   scripts/make_results.sh            # everything (~15 min, mostly reading from EOS)
#   scripts/make_results.sh 04 06      # only results/04_* and results/06_*
# Samples (see README.md):
#   $EIC_DATA/lambda_100ev/    lambda_{old,new}_s42.*        first 100-event samples
#   $EIC_DATA/lambda_1000ev/   lambda_{old,new}_s10{1,2}.*   2 x 500 events per geometry
#                              (.eicreconpr3116.: EICrecon#3116, hit seeding; results/07)
#   $EIC_DATA/geantino/        geantino_{old,new}_5000.*
H=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source "$H/scripts/env.sh"
source "$EIC_ROOT_SETUP" > /dev/null 2>&1
cd "$EIC_REPO"
S=$EIC_DATA/lambda_100ev; L=$EIC_DATA/lambda_1000ev; G=$EIC_DATA/geantino
want() { [ $# -eq 0 ] || [ -z "$ONLY" ] || [[ " $ONLY " == *" ${1:0:2} "* ]]; }
ONLY="$*"

# run <macro> <output base (also the .txt)> <args...>: compiled macro, printout to <output>.txt
run() {
  local macro=$1 out=$2; shift 2
  local args="\"$1\""; shift
  for a in "$@"; do args+=",$a"; done
  mkdir -p "$(dirname "$out")"
  echo "== $out"
  root -l -b -q "$macro+($args)" 2>&1 \
    | grep -vE "no dictionary for class|^Processing|ACLiC|^Info in <TCanvas|TNetXNGFile|SetEntryBase|SetLocalEntry|^$" \
    > "$out.txt"
  tail -n +1 "$out.txt" | head -40
}
# geometry labels: old = epic 43f427e (before eic/epic#1194), new = epic main ec774d8 (with it)

if want 01; then
  R=results/01_first_look
  for g in old new; do
    run macros/exploration/b0_hit_origins.C $R/b0_hit_origins_$g "$S/lambda_${g}_s42.edm4hep.root"
  done
  run macros/exploration/compare_tracks.C $R/tracks_containerreco_old_vs_new \
      "$S/lambda_old_s42.containerreco.edm4eic.root" "\"$S/lambda_new_s42.containerreco.edm4eic.root\"" '"TrackerTruthSeeds"' '"TrackerTruthSeeds"'
  for g in old new; do
    run macros/exploration/compare_tracks.C $R/tracks_${g}_containerreco_vs_eicreconmain \
        "$S/lambda_${g}_s42.containerreco.edm4eic.root" "\"$S/lambda_${g}_s42.eicreconmain.edm4eic.root\"" '"TrackerTruthSeeds"' '"B0TrackerTruthSeeds"'
  done
fi
if want 02; then
  for g in old new; do
    run macros/acceptance/b0_geometric_acceptance.C results/02_geometric_acceptance/geometric_acceptance_$g "$G/geantino_${g}_5000.edm4hep.root" '"results/02_geometric_acceptance/geometric_acceptance_'$g'"'
  done
fi
if want 03; then
  for g in old new; do
    run macros/acceptance/lambda_daughter_acceptance.C results/03_daughter_acceptance/daughter_acceptance_$g "$L/lambda_${g}_s10?.edm4hep.root" '"results/03_daughter_acceptance/daughter_acceptance_'$g'"'
  done
fi
for t in "" _5keV; do
  for g in old new; do
    f="$L/lambda_${g}_s10?.eicreconmain$t.edm4eic.root"; tag=${g}${t:-_10keV}
    want 04 && run macros/efficiency/lambda_tracking_efficiency.C results/04_tracking_efficiency/tracking_efficiency_$tag "$f" '"results/04_tracking_efficiency/tracking_efficiency_'$tag'"'
    want 05 && run macros/efficiency/lambda_acceptance_x_efficiency.C results/05_acceptance_x_efficiency/acceptance_x_efficiency_$tag "$f" '"results/05_acceptance_x_efficiency/acceptance_x_efficiency_'$tag'"'
    if want 06; then
      run macros/mass/lambda_mass_reco.C results/06_mass/mass_reco_$tag "$f" '"results/06_mass/mass_reco_'$tag'"'
      run macros/mass/lambda_mass_notruth.C results/06_mass/mass_notruth_$tag "$f" '"results/06_mass/mass_notruth_'$tag'"'
      run macros/mass/lambda_ratio_truth.C results/06_mass/ratio_$tag "$f" '"results/06_mass/ratio_'$tag'"'
    fi
  done
done
if want 06; then
  for g in old new; do
    run macros/mass/lambda_mass_truth.C results/06_mass/mass_truth_$g "$L/lambda_${g}_s10?.edm4hep.root" '"results/06_mass/mass_truth_'$g'"'
  done
fi
# 07: hit seeding of the B0 tracks (EICrecon#3116, built from its branch at 1ca147bc; 5 keV
# threshold by default). Truth and hit seeding come from the same files, i.e. the same events.
if want 07; then
  R=results/07_hit_seeding
  for g in old new; do
    f="$L/lambda_${g}_s10?.eicreconpr3116.edm4eic.root"
    for s in truth hit; do
      tag=${g}_$s
      run macros/efficiency/lambda_tracking_efficiency.C $R/tracking_efficiency_$tag "$f" "\"$R/tracking_efficiency_$tag\"" 3 "\"$s\""
      run macros/efficiency/lambda_acceptance_x_efficiency.C $R/acceptance_x_efficiency_$tag "$f" "\"$R/acceptance_x_efficiency_$tag\"" 3 "\"$s\""
      run macros/mass/lambda_mass_reco.C $R/mass_reco_$tag "$f" "\"$R/mass_reco_$tag\"" 0 "\"$s\""
      run macros/mass/lambda_mass_notruth.C $R/mass_notruth_$tag "$f" "\"$R/mass_notruth_$tag\"" 2.5 "\"$s\""
    done
    run macros/mass/lambda_mass_seeding_compare.C $R/mass_seeding_compare_$g "$f" "\"$R/mass_seeding_compare_$g\""
  done
fi
