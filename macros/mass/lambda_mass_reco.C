// lambda_mass_reco.C
//
// Lambda invariant mass from RECONSTRUCTED B0 tracks, with truth matching.
//
// Each reconstructed particle (ReconstructedTruthSeededChargedParticles) is linked to its best
// truth match (ReconstructedTruthSeededChargedParticleAssociations, highest weight) and
// classified: proton from a Lambda -> p pi- decay, pi- from it, or other (shower secondaries,
// products of a Lambda interaction in material). For each decay with both daughters
// reconstructed: M(p pi-) from the reconstructed momenta with PDG mass hypotheses.
// For comparison, all (+,-) pairs without any selection (+ = proton, - = pi-), and the momentum
// resolution of the matched daughters. A version without truth in the selection is
// lambda_mass_notruth.C.
//
// Input: EICrecon output files (wildcards combine files). verbose = 1 prints every track.
// Output: printout (counts, mean/RMS, Gaussian fit of the core if >= 30 pairs) and <outbase>.png.
// Usage (from the repository top):
//   root -l -b -q 'macros/mass/lambda_mass_reco.C+("/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconmain_5keV.edm4eic.root","results/06_mass/mass_reco_new_5keV")'
// Seeding: "truth" (default, ReconstructedTruthSeededChargedParticles) or "hit"
// (ReconstructedChargedParticles, needs EICrecon with #3116); see ../common/LambdaB0.h.
// Usage with hit seeding: add the arguments  ,0,"hit"
#include "../common/LambdaB0.h"
#include "TLorentzVector.h"
#include "TH1D.h"
#include "TF1.h"
#include <cstdio>

void lambda_mass_reco(const char* files = "/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconmain_5keV.edm4eic.root",
                      const char* outbase = "mass_reco_new_5keV", int verbose = 0, const char* seeding = "truth")
{
  using namespace lb0;
  TChain* ch = MakeChain(files);
  TTreeReader r(ch);
  MCEvent ev(r);
  Seeding seed(seeding);
  RecoEvent reco(r, seed);

  TH1D hM("hM", "M(p#pi^{-}), reco momenta, truth-matched;M [GeV];pairs / 2 MeV", 55, 1.06, 1.17);
  TH1D hA("hA", "M(+,-) all pairs, reco momenta (p, #pi^{-} hypotheses);M [GeV];pairs", 60, 1.0, 1.3);
  TH1D hR("hR", "matched #Lambda daughters: (p_{reco}-p_{true})/p_{true};#Deltap/p;tracks", 50, -0.25, 0.25);
  int nTrk = 0, nP = 0, nPi = 0, nOther = 0, nUnmatched = 0, nPairs = 0;
  auto p4 = [&](int k, double m) { TLorentzVector v; v.SetVectM(reco.P(k), m); return v; };

  while (r.Next()) {
    Long64_t entry = r.GetCurrentEntry();
    std::vector<int> mc = reco.BestMC();
    std::vector<int> recoOf = reco.RecoOfMC(ev.pdg.GetSize());
    for (int k = 0; k < reco.N(); ++k) {
      ++nTrk;
      int m = mc[k];
      if (m < 0) { ++nUnmatched; continue; }
      bool isP = ev.LambdaOf(m, 2212) >= 0, isPi = ev.LambdaOf(m, -211) >= 0;
      nP += isP; nPi += isPi; nOther += !isP && !isPi;
      double pt = ev.P(m).Mag(), pr = reco.P(k).Mag();
      if (isP || isPi) hR.Fill((pr - pt) / pt);
      if (verbose) printf("ev %3lld  trk %d  q=%+.0f  p_reco=%6.2f  p_true=%6.2f  PDG=%6d  vtx z=%7.1f mm  %s\n", entry, k,
                          reco.q[k], pr, pt, ev.pdg[m], ev.vz[m], isP ? "p (Lambda)" : isPi ? "pi- (Lambda)" : "other");
    }
    for (auto& L : ev.Lambdas()) {
      if (L.fate != MCEvent::kPPi || recoOf[L.p] < 0 || recoOf[L.pi] < 0) continue;
      double M = (p4(recoOf[L.p], kMassProton) + p4(recoOf[L.pi], kMassPion)).M();
      hM.Fill(M); ++nPairs;
      if (verbose) printf("ev %3lld  ==> truth-matched M(p pi-) = %.4f GeV\n", entry, M);
    }
    for (int a = 0; a < reco.N(); ++a) for (int b = 0; b < reco.N(); ++b)
      if (reco.q[a] > 0 && reco.q[b] < 0) hA.Fill((p4(a, kMassProton) + p4(b, kMassPion)).M());
  }
  printf("%s  (%lld events, %s)\n", files, ch->GetEntries(), seed.Name());
  printf("reco tracks: %d   matched p from Lambda: %d   pi- from Lambda: %d   other: %d   unmatched: %d\n",
         nTrk, nP, nPi, nOther, nUnmatched);
  printf("decays with both daughters reconstructed (truth-matched pairs): %d\n", nPairs);
  printf("M(p pi-) truth-matched: mean %.4f GeV  RMS %.4f GeV in 1.06-1.17 GeV (under/overflow %.0f / %.0f); PDG %.4f\n",
         hM.GetMean(), hM.GetRMS(), hM.GetBinContent(0), hM.GetBinContent(hM.GetNbinsX() + 1), kMassLambda);
  printf("dp/p of matched Lambda daughters: mean %.4f  RMS %.4f\n", hR.GetMean(), hR.GetRMS());
  TF1 g("g", "gaus", kMassLambda - 0.015, kMassLambda + 0.015);
  bool fit = hM.GetEntries() >= 30;
  if (fit) {
    hM.Fit(&g, "QR0");
    printf("Gaussian fit (+-15 MeV): mean %.4f +- %.4f GeV  sigma %.4f +- %.4f GeV\n",
           g.GetParameter(1), g.GetParError(1), g.GetParameter(2), g.GetParError(2));
  }
  TCanvas c("c", "", 1500, 450); c.Divide(3, 1);
  c.cd(1); hM.Draw(); if (fit) { g.SetLineColor(kRed); g.Draw("same"); }
  c.cd(2); hA.Draw();
  c.cd(3); hR.Draw();
  c.SaveAs(TString(outbase) + ".png");
}
