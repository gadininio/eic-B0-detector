# ddsim_gun.sh: particle-gun simulation into the B0 region (runs INSIDE eic-shell).
# Run it with scripts/in_eic_shell.sh, which sets the variables:
#
#   GEO    container = epic of the eic-shell container (/opt/detector/epic-main)
#          local     = local epic build in $EIC_TOP/epic/install
#   PART   lambda (default) or geantino (geometric acceptance; straight lines, no interactions)
#   N      number of events (default 100)
#   SEED   random seed (default 42)
#   PMIN, PMAX  momentum range in GeV (default 50, 100)
#   OUT    output file name, written in $EIC_WORK/<OUT without .edm4hep.root>/ (own folder per
#          job: the geometry downloads calibrations/ into the current folder, and jobs sharing a
#          folder can read each other's half-written files)
#
# Gun: theta 8-18 mrad uniform, phi uniform, 18x275 craterlake geometry, 25 mrad crossing angle.
# For geantinos the tracker hit filter is switched off (--filter.tracker ""): the default
# "edep1kev" drops hits below 1 keV, i.e. all geantino hits. ("None" is not accepted.)
set -e
set --
PART=${PART:-lambda}; N=${N:-100}; SEED=${SEED:-42}; PMIN=${PMIN:-50}; PMAX=${PMAX:-100}
OUT=${OUT:-${PART}_${GEO}_s${SEED}.edm4hep.root}
JOB=$EIC_WORK/${OUT%.edm4hep.root}
mkdir -p "$JOB" && cd "$JOB"
if [ "$GEO" = local ]; then source "$EIC_TOP/epic/install/bin/thisepic.sh"
elif [ "$GEO" = container ]; then source /opt/detector/epic-main/bin/thisepic.sh
else echo "GEO must be container or local" >&2; exit 1; fi
GEOM="$DETECTOR_PATH/epic_craterlake_18x275.xml"
echo "DETECTOR_PATH=$DETECTOR_PATH"
# geantino: keep zero-energy hits; Lambda: keep the MC history of particles made in the trackers
OPTS=(--part.userParticleHandler "Geant4TVUserParticleHandler")
[ "$PART" = geantino ] && OPTS=(--filter.tracker "")
ddsim \
    --compactFile "$GEOM" \
    --numberOfEvents "$N" \
    --enableGun \
    --gun.particle "$PART" \
    --gun.momentumMin "${PMIN}*GeV" \
    --gun.momentumMax "${PMAX}*GeV" \
    --gun.thetaMin "8*mrad" \
    --gun.thetaMax "18*mrad" \
    --gun.distribution uniform \
    --crossingAngleBoost -0.025 \
    --random.seed "$SEED" \
    "${OPTS[@]}" \
    --outputFile "$JOB/$OUT"
echo SIM_OK
