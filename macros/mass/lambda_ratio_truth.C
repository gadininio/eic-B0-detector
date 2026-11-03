// lambda_ratio_truth.C
//
// Check of the p(+)/p(-) > 2.5 cut of lambda_mass_notruth.C.
//
// For every Lambda -> p pi- decay: the TRUE momentum ratio p(p)/p(pi-). Overlaid: the
// expectation for a decay that is isotropic in the Lambda rest frame, in the collinear limit
// (beta = 1, transverse momentum neglected). With theta* the proton angle in the rest frame,
//   p(p) ~ gamma (E*p + p* cos theta*),  p(pi) ~ gamma (E*pi - p* cos theta*),
// gamma cancels and, with cos theta* uniform,
//   f(r) = (E*p + E*pi) / (2 p* (1 + r)^2)   for (E*p - p*)/(E*pi + p*) = 3.09 < r < 14.6.
// For truth-matched reconstructed pairs (EICrecon input): the RECONSTRUCTED ratio, to show
// how momentum mismeasurement smears it towards the cut.
//
// Input: EICrecon output files (wildcards combine files); edm4hep files give the true ratio only.
// Output: printout and <outbase>.png.
// Usage (from the repository top):
//   root -l -b -q 'macros/mass/lambda_ratio_truth.C+("/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconmain_5keV.edm4eic.root","results/06_mass/ratio_new_5keV")'
#include "../common/LambdaB0.h"
#include "TH1D.h"
#include "TF1.h"
#include "TLine.h"
#include <memory>
#include <cstdio>

void lambda_ratio_truth(const char* files = "/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconmain_5keV.edm4eic.root",
                        const char* outbase = "ratio_new_5keV", double cut = 2.5)
{
  using namespace lb0;
  const double mL = kMassLambda, mP = kMassProton, mPi = kMassPion;
  const double pStar = sqrt((mL * mL - (mP + mPi) * (mP + mPi)) * (mL * mL - (mP - mPi) * (mP - mPi))) / (2 * mL);
  const double EP = sqrt(pStar * pStar + mP * mP), EPi = sqrt(pStar * pStar + mPi * mPi);
  const double rMin = (EP - pStar) / (EPi + pStar), rMax = (EP + pStar) / (EPi - pStar);
  const double K = (EP + EPi) / (2 * pStar);

  TChain* ch = MakeChain(files);
  TTreeReader r(ch);
  MCEvent ev(r);
  const bool hasReco = ch->GetBranch("ReconstructedTruthSeededChargedParticleAssociations") != nullptr;
  std::unique_ptr<RecoEvent> reco(hasReco ? new RecoEvent(r) : nullptr);

  TH1D hT("hT", "p(p) / p(#pi^{-}) for #Lambda #rightarrow p#pi^{-};momentum ratio;decays / 0.25", 64, 0, 16);
  TH1D hR("hR", "", 64, 0, 16);
  double tMin = 1e9, tMax = 0, rRecoMax = 0;
  int nR = 0, nRbelow = 0;

  while (r.Next()) {
    std::vector<int> recoOf;
    if (hasReco) recoOf = reco->RecoOfMC(ev.pdg.GetSize());
    for (auto& L : ev.Lambdas()) {
      if (L.fate != MCEvent::kPPi) continue;
      double rt = ev.P(L.p).Mag() / ev.P(L.pi).Mag();
      hT.Fill(rt); tMin = std::min(tMin, rt); tMax = std::max(tMax, rt);
      if (hasReco && recoOf[L.p] >= 0 && recoOf[L.pi] >= 0) {
        double rr = reco->P(recoOf[L.p]).Mag() / reco->P(recoOf[L.pi]).Mag();
        hR.Fill(rr); ++nR; nRbelow += rr <= cut; rRecoMax = std::max(rRecoMax, rr);
      }
    }
  }
  printf("%s  (%lld events)\n", files, ch->GetEntries());
  printf("expected (collinear, isotropic decay): p* = %.4f GeV, ratio from %.2f to %.2f\n", pStar, rMin, rMax);
  printf("true ratio: %.0f decays, min %.2f, max %.2f, mean %.2f (expected mean %.2f)\n", hT.GetEntries(), tMin, tMax,
         hT.GetMean(), K * (log((1 + rMax) / (1 + rMin)) + 1 / (1 + rMax) - 1 / (1 + rMin)));
  if (hasReco) printf("reco ratio (truth-matched pairs): %d pairs, %d at or below the cut %.1f (%.1f%%), max %.2f\n",
                      nR, nRbelow, cut, Pct(nRbelow, nR), rRecoMax);

  // expectation, normalised to the number of true decays
  TF1 f("f", "[0]*[1]/(1+x)^2*(x>[2])*(x<[3])", 0, 16);
  f.SetParameters(hT.GetEntries() * hT.GetBinWidth(1), K, rMin, rMax);
  f.SetNpx(2000); f.SetLineColor(kRed); f.SetLineWidth(2);
  hT.SetLineColor(kBlack); hT.SetLineWidth(2); hT.SetStats(0);
  hR.SetLineColor(kBlue); hR.SetFillColor(kBlue - 9); hR.SetFillStyle(3004);
  TCanvas c("c", "", 900, 600);
  hT.SetMaximum(1.25 * std::max(hT.GetMaximum(), f.GetMaximum()));
  hT.Draw("hist"); if (hasReco) hR.Draw("hist same"); f.Draw("same"); hT.Draw("hist same");
  TLine lc(cut, 0, cut, hT.GetMaximum()); lc.SetLineColor(kMagenta + 1); lc.SetLineStyle(2); lc.SetLineWidth(2); lc.Draw();
  TLegend leg(0.45, 0.62, 0.88, 0.88);
  leg.AddEntry(&hT, "true momenta, all p#pi^{-} decays", "l");
  leg.AddEntry(&f, TString::Format("expected: isotropic decay, %.2f < r < %.2f", rMin, rMax), "l");
  if (hasReco) leg.AddEntry(&hR, "reconstructed momenta, truth-matched pairs", "f");
  leg.AddEntry(&lc, TString::Format("cut p(+)/p(-) > %.1f", cut), "l");
  leg.Draw();
  c.SaveAs(TString(outbase) + ".png");
}
