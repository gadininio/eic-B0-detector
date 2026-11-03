// lambda_mass_notruth.C
//
// Lambda invariant mass from reconstructed B0 tracks WITHOUT truth in the selection.
//
// Candidates: every (+,-) pair of ReconstructedTruthSeededChargedParticles in an event, with
// the + track as the proton and the - track as the pi- (PDG mass hypotheses).
// Selection: p(+)/p(-) > minRatio. In Lambda -> p pi- the momentum ratio of a true pair is
// between 3.09 and 14.6 for any Lambda momentum (two-body kinematics, see lambda_ratio_truth.C),
// so minRatio = 2.5 leaves room for momentum mismeasurement.
// Truth is used only afterwards, to label each candidate as signal (+ matched to the proton
// and - to the pi- of the same Lambda -> p pi- decay) or background, and to quote
//   purity     = signal candidates / all selected candidates
//   efficiency = signal candidates kept by the cut / signal candidates before the cut.
// Note: the tracks themselves still come from truth seeding.
//
// Input: EICrecon output files (wildcards combine files).
// Output: printout and <outbase>.png: all pairs and pairs after the cut, signal stacked on
// background, Gaussian fit of the core after the cut.
// Usage (from the repository top):
//   root -l -b -q 'macros/mass/lambda_mass_notruth.C+("/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconmain_5keV.edm4eic.root","results/06_mass/mass_notruth_new_5keV")'
#include "../common/LambdaB0.h"
#include "TLorentzVector.h"
#include "TH1D.h"
#include "THStack.h"
#include "TF1.h"
#include <cstdio>

void lambda_mass_notruth(const char* files = "/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconmain_5keV.edm4eic.root",
                         const char* outbase = "mass_notruth_new_5keV", double minRatio = 2.5)
{
  using namespace lb0;
  TChain* ch = MakeChain(files);
  TTreeReader r(ch);
  MCEvent ev(r);
  RecoEvent reco(r);

  const int nb = 100; const double lo = 1.05, hi = 1.35;   // 3 MeV bins
  TH1D hAllS("hAllS", "", nb, lo, hi), hAllB("hAllB", "", nb, lo, hi);
  TH1D hCutS("hCutS", "", nb, lo, hi), hCutB("hCutB", "", nb, lo, hi), hCut("hCut", "", nb, lo, hi);
  int nAll = 0, nAllS = 0, nCut = 0, nCutS = 0, nEvCand = 0;

  while (r.Next()) {
    int n = reco.N();
    if (n < 2) continue;
    std::vector<int> mc = reco.BestMC();   // only for labelling
    bool any = false;
    for (int a = 0; a < n; ++a) for (int b = 0; b < n; ++b) {
      if (!(reco.q[a] > 0 && reco.q[b] < 0)) continue;
      TLorentzVector vp, vpi;
      vp.SetVectM(reco.P(a), kMassProton); vpi.SetVectM(reco.P(b), kMassPion);
      double M = (vp + vpi).M();
      int lp = ev.LambdaOf(mc[a], 2212), lpi = ev.LambdaOf(mc[b], -211);
      bool sig = lp >= 0 && lp == lpi;
      ++nAll; nAllS += sig; (sig ? hAllS : hAllB).Fill(M);
      if (vp.P() / vpi.P() > minRatio) { ++nCut; nCutS += sig; any = true; (sig ? hCutS : hCutB).Fill(M); hCut.Fill(M); }
    }
    nEvCand += any;
  }

  TF1 g("g", "gaus", kMassLambda - 0.015, kMassLambda + 0.015);
  hCut.Fit(&g, "QR0");
  printf("%s  (%lld events)\n", files, ch->GetEntries());
  printf("all (+,-) pairs:          %4d   signal %4d   purity %5.1f%%\n", nAll, nAllS, Pct(nAllS, nAll));
  printf("pairs with p+/p- > %.1f:   %4d   signal %4d   purity %5.1f%%   (events with a candidate: %d)\n",
         minRatio, nCut, nCutS, Pct(nCutS, nCut), nEvCand);
  printf("efficiency of the cut for signal: %d of %d (%.1f%%)\n", nCutS, nAllS, Pct(nCutS, nAllS));
  printf("Gaussian fit after cut (+-15 MeV): mean %.4f +- %.4f GeV  sigma %.4f +- %.4f GeV\n",
         g.GetParameter(1), g.GetParError(1), g.GetParameter(2), g.GetParError(2));
  int b1 = hCut.FindBin(kMassLambda - 0.02), b2 = hCut.FindBin(kMassLambda + 0.02);
  printf("in +-20 MeV window after cut: %.0f candidates, %.0f signal\n", hCut.Integral(b1, b2), hCutS.Integral(b1, b2));

  // signal (green) stacked on background (grey)
  for (TH1D* h : {&hAllS, &hCutS}) { h->SetFillColor(kGreen - 6); h->SetLineColor(kGreen + 2); }
  for (TH1D* h : {&hAllB, &hCutB}) { h->SetFillColor(kGray); h->SetLineColor(kGray + 2); }
  THStack sAll("sAll", "all (+,-) pairs, no truth in selection;M(p#pi^{-}) [GeV];pairs / 3 MeV");
  sAll.Add(&hAllB); sAll.Add(&hAllS);
  THStack sCut("sCut", TString::Format("p(+)/p(-) > %.1f;M(p#pi^{-}) [GeV];pairs / 3 MeV", minRatio));
  sCut.Add(&hCutB); sCut.Add(&hCutS);
  TCanvas c("c", "", 1300, 500); c.Divide(2, 1);
  TLegend leg(0.55, 0.7, 0.88, 0.88);
  leg.AddEntry(&hAllS, "signal (truth label)", "f"); leg.AddEntry(&hAllB, "background", "f");
  c.cd(1); sAll.Draw("hist"); leg.Draw();
  c.cd(2); sCut.Draw("hist"); g.SetLineColor(kRed); g.Draw("same"); leg.Draw();
  c.SaveAs(TString(outbase) + ".png");
}
