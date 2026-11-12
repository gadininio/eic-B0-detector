# gen_dis_lambda.sh: Lambda-tagged DIS events with Pythia8 (runs INSIDE eic-shell).
# Run it with scripts/in_eic_shell.sh, which sets the variables:
#
#   N      number of KEPT events (with a Lambda at theta < THMAX), default 100
#   SEED   random seed (default 1)
#   OUT    output HepMC3 file, written in $EIC_WORK/<OUT without .hepmc>/
#   EE, EP beam energies in GeV (default 18, 275); Q2MIN in GeV^2 (default 1); THMAX in mrad (25)
#
# Compiles scripts/gen/pythia8_dis_lambda.cc once into $EIC_WORK/bin. (pythia8-config reads an
# environment variable OUT, hence "env -u OUT".)
# 18x275, Q2 > 1: 7 % of the events are kept (3 % have a Lambda in 5.5-20 mrad), ~10 ms/event.
set -e
set --
N=${N:-100}; SEED=${SEED:-1}; OUT=${OUT:-dis_lambda_s$SEED.hepmc}
SRC=$EIC_REPO/scripts/gen/pythia8_dis_lambda.cc
BIN=$EIC_WORK/bin/pythia8_dis_lambda
mkdir -p "$EIC_WORK/bin"
if [ ! -x "$BIN" ] || [ "$SRC" -nt "$BIN" ]; then
  g++ $(env -u OUT pythia8-config --cxxflags) -I/opt/local/include "$SRC" -o "$BIN" $(env -u OUT pythia8-config --libs) \
      -L/opt/local/lib -lHepMC3
fi
JOB=$EIC_WORK/${OUT%.hepmc}
mkdir -p "$JOB" && cd "$JOB"
"$BIN" "$JOB/$OUT" "$N" "$SEED" "${EE:-18}" "${EP:-275}" "${Q2MIN:-1}" "${THMAX:-25}"
