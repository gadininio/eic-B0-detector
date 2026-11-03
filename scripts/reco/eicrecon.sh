# eicrecon.sh: reconstruction with EICrecon (runs INSIDE eic-shell).
# Run it with scripts/in_eic_shell.sh, which sets the variables:
#
#   GEO      container | local: which epic geometry (must be the one used in the simulation)
#   RECO     main (default) = local EICrecon build in $EIC_TOP/EICrecon/install
#            container      = EICrecon of the eic-shell container
#   IN       input edm4hep file (absolute path, on a disk visible in the container)
#   OUT      output edm4eic file (absolute path)
#   EXTRA    more eicrecon options, e.g.
#              -PB0TRK:B0TrackerRawHits:threshold=5e-6                (B0 threshold 5 keV; GeV units)
#              -Ptracking:B0TrackerTruthSeeds:MomentumSmear=0         (no seed momentum smearing)
#            Parameter keys need the plugin prefix ("tracking:", "B0TRK:"); an unknown key makes
#            eicrecon stop at start-up ("appears to be unused") and leaves an empty output file.
set -e
set --
RECO=${RECO:-main}
JOB=$(dirname "$OUT")/work_$(basename "${OUT%.edm4eic.root}")
mkdir -p "$JOB" && cd "$JOB"
if [ "$GEO" = local ]; then source "$EIC_TOP/epic/install/bin/thisepic.sh"
elif [ "$GEO" = container ]; then source /opt/detector/epic-main/bin/thisepic.sh
else echo "GEO must be container or local" >&2; exit 1; fi
[ "$RECO" = main ] && source "$EIC_TOP/EICrecon/install/bin/eicrecon-this.sh"
echo "DETECTOR_PATH=$DETECTOR_PATH"
command -v eicrecon            # "which" in the container fails under set -e
GEOM="$DETECTOR_PATH/epic_craterlake_18x275.xml"
eicrecon -Pjana:nevents=-1 -Pdd4hep:xml_files="$GEOM" $EXTRA -Ppodio:output_file="$OUT" "$IN"
echo RECO_OK
