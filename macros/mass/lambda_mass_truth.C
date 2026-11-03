// lambda_mass_truth.C
//
// Lambda invariant mass from the TRUE momenta of its daughters (sanity check of the sample).
//
// For each generated Lambda: decayed to p pi- (exactly 2 daughters), to n pi0, or other
// (interacted in material / left the world volume). For p pi- decays, M(p pi-) from the
// daughters' true momenta at their production vertex; it must equal the PDG mass exactly.
//
// Input: ddsim (edm4hep) or EICrecon (edm4eic) files (wildcards combine files).
// Output: printout and <outbase>.png.
// Usage (from the repository top):
//   root -l -b -q 'macros/mass/lambda_mass_truth.C+("/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.edm4hep.root","results/06_mass/mass_truth_new")'
#include "../common/LambdaB0.h"
#include "TLorentzVector.h"
#include "TH1D.h"
#include <cstdio>

void lambda_mass_truth(const char* files = "/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.edm4hep.root",
                       const char* outbase = "mass_truth_new")
{
  using namespace lb0;
  TChain* ch = MakeChain(files);
  TTreeReader r(ch);
  MCEvent ev(r);
  TTreeReaderArray<double> mass(r, "MCParticles.mass");

  TH1D h("h", "M(p#pi^{-}) from true momenta;M [GeV];decays", 100, 1.10, 1.13);
  int nPPi = 0, nNPi0 = 0, nOther = 0;
  auto p4 = [&](int i) { TLorentzVector v; v.SetVectM(ev.P(i), mass[i]); return v; };
  while (r.Next()) {
    for (auto& L : ev.Lambdas()) {
      if (L.fate == MCEvent::kPPi) { ++nPPi; h.Fill((p4(L.p) + p4(L.pi)).M()); }
      else if (L.fate == MCEvent::kNPi0) ++nNPi0;
      else ++nOther;
    }
  }
  int n = nPPi + nNPi0 + nOther;
  printf("%s  (%lld events)\n", files, ch->GetEntries());
  printf("generated Lambda: %d   -> p pi-: %d (%.0f%%)   -> n pi0: %d (%.0f%%)   other: %d (%.0f%%)\n",
         n, nPPi, Pct(nPPi, n), nNPi0, Pct(nNPi0, n), nOther, Pct(nOther, n));
  printf("M(p pi-) from true momenta: mean %.6f GeV, RMS %.2e GeV (PDG %.6f GeV)\n", h.GetMean(), h.GetRMS(), kMassLambda);
  TCanvas c("c", "", 700, 500);
  h.Draw();
  c.SaveAs(TString(outbase) + ".png");
}
