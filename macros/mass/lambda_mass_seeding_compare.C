// lambda_mass_seeding_compare.C
//
// Lambda mass with TRUTH-SEEDED vs HIT-SEEDED B0 tracks, overlaid, from the same events.
//
// Both kinds of tracks are in the same EICrecon output (EICrecon with #3116):
//   truth seeding  ReconstructedTruthSeededChargedParticles  (seeds from the true vertex and momentum)
//   hit seeding    ReconstructedChargedParticles             (seeds from triplets of B0 hits)
// For each kind, as in lambda_mass_reco.C and lambda_mass_notruth.C:
//   truth-matched  M(p pi-) of the reconstructed daughters of each Lambda -> p pi- decay
//   no truth       M of all (+,-) pairs (+ = proton, - = pi-), before and after p(+)/p(-) > minRatio;
//                  truth only labels signal afterwards (purity = signal / selected)
//   dp/p           (p_reco - p_true) / p_true of the matched Lambda daughters
// The printout also counts the decays reconstructed with both kinds of seeding, only one, and
// for those with both, the difference of the two masses.
//
// Input: EICrecon output files of the Lambda gun reconstructed with #3116 (wildcards combine files).
// Output: printout and <outbase>.png (2 x 2: truth-matched mass, all pairs, pairs after the cut, dp/p).
// Usage (from the repository top):
//   root -l -b -q 'macros/mass/lambda_mass_seeding_compare.C+("/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconpr3116.edm4eic.root","results/07_hit_seeding/mass_seeding_compare_new")'
#include "../common/LambdaB0.h"
#include "TLorentzVector.h"
#include "TH1D.h"
#include "TF1.h"
#include <cstdio>
#include <map>

void lambda_mass_seeding_compare(const char* files = "/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconpr3116.edm4eic.root",
                                 const char* outbase = "mass_seeding_compare_new", double minRatio = 2.5)
{
  using namespace lb0;
  TChain* ch = MakeChain(files);
  TTreeReader r(ch);
  MCEvent ev(r);
  const char* kind[2] = {"truth", "hit"};
  Seeding seed[2] = {Seeding("truth"), Seeding("hit")};
  RecoEvent reco0(r, seed[0]), reco1(r, seed[1]);
  RecoEvent* reco[2] = {&reco0, &reco1};

  std::vector<TH1D> hM, hAll, hCut, hR;
  for (auto* v : {&hM, &hAll, &hCut, &hR}) v->reserve(2);
  for (int s = 0; s < 2; ++s) {
    hM.emplace_back(TString("hM_") + kind[s], "M(p#pi^{-}), truth-matched daughters;M [GeV];pairs / 2 MeV", 55, 1.06, 1.17);
    hAll.emplace_back(TString("hAll_") + kind[s], "all (+,-) pairs, no truth in selection;M(p#pi^{-}) [GeV];pairs / 3 MeV", 100, 1.05, 1.35);
    hCut.emplace_back(TString("hCut_") + kind[s], TString::Format("p(+)/p(-) > %.1f, no truth in selection;M(p#pi^{-}) [GeV];pairs / 3 MeV", minRatio), 100, 1.05, 1.35);
    hR.emplace_back(TString("hR_") + kind[s], "matched #Lambda daughters: (p_{reco}-p_{true})/p_{true};#Deltap/p;tracks", 50, -0.25, 0.25);
  }
  int nPairs[2] = {0}, nAll[2] = {0}, nAllS[2] = {0}, nCut[2] = {0}, nCutS[2] = {0}, nWin[2] = {0}, nWinS[2] = {0};
  int nBoth = 0, nOnly[2] = {0};
  TH1D hDiff("hDiff", "", 200, -0.05, 0.05);
  auto p4 = [](RecoEvent* rc, int k, double m) { TLorentzVector v; v.SetVectM(rc->P(k), m); return v; };

  while (r.Next()) {
    std::map<int, double> mass[2];   // Lambda index -> truth-matched mass
    for (int s = 0; s < 2; ++s) {
      RecoEvent* rc = reco[s];
      std::vector<int> mc = rc->BestMC();
      std::vector<int> recoOf = rc->RecoOfMC(ev.pdg.GetSize());
      for (auto& L : ev.Lambdas()) {
        if (L.fate != MCEvent::kPPi || recoOf[L.p] < 0 || recoOf[L.pi] < 0) continue;
        double M = (p4(rc, recoOf[L.p], kMassProton) + p4(rc, recoOf[L.pi], kMassPion)).M();
        hM[s].Fill(M); ++nPairs[s]; mass[s][L.idx] = M;
        for (int d : {L.p, L.pi}) { double pt = ev.P(d).Mag(); hR[s].Fill((rc->P(recoOf[d]).Mag() - pt) / pt); }
      }
      for (int a = 0; a < rc->N(); ++a) for (int b = 0; b < rc->N(); ++b) {
        if (!(rc->q[a] > 0 && rc->q[b] < 0)) continue;
        TLorentzVector vp = p4(rc, a, kMassProton), vpi = p4(rc, b, kMassPion);
        double M = (vp + vpi).M();
        int lp = ev.LambdaOf(mc[a], 2212), lpi = ev.LambdaOf(mc[b], -211);
        bool sig = lp >= 0 && lp == lpi;
        ++nAll[s]; nAllS[s] += sig; hAll[s].Fill(M);
        if (vp.P() / vpi.P() > minRatio) {
          ++nCut[s]; nCutS[s] += sig; hCut[s].Fill(M);
          if (fabs(M - kMassLambda) < 0.02) { ++nWin[s]; nWinS[s] += sig; }
        }
      }
    }
    for (auto& [l, m] : mass[0]) {
      auto it = mass[1].find(l);
      if (it == mass[1].end()) ++nOnly[0]; else { ++nBoth; hDiff.Fill(it->second - m); }
    }
    for (auto& [l, m] : mass[1]) if (!mass[0].count(l)) ++nOnly[1];
  }

  printf("%s  (%lld events)\n", files, ch->GetEntries());
  std::vector<TF1> fit;
  for (int s = 0; s < 2; ++s) {
    fit.emplace_back(TString("g_") + kind[s], "gaus", kMassLambda - 0.015, kMassLambda + 0.015);
    hM[s].Fit(&fit[s], "QR0");
  }
  printf("%-34s %16s %16s\n", "", "truth seeding", "hit seeding");
  printf("%-34s %16d %16d\n", "truth-matched pairs", nPairs[0], nPairs[1]);
  printf("%-34s %16.4f %16.4f\n", "  Gaussian mean [GeV] (PDG 1.1157)", fit[0].GetParameter(1), fit[1].GetParameter(1));
  printf("%-34s %16.4f %16.4f\n", "  +- [GeV]", fit[0].GetParError(1), fit[1].GetParError(1));
  printf("%-34s %16.4f %16.4f\n", "  Gaussian sigma [GeV]", fit[0].GetParameter(2), fit[1].GetParameter(2));
  printf("%-34s %16.4f %16.4f\n", "  +- [GeV]", fit[0].GetParError(2), fit[1].GetParError(2));
  printf("%-34s %16.4f %16.4f\n", "  dp/p of the daughters: RMS", hR[0].GetRMS(), hR[1].GetRMS());
  printf("%-34s %16d %16d\n", "no truth: all (+,-) pairs", nAll[0], nAll[1]);
  printf("%-34s %15.1f%% %15.1f%%\n", "  purity", Pct(nAllS[0], nAll[0]), Pct(nAllS[1], nAll[1]));
  printf("%-34s %16d %16d\n", TString::Format("  after p(+)/p(-) > %.1f", minRatio).Data(), nCut[0], nCut[1]);
  printf("%-34s %15.1f%% %15.1f%%\n", "  purity", Pct(nCutS[0], nCut[0]), Pct(nCutS[1], nCut[1]));
  printf("%-34s %9d/%-6d %9d/%-6d\n", "  in +-20 MeV: signal/candidates", nWinS[0], nWin[0], nWinS[1], nWin[1]);
  printf("decays with truth-matched pairs: both seedings %d, only truth %d, only hit %d\n", nBoth, nOnly[0], nOnly[1]);
  printf("  both: M(hit) - M(truth) mean %.5f GeV, RMS %.5f GeV\n", hDiff.GetMean(), hDiff.GetRMS());

  const int col[2] = {kBlue + 1, kRed + 1};
  for (int s = 0; s < 2; ++s)
    for (TH1D* h : {&hM[s], &hAll[s], &hCut[s], &hR[s]}) { h->SetLineColor(col[s]); h->SetLineWidth(2); h->SetStats(0); }
  for (int s = 0; s < 2; ++s) { fit[s].SetLineColor(col[s]); fit[s].SetLineStyle(2); }
  auto draw = [&](std::vector<TH1D>& h, bool logy) {
    gPad->SetLogy(logy);
    double mx = std::max(h[0].GetMaximum(), h[1].GetMaximum());
    h[0].SetMaximum(logy ? mx * 3 : mx * 1.25); if (logy) h[0].SetMinimum(0.5);
    h[0].Draw("hist"); h[1].Draw("hist same");
  };
  TCanvas c("c", "", 1300, 1000); c.Divide(2, 2);
  c.cd(1); draw(hM, false); fit[0].Draw("same"); fit[1].Draw("same");
  TLegend l1(0.12, 0.65, 0.5, 0.88); l1.SetBorderSize(0); l1.SetTextSize(0.035);
  for (int s = 0; s < 2; ++s)
    l1.AddEntry(&hM[s], TString::Format("%s seeding: %d, #sigma = %.1f MeV", kind[s], nPairs[s], 1000 * fit[s].GetParameter(2)), "l");
  l1.Draw();
  c.cd(2); draw(hAll, true);
  TLegend l2(0.45, 0.72, 0.88, 0.88); l2.SetBorderSize(0); l2.SetTextSize(0.035);
  for (int s = 0; s < 2; ++s) l2.AddEntry(&hAll[s], TString::Format("%s seeding: %d, purity %.0f%%", kind[s], nAll[s], Pct(nAllS[s], nAll[s])), "l");
  l2.Draw();
  c.cd(3); draw(hCut, true);
  TLegend l3(0.45, 0.72, 0.88, 0.88); l3.SetBorderSize(0); l3.SetTextSize(0.035);
  for (int s = 0; s < 2; ++s) l3.AddEntry(&hCut[s], TString::Format("%s seeding: %d, purity %.0f%%", kind[s], nCut[s], Pct(nCutS[s], nCut[s])), "l");
  l3.Draw();
  c.cd(4); draw(hR, false);
  TLegend l4(0.12, 0.72, 0.5, 0.88); l4.SetBorderSize(0); l4.SetTextSize(0.035);
  for (int s = 0; s < 2; ++s) l4.AddEntry(&hR[s], TString::Format("%s seeding: RMS %.3f", kind[s], hR[s].GetRMS()), "l");
  l4.Draw();
  c.SaveAs(TString(outbase) + ".png");
}
