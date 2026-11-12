// pythia8_dis_lambda.cc: Pythia8 neutral-current DIS (ep, head-on, proton along +z), keeping only
// events with a Lambda (pdg 3122, not anti-Lambda) at theta < THMAX mrad, written as HepMC3.
//
//   pythia8_dis_lambda <out.hepmc> <nAccepted> <seed> [eE=18] [eP=275] [Q2min=1] [thMax=25]
//
// Settings as in the ePIC NC DIS campaign (ParticleDecays:limitTau0 with 10 mm keeps the Lambda
// stable, so Geant4 decays it). The 25 mrad crossing angle is applied later by ddsim
// (--crossingAngleBoost); no beam divergence / vertex spread. B0 = about 5.5-20 mrad.
#include "Pythia8/Pythia.h"
#include "Pythia8Plugins/HepMC3.h"
#include <cmath>
#include <cstdio>
#include <string>
using namespace Pythia8;

int main(int argc, char** argv) {
  if (argc < 4) { printf("usage: %s out.hepmc nAccepted seed [eE eP Q2min thMax]\n", argv[0]); return 1; }
  std::string out = argv[1];
  long nAcc = atol(argv[2]);
  int seed = atoi(argv[3]);
  double eE = argc > 4 ? atof(argv[4]) : 18, eP = argc > 5 ? atof(argv[5]) : 275;
  double q2min = argc > 6 ? atof(argv[6]) : 1, thMax = (argc > 7 ? atof(argv[7]) : 25) * 1e-3;

  Pythia py;
  py.readString("Beams:frameType = 2");
  py.readString("Beams:idA = 2212");
  py.readString("Beams:idB = 11");
  py.settings.parm("Beams:eA", eP);
  py.settings.parm("Beams:eB", eE);
  py.readString("WeakBosonExchange:ff2ff(t:gmZ) = on");
  py.settings.parm("PhaseSpace:Q2Min", q2min);
  py.readString("SpaceShower:dipoleRecoil = on");
  py.readString("SpaceShower:pTmaxMatch = 2");
  py.readString("PDF:lepton = off");
  py.readString("TimeShower:QEDshowerByL = off");
  py.readString("ParticleDecays:limitTau0 = on");
  py.readString("ParticleDecays:tau0Max = 10");
  py.readString("Random:setSeed = on");
  py.settings.mode("Random:seed", seed);
  py.readString("Next:numberCount = 0");
  if (!py.init()) return 1;

  Pythia8ToHepMC toHepMC(out);
  long nGen = 0, nKept = 0, nLam = 0;
  while (nKept < nAcc) {
    if (!py.next()) continue;
    ++nGen;
    bool keep = false;
    for (int i = 0; i < py.event.size(); ++i) {
      const Particle& p = py.event[i];
      if (!p.isFinal() || p.id() != 3122) continue;
      ++nLam;
      if (p.theta() < thMax) keep = true;
    }
    if (!keep) continue;
    toHepMC.writeNextEvent(py);
    ++nKept;
  }
  py.stat();
  printf("GEN_SUMMARY generated %ld kept %ld (%.4f) lambdas/event %.4f sigma %.4g mb\n", nGen, nKept,
         double(nKept) / nGen, double(nLam) / nGen, py.info.sigmaGen());
  printf("GEN_OK\n");
  return 0;
}
