// lambda_tracking_efficiency.C
//
// TRACKING EFFICIENCY for the Lambda -> p pi- daughters, and where they are lost.
//
//   tracking efficiency = (accepted daughters with a reconstructed particle) / (accepted daughters)
//
// Accepted = the daughter's simulated B0 hits cover >= 3 layers (as in
// ../acceptance/lambda_daughter_acceptance.C), so the geometry cancels in the ratio.
// Reconstructed = a reconstructed charged particle whose best truth match is the daughter:
// seeding "truth" (default) ReconstructedTruthSeededChargedParticles, B0 tracks seeded from the
// true vertex and momentum (EICrecon#2901); "hit" ReconstructedChargedParticles, B0 tracks seeded
// from triplets of B0 hits (EICrecon#3116). See ../common/LambdaB0.h.
// Total = acceptance x tracking efficiency: ../efficiency/lambda_acceptance_x_efficiency.C.
//
// Step by step, for each accepted daughter:
//   rec hits       its reconstructed hits (B0TrackerRecHits -> raw hit -> B0TrackerRawHitAssociations
//                  -> simulated hit -> MC particle) still cover >= 3 layers after digitization
//                  (threshold, 10 keV by default)
//   seed           truth seeding: a B0TrackerTruthSeedParameters entry with the daughter's charge
//                  and direction (seeds have no MC link; they are made from the true momentum
//                  smeared by 10 %)
//                  hit seeding: a B0TrackerSeeds triplet with >= 2 of its 3 hits made by the
//                  daughter (seeds have no MC link; matched through their hits)
//   CKF track      B0TrackerCKF[TruthSeeded]TrackUnfilteredAssociations links a track to it
//   filtered track B0TrackerCKF[TruthSeeded]TrackAssociations links a track to it (after the
//                  ambiguity solver)
//   reco particle  best truth match of a reconstructed particle (= the numerator above)
//
// Input: EICrecon output files of the Lambda gun (wildcards combine files).
// Output: printout and <outbase>.png: tracking efficiency for proton and pi- vs the daughter's
// true momentum, its theta w.r.t. the proton beam, the Lambda decay z, and the number of layers
// with reconstructed hits.
// Usage (from the repository top):
//   root -l -b -q 'macros/efficiency/lambda_tracking_efficiency.C+("/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconmain_5keV.edm4eic.root","results/04_tracking_efficiency/tracking_efficiency_new_5keV")'
//   hit seeding: add the arguments  ,3,"hit"
#include "../common/LambdaB0.h"
#include "TVector2.h"
#include <memory>
#include <cstdio>

void lambda_tracking_efficiency(const char* files = "/eos/user/g/gninio/eic/lambda_1000ev/lambda_new_s10?.eicreconmain_5keV.edm4eic.root",
                                const char* outbase = "tracking_efficiency_new_5keV", int minLayers = lb0::kMinLayers,
                                const char* seeding = "truth")
{
  using namespace lb0;
  TChain* ch = MakeChain(files);
  TTreeReader r(ch);
  MCEvent ev(r);
  Seeding seed(seeding);
  RecoEvent reco(r, seed);
  RecHitLayers recHits(r);
  TTreeReaderArray<float> charge(r, "MCParticles.charge");
  // seeds of the chosen kind (only those readers are created: the other collection may be absent)
  std::unique_ptr<TTreeReaderArray<float>> sQoP, sTh, sPh;
  std::unique_ptr<TTreeReaderArray<int>> sHit;
  std::unique_ptr<TTreeReaderArray<unsigned int>> sBeg, sEnd;
  if (seed.truth) {
    sQoP.reset(new TTreeReaderArray<float>(r, "B0TrackerTruthSeedParameters.qOverP"));
    sTh.reset(new TTreeReaderArray<float>(r, "B0TrackerTruthSeedParameters.theta"));
    sPh.reset(new TTreeReaderArray<float>(r, "B0TrackerTruthSeedParameters.phi"));
  } else {
    sHit.reset(new TTreeReaderArray<int>(r, "_B0TrackerSeeds_hits.index"));
    sBeg.reset(new TTreeReaderArray<unsigned int>(r, "B0TrackerSeeds.hits_begin"));
    sEnd.reset(new TTreeReaderArray<unsigned int>(r, "B0TrackerSeeds.hits_end"));
  }
  // MC indices linked to a track by an association collection (any weight)
  struct Links { TTreeReaderArray<int> sim; Links(TTreeReader& rr, const TString& n) : sim(rr, "_" + n + "_sim.index") {} };
  Links lUnf(r, seed.B0TrackAssociations(true));
  Links lTrk(r, seed.B0TrackAssociations(false));
  auto linked = [](Links& l, int i) { for (size_t a = 0; a < l.sim.GetSize(); ++a) if (l.sim[a] == i) return true; return false; };
  auto seeded = [&](int i) {
    if (!seed.truth) {
      // hit seed: >= 2 of its hits made by the daughter
      for (size_t s = 0; s < sBeg->GetSize(); ++s) {
        int nMine = 0;
        for (unsigned int k = (*sBeg)[s]; k < (*sEnd)[s]; ++k) nMine += recHits.MadeBy((*sHit)[k], i, ev);
        if (nMine >= 2) return true;
      }
      return false;
    }
    auto& sQoP_ = *sQoP; auto& sTh_ = *sTh; auto& sPh_ = *sPh;
    TVector3 p = ev.P(i);
    double qop = charge[i] / p.Mag();
    // the seeder smears the momentum by 10 %: match charge sign and direction, q/p only loosely
    for (size_t s = 0; s < sQoP_.GetSize(); ++s)
      if (sQoP_[s] * qop > 0 && fabs(sQoP_[s] / qop - 1) < 0.5 && fabs(sTh_[s] - p.Theta()) < 1e-3
          && fabs(TVector2::Phi_mpi_pi(sPh_[s] - p.Phi())) < 0.05) return true;
    return false;
  };

  // [0] proton, [1] pi-
  const char* name[2] = {"proton", "pi-"};
  const char* tag[2] = {"p", "pi"};
  std::vector<TEfficiency> eP, eT, eZ, eL;
  for (auto* v : {&eP, &eT, &eZ, &eL}) v->reserve(2);
  for (int t = 0; t < 2; ++t) {
    eP.emplace_back(TString("eP") + tag[t], "tracking efficiency (accepted daughters);daughter true momentum [GeV];efficiency", 20, 0, 100);
    eT.emplace_back(TString("eT") + tag[t], "tracking efficiency (accepted daughters);daughter #theta w.r.t. proton beam [mrad];efficiency", 15, 0, 30);
    eZ.emplace_back(TString("eZ") + tag[t], "tracking efficiency (accepted daughters);#Lambda decay z [m];efficiency", 14, 0, 7);
    eL.emplace_back(TString("eL") + tag[t], "tracking efficiency (accepted daughters);layers with reconstructed hits;efficiency", 5, -0.5, 4.5);
  }
  const int nS = 6;
  const char* stage[nS] = {"accepted", "rec hits ok", "seed", "CKF track", "filtered track", "reco particle"};
  int n[2][nS] = {{0}}, nRecoGivenHits[2] = {0};
  const double pEdges[] = {0, 5, 10, 15, 20, 30, 100};
  const int nPB = 6;
  int piP[nPB][2] = {{0}};

  while (r.Next()) {
    recHits.Update();
    std::vector<int> recoOf = reco.RecoOfMC(ev.pdg.GetSize());
    for (auto& L : ev.Lambdas()) {
      if (L.fate != MCEvent::kPPi) continue;
      int d[2] = {L.p, L.pi};
      for (int t = 0; t < 2; ++t) {
        int m = d[t];
        if (ev.SimLayers(m) < minLayers) continue;
        int rl = recHits.Layers(m, ev);
        bool st[nS] = {true, rl >= minLayers, seeded(m), linked(lUnf, m), linked(lTrk, m), recoOf[m] >= 0};
        for (int s = 0; s < nS; ++s) n[t][s] += st[s];
        if (st[1]) nRecoGivenHits[t] += st[5];
        double pm = ev.P(m).Mag();
        eP[t].Fill(st[5], pm); eT[t].Fill(st[5], BeamFrame(ev.P(m)).Theta() * 1000);
        eZ[t].Fill(st[5], ev.ez[L.idx] / 1000); eL[t].Fill(st[5], rl);
        if (t == 1) for (int b = 0; b < nPB; ++b) if (pm >= pEdges[b] && pm < pEdges[b + 1]) { ++piP[b][0]; piP[b][1] += st[5]; }
      }
    }
  }
  printf("%s  (%lld events, %s)\n", files, ch->GetEntries(), seed.Name());
  printf("accepted daughters (>= %d layers of simulated hits) passing each step:\n", minLayers);
  printf("  %-16s %14s %14s\n", "step", name[0], name[1]);
  for (int s = 0; s < nS; ++s)
    printf("  %-16s %6d (%3.0f%%)  %6d (%3.0f%%)\n", stage[s], n[0][s], Pct(n[0][s], n[0][0]), n[1][s], Pct(n[1][s], n[1][0]));
  printf("TRACKING EFFICIENCY (reco particle / accepted): proton %.1f%%   pi- %.1f%%\n", Pct(n[0][5], n[0][0]), Pct(n[1][5], n[1][0]));
  printf("tracking efficiency given >= %d layers of reconstructed hits: proton %.1f%%   pi- %.1f%%\n", minLayers,
         Pct(nRecoGivenHits[0], n[0][1]), Pct(nRecoGivenHits[1], n[1][1]));
  printf("accepted pi-: tracking efficiency vs true momentum [GeV]\n");
  for (int b = 0; b < nPB; ++b)
    printf("  %3.0f-%3.0f: %4d / %4d (%3.0f%%)\n", pEdges[b], pEdges[b + 1], piP[b][1], piP[b][0], Pct(piP[b][1], piP[b][0]));

  for (int t = 0; t < 2; ++t)
    for (TEfficiency* e : {&eP[t], &eT[t], &eZ[t], &eL[t]}) Style(*e, t == 0 ? kBlue : kRed, t == 0 ? 20 : 21);
  TCanvas c("c", "", 1300, 1000); c.Divide(2, 2);
  TLegend leg(0.6, 0.15, 0.85, 0.3); leg.AddEntry(&eP[0], "proton", "p"); leg.AddEntry(&eP[1], "#pi^{-}", "p");
  c.cd(1); DrawEff({&eP[0], &eP[1]}); leg.Draw();
  c.cd(2); DrawEff({&eT[0], &eT[1]}); leg.Draw();
  c.cd(3); DrawEff({&eZ[0], &eZ[1]}); leg.Draw();
  c.cd(4); DrawEff({&eL[0], &eL[1]}); leg.Draw();
  c.SaveAs(TString(outbase) + ".png");
}
