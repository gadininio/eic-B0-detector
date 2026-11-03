// lambda_daughter_acceptance.C
//
// ACCEPTANCE of the B0 tracker for the Lambda -> p pi- daughters, from the full simulation
// (Lambda decay position, B0 dipole field and material all included).
//
//   acceptance = (daughters whose own simulated B0 hits cover >= 3 layers) / (Lambda -> p pi- decays)
//
// counted for the proton, the pi-, and both daughters of the same Lambda. Only simulated hits
// (Geant4, before digitization) are used: no threshold, no tracking. The tracking efficiency
// for accepted daughters is in ../efficiency/lambda_tracking_efficiency.C, and the product
// (decays with reconstructed daughters) in ../efficiency/lambda_acceptance_x_efficiency.C.
//
// The generated Lambdas are first classified (decayed to p pi-, to n pi0, or other = interacted
// in material / left the world volume); the denominator is the p pi- decays.
//
// Input: ddsim (edm4hep) or EICrecon (edm4eic) files of the Lambda gun; wildcards combine files.
// Output: printout and <outbase>.png: acceptance vs Lambda theta and phi (w.r.t. proton beam),
// 2D map vs Lambda theta_x, theta_y (both daughters), and vs Lambda decay z.
// Usage (from the repository top):
//   root -l -b -q 'macros/acceptance/lambda_daughter_acceptance.C+("/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.edm4hep.root","results/03_daughter_acceptance/daughter_acceptance_new")'
#include "../common/LambdaB0.h"
#include <cstdio>

void lambda_daughter_acceptance(const char* files = "/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.edm4hep.root",
                                const char* outbase = "daughter_acceptance_new", int minLayers = lb0::kMinLayers)
{
  using namespace lb0;
  TChain* ch = MakeChain(files);
  TTreeReader r(ch);
  MCEvent ev(r);

  FourPanelPlots plots("acc", TString::Format("accepted (#geq%d layers)", minLayers));
  int nL = 0, nPPi = 0, nNPi0 = 0, nOther = 0, nBefore = 0;
  int nP = 0, nPi = 0, nBoth = 0, nP1 = 0, nPi1 = 0, nBoth1 = 0;

  while (r.Next()) {
    for (auto& L : ev.Lambdas()) {
      ++nL;
      if (L.fate == MCEvent::kNPi0) { ++nNPi0; continue; }
      if (L.fate == MCEvent::kOther) { ++nOther; continue; }
      ++nPPi;
      if (ev.ez[L.idx] < kZFrontB0) ++nBefore;
      int lp = ev.SimLayers(L.p), lpi = ev.SimLayers(L.pi);
      bool ap = lp >= minLayers, api = lpi >= minLayers;
      nP += ap; nPi += api; nBoth += ap && api;
      nP1 += lp >= 1; nPi1 += lpi >= 1; nBoth1 += lp >= 1 && lpi >= 1;
      plots.Fill(ap, api, BeamFrame(ev.P(L.idx)), ev.ez[L.idx] / 1000);
    }
  }
  printf("%s  (%lld events)\n", files, ch->GetEntries());
  printf("generated Lambda: %d   -> p pi-: %d (%.0f%%)   -> n pi0: %d (%.0f%%)   other: %d (%.0f%%)\n",
         nL, nPPi, Pct(nPPi, nL), nNPi0, Pct(nNPi0, nL), nOther, Pct(nOther, nL));
  printf("p pi- decays before the B0 tracker (z < %.0f mm): %d of %d (%.0f%%)\n", kZFrontB0, nBefore, nPPi, Pct(nBefore, nPPi));
  printf("ACCEPTANCE per p pi- decay (>= %d layers):  proton %d (%.1f%%)   pi- %d (%.1f%%)   both %d (%.1f%%)\n",
         minLayers, nP, Pct(nP, nPPi), nPi, Pct(nPi, nPPi), nBoth, Pct(nBoth, nPPi));
  printf("per p pi- decay, >= 1 layer:              proton %d (%.1f%%)   pi- %d (%.1f%%)   both %d (%.1f%%)\n",
         nP1, Pct(nP1, nPPi), nPi1, Pct(nPi1, nPPi), nBoth1, Pct(nBoth1, nPPi));
  printf("both daughters accepted, per generated Lambda: %d of %d (%.1f%%)\n", nBoth, nL, Pct(nBoth, nL));
  plots.Draw(TString(outbase) + ".png");
}
