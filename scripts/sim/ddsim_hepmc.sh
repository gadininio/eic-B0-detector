# ddsim_hepmc.sh: full simulation of generator events from a HepMC3 file (runs INSIDE eic-shell).
# Run it with scripts/in_eic_shell.sh, which sets the variables:
#
#   GEO    epic geometry, as in scripts/sim/ddsim_gun.sh (container | local | <name>)
#   IN     input HepMC3 file (absolute path, visible in the container)
#   N      number of events (default -1 = all)
#   OUT    output file name, written in $EIC_WORK/<OUT without .edm4hep.root>/
#   BEAM   beam configuration of the geometry, epic_craterlake_<BEAM>.xml (default 18x275; also
#          9x130, 10x275, ...); must match the generator energies
#
# The generator is head-on, ddsim applies the 25 mrad crossing angle.
# No beam divergence or vertex spread (no afterburner).
set -e
set --
N=${N:--1}
JOB=$EIC_WORK/${OUT%.edm4hep.root}
mkdir -p "$JOB" && cd "$JOB"
if [ "$GEO" = local ]; then source "$EIC_TOP/epic/install/bin/thisepic.sh"
elif [ "$GEO" = container ]; then source /opt/detector/epic-main/bin/thisepic.sh
elif [ -f "/opt/detector/$GEO/bin/thisepic.sh" ]; then source "/opt/detector/$GEO/bin/thisepic.sh"
elif [ -f "$EIC_TOP/$GEO/install/bin/thisepic.sh" ]; then source "$EIC_TOP/$GEO/install/bin/thisepic.sh"
else echo "GEO=$GEO: no such epic (container, local, /opt/detector/<GEO>, \$EIC_TOP/<GEO>/install)" >&2; exit 1; fi
GEOM="$DETECTOR_PATH/epic_craterlake_${BEAM:-18x275}.xml"
echo "DETECTOR_PATH=$DETECTOR_PATH"
ddsim \
    --compactFile "$GEOM" \
    --inputFiles "$IN" \
    --numberOfEvents "$N" \
    --crossingAngleBoost -0.025 \
    --part.userParticleHandler "Geant4TVUserParticleHandler" \
    --outputFile "$JOB/$OUT"
echo SIM_OK
