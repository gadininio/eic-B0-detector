// lambda_acceptance_x_efficiency.C
//
// ACCEPTANCE x EFFICIENCY for the Lambda -> p pi- daughters: the fraction of decays whose
// daughters are reconstructed.
//
//   acceptance x efficiency = (daughters with a reconstructed particle) / (Lambda -> p pi- decays)
//
// for the proton, the pi-, and both daughters of the same Lambda. Same denominator as the
// acceptance (../acceptance/lambda_daughter_acceptance.C), so the two sets of plots can be
// compared directly; it factorizes as acceptance x tracking efficiency
// (../efficiency/lambda_tracking_efficiency.C). The printout gives all three and checks the
// factorization (a daughter can in principle be reconstructed without being "accepted").
// Reconstructed = a ReconstructedTruthSeededChargedParticle whose best truth match is the daughter.
//
// Input: EICrecon output files of the Lambda gun (wildcards combine files).
// Output: printout and <outbase>.png (same four panels as the acceptance: vs Lambda theta,
// phi, 2D theta_x/theta_y, decay z).
// Usage (from the repository top):
//   root -l -b -q 'macros/efficiency/lambda_acceptance_x_efficiency.C+("/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconmain_5keV.edm4eic.root","results/05_acceptance_x_efficiency/acceptance_x_efficiency_new_5keV")'
#include "../common/LambdaB0.h"
#include <cstdio>

void lambda_acceptance_x_efficiency(const char* files = "/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconmain_5keV.edm4eic.root",
                                    const char* outbase = "acceptance_x_efficiency_new_5keV", int minLayers = lb0::kMinLayers)
{
  using namespace lb0;
  TChain* ch = MakeChain(files);
  TTreeReader r(ch);
  MCEvent ev(r);
  RecoEvent reco(r);

  FourPanelPlots plots("rec", "reconstructed");
  int nL = 0, nPPi = 0;
  // [0] proton, [1] pi-, [2] both: accepted, reconstructed, reconstructed and accepted
  int acc[3] = {0}, rec[3] = {0}, recAcc[3] = {0};

  while (r.Next()) {
    std::vector<int> recoOf = reco.RecoOfMC(ev.pdg.GetSize());
    for (auto& L : ev.Lambdas()) {
      ++nL;
      if (L.fate != MCEvent::kPPi) continue;
      ++nPPi;
      bool a[3], rc[3];
      a[0] = ev.SimLayers(L.p) >= minLayers;  a[1] = ev.SimLayers(L.pi) >= minLayers;  a[2] = a[0] && a[1];
      rc[0] = recoOf[L.p] >= 0;               rc[1] = recoOf[L.pi] >= 0;               rc[2] = rc[0] && rc[1];
      for (int t = 0; t < 3; ++t) { acc[t] += a[t]; rec[t] += rc[t]; recAcc[t] += a[t] && rc[t]; }
      plots.Fill(rc[0], rc[1], BeamFrame(ev.P(L.idx)), ev.ez[L.idx] / 1000);
    }
  }
  const char* name[3] = {"proton", "pi-", "both"};
  printf("%s  (%lld events)\n", files, ch->GetEntries());
  printf("generated Lambda: %d   -> p pi-: %d\n", nL, nPPi);
  printf("%-7s %12s %22s %16s %26s\n", "", "acceptance", "tracking efficiency", "acc x eff", "acc x eff (product check)");
  for (int t = 0; t < 3; ++t) {
    double A = Pct(acc[t], nPPi), E = Pct(recAcc[t], acc[t]);
    printf("%-7s %10.1f%% %20.1f%% %14.1f%% %24.1f%%\n", name[t], A, E, Pct(rec[t], nPPi), A * E / 100);
  }
  printf("reconstructed but not accepted: proton %d, pi- %d\n", rec[0] - recAcc[0], rec[1] - recAcc[1]);
  printf("ACCEPTANCE x EFFICIENCY, both daughters, per generated Lambda: %d of %d (%.1f%%)\n", rec[2], nL, Pct(rec[2], nL));
  plots.Draw(TString(outbase) + ".png");
}
