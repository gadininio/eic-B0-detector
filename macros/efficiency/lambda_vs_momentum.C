// lambda_vs_momentum.C
//
// Acceptance and efficiency of the Lambda -> p pi- daughters vs the LAMBDA MOMENTUM, for
// truth- and hit-seeded B0 tracks (EICrecon with #3116: both are in the same file).
// Meant for the 50-100 GeV and 100-275 GeV samples together.
//
//   decayed before the B0  generated Lambdas that decayed to p pi- at z < 5.8 m, per generated
//                          Lambda; compared with BR(p pi-) x (1 - exp(-L / lambda)), L = 5.8 m,
//                          lambda = p / m x c tau (path ~ z: the Lambda flies at ~10 mrad)
//   acceptance             accepted daughters (>= 3 B0 layers of own simulated hits) per p pi- decay
//   tracking efficiency    reconstructed / accepted daughters, vs Lambda momentum and vs the
//                          daughter's own momentum
//   acceptance x eff.      both daughters reconstructed, per generated Lambda
// Definitions as in ../common/LambdaB0.h.
//
// Input: comma-separated list of EICrecon output files or patterns.
// Output: printout and <outbase>.png (2 x 3).
// Usage (from the repository top):
//   root -l -b -q 'macros/efficiency/lambda_vs_momentum.C+("/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconpr3116.edm4eic.root,/eos/user/g/gninio/eic/lambda_100to275/lambda_new_s20?.eicreconpr3116.edm4eic.root","results/08_momentum/vs_momentum_new")'
#include "../common/LambdaB0.h"
#include "TF1.h"
#include "TObjArray.h"
#include "TObjString.h"
#include <cstdio>

void lambda_vs_momentum(const char* files = "/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconpr3116.edm4eic.root,/eos/user/g/gninio/eic/lambda_100to275/lambda_new_s20?.eicreconpr3116.edm4eic.root",
                        const char* outbase = "vs_momentum_new", int minLayers = lb0::kMinLayers)
{
  using namespace lb0;
  const double kCTau = 0.0789;   // m
  const double kBR   = 0.641;    // Lambda -> p pi-
  TChain* ch = new TChain("events");
  TObjArray* list = TString(files).Tokenize(",");
  for (auto* o : *list) ch->Add(((TObjString*)o)->GetString());
  TTreeReader r(ch);
  MCEvent ev(r);
  Seeding seed[2] = {Seeding("truth"), Seeding("hit")};
  RecoEvent reco0(r, seed[0]), reco1(r, seed[1]);
  RecoEvent* reco[2] = {&reco0, &reco1};
  const char* kind[2] = {"truth", "hit"};

  const int nb = 9; const double pLo = 50, pHi = 275;
  const double dBins[] = {0, 5, 10, 15, 20, 30, 40, 60, 80, 120, 160, 200, 280};
  const int nd = sizeof(dBins) / sizeof(dBins[0]) - 1;
  TEfficiency eDec("eDec", "p#pi^{-} decay at z < 5.8 m, per generated #Lambda;p_{#Lambda} [GeV];fraction", nb, pLo, pHi);
  TEfficiency aP("aP", "accepted (#geq 3 B0 layers), per p#pi^{-} decay;p_{#Lambda} [GeV];fraction", nb, pLo, pHi);
  TEfficiency aPi("aPi", "", nb, pLo, pHi), aB("aB", "", nb, pLo, pHi);
  std::vector<TEfficiency> eP, ePi, dP, dPi, axe;
  for (auto* v : {&eP, &ePi, &dP, &dPi, &axe}) v->reserve(2);
  for (int s = 0; s < 2; ++s) {
    eP.emplace_back(TString("eP_") + kind[s], "tracking efficiency of accepted daughters;p_{#Lambda} [GeV];efficiency", nb, pLo, pHi);
    ePi.emplace_back(TString("ePi_") + kind[s], "", nb, pLo, pHi);
    dP.emplace_back(TString("dP_") + kind[s], "tracking efficiency of accepted daughters;daughter p [GeV];efficiency", nd, dBins);
    dPi.emplace_back(TString("dPi_") + kind[s], "", nd, dBins);
    axe.emplace_back(TString("axe_") + kind[s], "both daughters reconstructed, per generated #Lambda;p_{#Lambda} [GeV];fraction", nb, pLo, pHi);
  }
  TEfficiency zB("zB", "both daughters accepted, per p#pi^{-} decay;#Lambda decay z [m];fraction", 16, 0, 8);
  // per momentum bin: generated, p pi- decays, ... for the printout
  std::vector<int> nGen(nb), nPPi(nb), nBef(nb), nAccB(nb), nAccP(nb), nAccPi(nb);
  std::vector<std::vector<int>> nRecP(2, std::vector<int>(nb)), nRecPi(2, std::vector<int>(nb)), nRecB(2, std::vector<int>(nb));

  while (r.Next()) {
    std::vector<int> recoOf[2] = {reco0.RecoOfMC(ev.pdg.GetSize()), reco1.RecoOfMC(ev.pdg.GetSize())};
    for (auto& L : ev.Lambdas()) {
      double p = ev.P(L.idx).Mag();
      int b = std::min(nb - 1, std::max(0, int((p - pLo) / (pHi - pLo) * nb)));
      ++nGen[b];
      bool pp = L.fate == MCEvent::kPPi;
      double z = ev.ez[L.idx] / 1000;
      bool before = pp && ev.ez[L.idx] < kZFrontB0;
      eDec.Fill(before, p); nBef[b] += before;
      bool rcB[2] = {false, false};
      if (pp) {
        ++nPPi[b];
        bool ap = ev.SimLayers(L.p) >= minLayers, api = ev.SimLayers(L.pi) >= minLayers;
        aP.Fill(ap, p); aPi.Fill(api, p); aB.Fill(ap && api, p); zB.Fill(ap && api, z);
        nAccP[b] += ap; nAccPi[b] += api; nAccB[b] += ap && api;
        for (int s = 0; s < 2; ++s) {
          bool rp = recoOf[s][L.p] >= 0, rpi = recoOf[s][L.pi] >= 0;
          rcB[s] = rp && rpi;
          if (ap)  { eP[s].Fill(rp, p);   dP[s].Fill(rp, ev.P(L.p).Mag());   nRecP[s][b] += rp; }
          if (api) { ePi[s].Fill(rpi, p); dPi[s].Fill(rpi, ev.P(L.pi).Mag()); nRecPi[s][b] += rpi; }
          nRecB[s][b] += rcB[s];
        }
      }
      for (int s = 0; s < 2; ++s) axe[s].Fill(rcB[s], p);
    }
  }

  printf("%s\n  (%lld events)\n", files, ch->GetEntries());
  printf("%-9s %5s %5s %7s %7s | %6s %6s %6s | %-13s %-13s | %-13s\n", "p_L [GeV]", "gen", "ppi", "z<5.8m", "expect",
         "acc p", "acc pi", "both", "eff p t/h", "eff pi t/h", "AxE/L t/h");
  for (int b = 0; b < nb; ++b) {
    double lo = pLo + b * (pHi - pLo) / nb, hi = lo + (pHi - pLo) / nb, pm = 0.5 * (lo + hi);
    double expect = kBR * (1 - exp(-kZFrontB0 / 1000 / (pm / kMassLambda * kCTau)));
    printf("%4.0f-%-4.0f %5d %5d %6.1f%% %6.1f%% | %5.1f%% %5.1f%% %5.1f%% | %5.1f/%5.1f%% %5.1f/%5.1f%% | %5.1f/%5.1f%%\n",
           lo, hi, nGen[b], nPPi[b], Pct(nBef[b], nGen[b]), 100 * expect,
           Pct(nAccP[b], nPPi[b]), Pct(nAccPi[b], nPPi[b]), Pct(nAccB[b], nPPi[b]),
           Pct(nRecP[0][b], nAccP[b]), Pct(nRecP[1][b], nAccP[b]), Pct(nRecPi[0][b], nAccPi[b]), Pct(nRecPi[1][b], nAccPi[b]),
           Pct(nRecB[0][b], nGen[b]), Pct(nRecB[1][b], nGen[b]));
  }

  TF1 fDec("fDec", "[0]*(1-exp(-[1]/(x/[2]*[3])))", pLo, pHi);
  fDec.SetParameters(kBR, kZFrontB0 / 1000, kMassLambda, kCTau);
  fDec.SetLineColor(kGray + 2); fDec.SetLineStyle(2);
  Style(eDec, kBlack, 20);
  Style(aP, kBlue, 20); Style(aPi, kRed, 21); Style(aB, kBlack, 22); Style(zB, kBlack, 22);
  for (int s = 0; s < 2; ++s) {
    int m = s ? 24 : 20;   // truth filled, hit open
    Style(eP[s], kBlue, m); Style(ePi[s], kRed, m + 1); Style(dP[s], kBlue, m); Style(dPi[s], kRed, m + 1);
    Style(axe[s], s ? kRed + 1 : kBlue + 1, m);
  }
  TCanvas c("c", "", 1800, 1000); c.Divide(3, 2);
  c.cd(1); DrawEff({&eDec}); fDec.Draw("same");
  TLegend l1(0.45, 0.72, 0.88, 0.88); l1.SetBorderSize(0);
  l1.AddEntry(&eDec, "simulation", "p"); l1.AddEntry(&fDec, "BR (1 - e^{-L/#lambda}), L = 5.8 m", "l"); l1.Draw();
  c.cd(2); DrawEff({&aP, &aPi, &aB});
  TLegend l2(0.15, 0.15, 0.4, 0.35); l2.SetBorderSize(0);
  l2.AddEntry(&aP, "proton", "p"); l2.AddEntry(&aPi, "#pi^{-}", "p"); l2.AddEntry(&aB, "both", "p"); l2.Draw();
  c.cd(3); DrawEff({&zB});
  TLegend l3(0.15, 0.15, 0.6, 0.38); l3.SetBorderSize(0);
  l3.AddEntry(&eP[0], "proton, truth seeding", "p"); l3.AddEntry(&eP[1], "proton, hit seeding", "p");
  l3.AddEntry(&ePi[0], "#pi^{-}, truth seeding", "p"); l3.AddEntry(&ePi[1], "#pi^{-}, hit seeding", "p");
  c.cd(4); DrawEff({&eP[0], &eP[1], &ePi[0], &ePi[1]}); l3.Draw();
  c.cd(5); DrawEff({&dP[0], &dP[1], &dPi[0], &dPi[1]}); l3.Draw();
  c.cd(6); DrawEff({&axe[0], &axe[1]});
  TLegend l6(0.5, 0.75, 0.88, 0.88); l6.SetBorderSize(0);
  l6.AddEntry(&axe[0], "truth seeding", "p"); l6.AddEntry(&axe[1], "hit seeding", "p"); l6.Draw();
  c.SaveAs(TString(outbase) + ".png");
}
