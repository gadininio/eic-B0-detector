// lambda_dis.C
//
// Lambda -> p pi- in the B0 for Lambda-tagged DIS (Pythia8 ep 18x275, Q2 > 1 GeV^2,
// scripts/gen/gen_dis_lambda.sh), with truth- and hit-seeded tracks (EICrecon#3116 build).
//
// B0-direction Lambda: generated Lambda (generatorStatus 1) at 5.5-20 mrad from the proton beam
// (beam frame, crossing angle undone). For these, as for the particle gun (../common/LambdaB0.h):
//   decayed before the B0  p pi- decay at z < 5.8 m, per B0-direction Lambda
//   acceptance             accepted daughters (>= 3 B0 layers of own simulated hits) per p pi- decay
//   tracking efficiency    reconstructed / accepted daughters
//   yield                  both daughters reconstructed, per B0-direction Lambda
// Mass: every (+,-) pair of FORWARD reconstructed particles (beam-frame theta < 30 mrad, i.e. B0
// tracks; the central tracker starts at ~40 mrad), + as proton, - as pi-, p(+)/p(-) > 2.5 as in
// ../mass/lambda_mass_notruth.C. Truth only labels signal (both from the same Lambda -> p pi-).
//
// Input: comma-separated list of EICrecon output files or patterns.
// Output: printout and <outbase>.png (2 x 3).
// Usage (from the repository top):
//   root -l -b -q 'macros/dis/lambda_dis.C+("/eos/user/g/gninio/eic/lambda_dis/dis_lambda_new_s30?.eicreconpr3116.edm4eic.root","results/09_dis/dis_new")'
#include "../common/LambdaB0.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TLorentzVector.h"
#include "TH1D.h"
#include "THStack.h"
#include "TF1.h"
#include "TStyle.h"
#include <cstdio>

void lambda_dis(const char* files = "/eos/user/g/gninio/eic/lambda_dis/dis_lambda_new_s30?.eicreconpr3116.edm4eic.root",
                const char* outbase = "dis_new", int minLayers = lb0::kMinLayers, double minRatio = 2.5)
{
  using namespace lb0;
  const double thLo = 5.5, thHi = 20, thFwd = 30;   // mrad
  TChain* ch = new TChain("events");
  TObjArray* list = TString(files).Tokenize(",");
  for (auto* o : *list) ch->Add(((TObjString*)o)->GetString());
  TTreeReader r(ch);
  MCEvent ev(r);
  Seeding seed[2] = {Seeding("truth"), Seeding("hit")};
  RecoEvent reco0(r, seed[0]), reco1(r, seed[1]);
  RecoEvent* reco[2] = {&reco0, &reco1};
  const char* kind[2] = {"truth", "hit"};

  const int nb = 8; const double pLo = 0, pHi = 200;
  TH1D hAll("hAll", "B0-direction #Lambda (5.5-20 mrad);p_{#Lambda} [GeV];#Lambda / 25 GeV", nb, pLo, pHi);
  TH1D hBef("hBef", "", nb, pLo, pHi), hAcc("hAcc", "", nb, pLo, pHi), hRec("hRec", "", nb, pLo, pHi);
  TEfficiency aP("aP", "accepted (#geq 3 B0 layers), per p#pi^{-} decay;p_{#Lambda} [GeV];fraction", nb, pLo, pHi);
  TEfficiency aPi("aPi", "", nb, pLo, pHi), aB("aB", "", nb, pLo, pHi);
  std::vector<TEfficiency> eP, ePi, yld;
  for (auto* v : {&eP, &ePi, &yld}) v->reserve(2);
  for (int s = 0; s < 2; ++s) {
    eP.emplace_back(TString("eP_") + kind[s], "tracking efficiency of accepted daughters;p_{#Lambda} [GeV];efficiency", nb, pLo, pHi);
    ePi.emplace_back(TString("ePi_") + kind[s], "", nb, pLo, pHi);
    yld.emplace_back(TString("yld_") + kind[s], "both daughters reconstructed, per B0-direction #Lambda;p_{#Lambda} [GeV];fraction", nb, pLo, pHi);
  }
  const int nm = 60; const double mLo = 1.08, mHi = 1.26;   // 3 MeV bins
  TH1D hS[2] = {TH1D("hS0", "", nm, mLo, mHi), TH1D("hS1", "", nm, mLo, mHi)};
  TH1D hB[2] = {TH1D("hB0", "", nm, mLo, mHi), TH1D("hB1", "", nm, mLo, mHi)};

  long nEv = 0, nEvB0 = 0, nLamAll = 0, nLamFwd = 0, nB0 = 0, nPPi = 0, nBef = 0, nAccB = 0;
  long nRecB[2] = {0, 0}, nEvRec[2] = {0, 0}, nFwdTrk[2] = {0, 0}, nPair[2] = {0, 0}, nPairS[2] = {0, 0};
  long nAccP = 0, nAccPi = 0, nRecP[2] = {0, 0}, nRecPi[2] = {0, 0};

  while (r.Next()) {
    ++nEv;
    std::vector<int> recoOf[2] = {reco0.RecoOfMC(ev.pdg.GetSize()), reco1.RecoOfMC(ev.pdg.GetSize())};
    bool evB0 = false, evRec[2] = {false, false};
    for (auto& L : ev.Lambdas()) {
      ++nLamAll;
      double th = BeamFrame(ev.P(L.idx)).Theta() * 1000;
      if (th < thFwd) ++nLamFwd;
      if (th < thLo || th > thHi) continue;
      evB0 = true; ++nB0;
      double p = ev.P(L.idx).Mag();
      hAll.Fill(p);
      bool pp = L.fate == MCEvent::kPPi;
      bool before = pp && ev.ez[L.idx] < kZFrontB0;
      nPPi += pp; nBef += before;
      if (before) hBef.Fill(p);
      bool rcB[2] = {false, false};
      if (pp) {
        bool ap = ev.SimLayers(L.p) >= minLayers, api = ev.SimLayers(L.pi) >= minLayers;
        aP.Fill(ap, p); aPi.Fill(api, p); aB.Fill(ap && api, p);
        nAccP += ap; nAccPi += api; nAccB += ap && api;
        if (ap && api) hAcc.Fill(p);
        for (int s = 0; s < 2; ++s) {
          bool rp = recoOf[s][L.p] >= 0, rpi = recoOf[s][L.pi] >= 0;
          rcB[s] = rp && rpi;
          if (ap)  { eP[s].Fill(rp, p);   nRecP[s] += rp; }
          if (api) { ePi[s].Fill(rpi, p); nRecPi[s] += rpi; }
        }
      }
      for (int s = 0; s < 2; ++s) { yld[s].Fill(rcB[s], p); nRecB[s] += rcB[s]; evRec[s] |= rcB[s]; }
      if (rcB[1]) hRec.Fill(p);
    }
    nEvB0 += evB0;
    for (int s = 0; s < 2; ++s) nEvRec[s] += evRec[s];

    // mass from forward (+,-) pairs, no truth in the selection
    for (int s = 0; s < 2; ++s) {
      RecoEvent& rc = *reco[s];
      std::vector<int> mc = rc.BestMC(), fwd;
      for (int k = 0; k < rc.N(); ++k) if (BeamFrame(rc.P(k)).Theta() * 1000 < thFwd) fwd.push_back(k);
      nFwdTrk[s] += fwd.size();
      for (int a : fwd) for (int b : fwd) {
        if (!(rc.q[a] > 0 && rc.q[b] < 0)) continue;
        TLorentzVector vp, vpi;
        vp.SetVectM(rc.P(a), kMassProton); vpi.SetVectM(rc.P(b), kMassPion);
        if (vp.P() / vpi.P() <= minRatio) continue;
        int lp = ev.LambdaOf(mc[a], 2212), lpi = ev.LambdaOf(mc[b], -211);
        bool sig = lp >= 0 && lp == lpi;
        ++nPair[s]; nPairS[s] += sig;
        (sig ? hS[s] : hB[s]).Fill((vp + vpi).M());
      }
    }
  }

  printf("%s\n", files);
  printf("events %ld; generated Lambdas %ld (%.3f / event), at < 30 mrad %ld, B0-direction (5.5-20 mrad) %ld\n",
         nEv, nLamAll, double(nLamAll) / nEv, nLamFwd, nB0);
  printf("events with a B0-direction Lambda: %ld (%.1f%%)\n", nEvB0, Pct(nEvB0, nEv));
  printf("B0-direction Lambdas: p pi- %ld (%.1f%%), before z = 5.8 m %ld (%.1f%%), both accepted %ld (%.1f%% of p pi-)\n",
         nPPi, Pct(nPPi, nB0), nBef, Pct(nBef, nB0), nAccB, Pct(nAccB, nPPi));
  for (int s = 0; s < 2; ++s)
    printf("%-14s eff p %5.1f%% (%ld/%ld)  pi %5.1f%% (%ld/%ld)  both reco %ld = %.1f%% per B0-dir. Lambda;"
           "  events with one: %ld (%.2f%% of events)\n", seed[s].Name(),
           Pct(nRecP[s], nAccP), nRecP[s], nAccP, Pct(nRecPi[s], nAccPi), nRecPi[s], nAccPi,
           nRecB[s], Pct(nRecB[s], nB0), nEvRec[s], Pct(nEvRec[s], nEv));
  for (int s = 0; s < 2; ++s) {
    int b1 = hS[s].FindBin(kMassLambda - 0.01), b2 = hS[s].FindBin(kMassLambda + 0.01);
    double S = hS[s].Integral(b1, b2), B = hB[s].Integral(b1, b2);
    printf("%-14s forward tracks / event %.2f; (+,-) pairs with p+/p- > %.1f: %ld, signal %ld; in +-10 MeV: S %.0f B %.0f\n",
           seed[s].Name(), double(nFwdTrk[s]) / nEv, minRatio, nPair[s], nPairS[s], S, B);
  }

  gStyle->SetOptStat(0);
  TCanvas c("c", "", 1800, 1000); c.Divide(3, 2);
  c.cd(1);
  hAll.SetLineColor(kBlack); hBef.SetLineColor(kBlue); hAcc.SetLineColor(kGreen + 2); hRec.SetLineColor(kRed);
  for (TH1D* h : {&hAll, &hBef, &hAcc, &hRec}) h->SetLineWidth(2);
  hAll.SetMinimum(0); hAll.Draw("hist"); hBef.Draw("hist same"); hAcc.Draw("hist same"); hRec.Draw("hist same");
  TLegend l1(0.42, 0.65, 0.88, 0.88); l1.SetBorderSize(0);
  l1.AddEntry(&hAll, "all", "l"); l1.AddEntry(&hBef, "p#pi^{-} decay at z < 5.8 m", "l");
  l1.AddEntry(&hAcc, "both daughters accepted", "l"); l1.AddEntry(&hRec, "both reconstructed (hit seeding)", "l"); l1.Draw();
  Style(aP, kBlue, 20); Style(aPi, kRed, 21); Style(aB, kBlack, 22);
  c.cd(2); DrawEff({&aP, &aPi, &aB});
  TLegend l2(0.6, 0.72, 0.88, 0.88); l2.SetBorderSize(0);
  l2.AddEntry(&aP, "proton", "p"); l2.AddEntry(&aPi, "#pi^{-}", "p"); l2.AddEntry(&aB, "both", "p"); l2.Draw();
  for (int s = 0; s < 2; ++s) {
    int m = s ? 24 : 20;   // truth filled, hit open
    Style(eP[s], kBlue, m); Style(ePi[s], kRed, m + 1); Style(yld[s], s ? kRed + 1 : kBlue + 1, m);
  }
  c.cd(3); DrawEff({&eP[0], &eP[1], &ePi[0], &ePi[1]});
  TLegend l3(0.45, 0.15, 0.88, 0.38); l3.SetBorderSize(0);
  l3.AddEntry(&eP[0], "proton, truth seeding", "p"); l3.AddEntry(&eP[1], "proton, hit seeding", "p");
  l3.AddEntry(&ePi[0], "#pi^{-}, truth seeding", "p"); l3.AddEntry(&ePi[1], "#pi^{-}, hit seeding", "p"); l3.Draw();
  c.cd(4); DrawEff({&yld[0], &yld[1]});
  TLegend l4(0.5, 0.75, 0.88, 0.88); l4.SetBorderSize(0);
  l4.AddEntry(&yld[0], "truth seeding", "p"); l4.AddEntry(&yld[1], "hit seeding", "p"); l4.Draw();
  THStack st[2] = {THStack("st0", ""), THStack("st1", "")};
  TLegend l5(0.55, 0.72, 0.88, 0.88); l5.SetBorderSize(0);
  l5.AddEntry(&hS[0], "signal (truth label)", "f"); l5.AddEntry(&hB[0], "background", "f");
  for (int s = 0; s < 2; ++s) {
    hS[s].SetFillColor(kGreen - 6); hS[s].SetLineColor(kGreen + 2);
    hB[s].SetFillColor(kGray); hB[s].SetLineColor(kGray + 2);
    st[s].SetTitle(TString::Format("forward (+,-) pairs, p(+)/p(-) > %.1f, %s;M(p#pi^{-}) [GeV];pairs / 3 MeV", minRatio, seed[s].Name()));
    st[s].Add(&hB[s]); st[s].Add(&hS[s]);
    c.cd(5 + s); st[s].Draw("hist"); l5.Draw();
  }
  c.SaveAs(TString(outbase) + ".png");
}
