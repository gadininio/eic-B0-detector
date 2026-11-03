// b0_geometric_acceptance.C
//
// GEOMETRIC B0 tracker acceptance along the Lambda directions, from a geantino gun.
//
// What it measures: the fraction of straight lines from the interaction point, at the
// angles of the Lambda gun (theta = 8-18 mrad w.r.t. the proton beam, 25 mrad crossing
// angle), that cross >= 1, >= 3 and 4 B0 layers. A geantino has no charge and no
// interactions, so this is pure detector coverage.
// What it does NOT measure: the acceptance for Lambdas. A Lambda is neutral and leaves no
// hits; its daughters start at the decay point, fly at other angles and are bent by the B0
// dipole. For that see lambda_daughter_acceptance.C.
//
// Accepted = the geantino's simulated B0 hits cover >= 3 layers (enough for a track).
// Angles are w.r.t. the proton beam axis (crossing-angle rotation undone).
//
// Input: ddsim output of scripts/sim/ddsim_gun.sh with PART=geantino. It needs
// --filter.tracker "" (geantinos deposit no energy; the default 1 keV filter would drop all hits).
// Output: printout and <outbase>.png (acceptance vs theta, vs phi, 2D map vs theta_x, theta_y).
// Usage (from the repository top):
//   root -l -b -q 'macros/acceptance/b0_geometric_acceptance.C+("/eos/user/g/gninio/eic/geantino/geantino_new_5000.edm4hep.root","results/02_geometric_acceptance/geometric_acceptance_new")'
#include "../common/LambdaB0.h"
#include "TH1D.h"
#include <cstdio>

void b0_geometric_acceptance(const char* files = "/eos/user/g/gninio/eic/geantino/geantino_new_5000.edm4hep.root",
                             const char* outbase = "geometric_acceptance_new")
{
  using namespace lb0;
  TChain* ch = MakeChain(files);
  TTreeReader r(ch);
  TTreeReaderArray<double> px(r, "MCParticles.momentum.x");
  TTreeReaderArray<double> py(r, "MCParticles.momentum.y");
  TTreeReaderArray<double> pz(r, "MCParticles.momentum.z");
  TTreeReaderArray<double> hz(r, "B0TrackerHits.position.z");

  TH1D hTh("hTh", "generated;#theta w.r.t. proton beam [mrad];events", 20, 8, 18);
  TEfficiency eTh("eTh", "geometric B0 acceptance (#geq3 layers);#theta w.r.t. proton beam [mrad];acceptance", 20, 8, 18);
  TEfficiency ePh("ePh", "geometric B0 acceptance (#geq3 layers);#phi w.r.t. proton beam [deg];acceptance", 36, -180, 180);
  TH2D hAll("hAll", "", 20, -20, 20, 20, -20, 20);
  TH2D hAcc("hAcc", "geometric B0 acceptance (#geq3 layers);#theta_{x} [mrad];#theta_{y} [mrad]", 20, -20, 20, 20, -20, 20);
  int n = 0, nL[6] = {0};

  while (r.Next()) {
    TVector3 p = BeamFrame(TVector3(px[0], py[0], pz[0]));   // the gun particle is MCParticles[0]
    double th = p.Theta() * 1000, ph = p.Phi() * TMath::RadToDeg();
    int layers = CountLayers(std::vector<double>(hz.begin(), hz.end()));
    bool acc = layers >= kMinLayers;
    ++n; ++nL[std::min(layers, 5)];
    hTh.Fill(th); eTh.Fill(acc, th); ePh.Fill(acc, ph);
    hAll.Fill(p.X() / p.Z() * 1000, p.Y() / p.Z() * 1000);
    if (acc) hAcc.Fill(p.X() / p.Z() * 1000, p.Y() / p.Z() * 1000);
  }
  int ge1 = n - nL[0], ge3 = nL[3] + nL[4] + nL[5], ge4 = nL[4] + nL[5];
  printf("%s: %d events\n", files, n);
  printf("layers crossed: 0:%d 1:%d 2:%d 3:%d 4:%d >4:%d\n", nL[0], nL[1], nL[2], nL[3], nL[4], nL[5]);
  auto pr = [&](const char* s, int k) { double a = double(k) / n; printf("  %-10s %5.1f %% +- %.1f\n", s, 100 * a, 100 * sqrt(a * (1 - a) / n)); };
  pr(">=1 layer", ge1); pr(">=3 layers", ge3); pr("4 layers", ge4);
  printf("theta distribution: events 13-18 / 8-13 mrad = %.2f (1.0 = flat in theta, ~1.5 = flat in cos theta)\n",
         hTh.Integral(11, 20) / hTh.Integral(1, 10));
  printf("acceptance (>=3 layers) vs theta [mrad]:");
  for (int b = 1; b <= 20; b += 2) printf("  %.0f:%.0f%%", eTh.GetTotalHistogram()->GetBinLowEdge(b), 100 * eTh.GetEfficiency(b));
  printf("\n");

  Style(eTh, kBlack, 20); Style(ePh, kBlack, 20);
  TCanvas c("c", "", 1500, 450); c.Divide(3, 1);
  c.cd(1); DrawEff({&eTh});
  c.cd(2); DrawEff({&ePh});
  c.cd(3); DrawRatioMap(hAcc, hAll);
  c.SaveAs(TString(outbase) + ".png");
}
