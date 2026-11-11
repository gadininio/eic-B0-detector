// LambdaB0.h: shared definitions for the Lambda -> p pi- / B0 tracker macros.
//
// Included by the macros in ../acceptance, ../efficiency and ../mass with
//   #include "../common/LambdaB0.h"
// and compiled with ACLiC (root -l -b -q 'macro.C+(...)').
//
// Contents
//   constants        masses, B0 layer grouping, crossing angle
//   CountLayers()    number of B0 layers covered by a set of hit z positions
//   BeamFrame()      momentum rotated into the proton-beam frame (25 mrad crossing undone)
//   MCEvent          MCParticles + simulated B0 hits; the generated Lambdas and how they ended;
//                    layers crossed by each MC particle (simulated hits)
//   Seeding()        collection names for truth-seeded or hit-seeded tracking
//   RecoEvent        reconstructed charged particles + their truth links; which MC particles
//                    have a reconstructed particle
//   RecHitLayers     layers with reconstructed B0 hits made by an MC particle
//   FourPanelPlots   the standard plots: fraction vs Lambda theta, phi, (theta_x, theta_y), decay z
//
// Definitions used everywhere (see README.md):
//   Lambda -> p pi- decay  generated Lambda (generatorStatus 1) with exactly two daughters,
//                          a proton and a pi-. A Lambda that interacted in material also has
//                          the Lambda as parent of its products, but has more daughters.
//   accepted daughter      its own simulated B0TrackerHits cover >= 3 B0 layers
//   reconstructed daughter a reconstructed charged particle whose best truth match (highest
//                          association weight) is that daughter. Two kinds of tracking, chosen
//                          with the "seeding" argument of the macros:
//                            "truth" (default) ReconstructedTruthSeededChargedParticles: B0 tracks
//                                    seeded from the true vertex and momentum (EICrecon#2901)
//                            "hit"   ReconstructedChargedParticles: B0 tracks seeded from triplets
//                                    of hits (EICrecon#3116; empty in EICrecon builds without it)
#pragma once
#include "TTreeReader.h"
#include "TTreeReaderArray.h"
#include "TChain.h"
#include "TVector3.h"
#include "TMath.h"
#include "TString.h"
#include "TEfficiency.h"
#include "TGraphAsymmErrors.h"
#include "TH1.h"
#include "TH2D.h"
#include "TAxis.h"
#include "TLegend.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TROOT.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace lb0 {

// ---- constants -------------------------------------------------------------------------
const double kMassLambda = 1.115683;   // GeV
const double kMassProton = 0.938272;   // GeV
const double kMassPion   = 0.139570;   // GeV
const double kLayerGap   = 50;         // mm: hits further apart in z belong to different B0 layers
const int    kMinLayers  = 3;          // layers needed for a track
const double kCrossing   = 0.025;      // rad: crossing angle used in ddsim (--crossingAngleBoost -0.025)
const double kZFrontB0   = 5800;       // mm: just in front of the first B0 layer (both geometries)

// ---- helpers ---------------------------------------------------------------------------
// number of B0 layers covered by hits at these z positions [mm]
inline int CountLayers(std::vector<double> z) {
  std::sort(z.begin(), z.end());
  int n = 0;
  for (size_t k = 0; k < z.size(); ++k) if (k == 0 || z[k] - z[k - 1] > kLayerGap) ++n;
  return n;
}

// momentum in the proton-beam frame: undo the rotation of the crossing-angle boost
inline TVector3 BeamFrame(TVector3 p, double crossing = kCrossing) { p.RotateY(crossing); return p; }

// TChain from a file name or pattern (wildcards allowed)
inline TChain* MakeChain(const char* files) {
  TChain* ch = new TChain("events");
  ch->Add(files);
  return ch;
}

// ---- MC truth --------------------------------------------------------------------------
struct MCEvent {
  TTreeReaderArray<int>          pdg, status;
  TTreeReaderArray<double>       px, py, pz, vz, ez;
  TTreeReaderArray<unsigned int> pBeg, pEnd, dBeg, dEnd;
  TTreeReaderArray<int>          pIdx, dIdx;
  TTreeReaderArray<double>       hz;      // simulated B0 hits: z position
  TTreeReaderArray<int>          hPart;   // simulated B0 hits: MC particle index

  explicit MCEvent(TTreeReader& r)
    : pdg(r, "MCParticles.PDG"), status(r, "MCParticles.generatorStatus"),
      px(r, "MCParticles.momentum.x"), py(r, "MCParticles.momentum.y"), pz(r, "MCParticles.momentum.z"),
      vz(r, "MCParticles.vertex.z"), ez(r, "MCParticles.endpoint.z"),
      pBeg(r, "MCParticles.parents_begin"), pEnd(r, "MCParticles.parents_end"),
      dBeg(r, "MCParticles.daughters_begin"), dEnd(r, "MCParticles.daughters_end"),
      pIdx(r, "_MCParticles_parents.index"), dIdx(r, "_MCParticles_daughters.index"),
      hz(r, "B0TrackerHits.position.z"), hPart(r, "_B0TrackerHits_particle.index") {}

  enum Fate { kPPi, kNPi0, kOther };   // decayed to p pi-, to n pi0, or other (interaction, no decay)
  struct Lambda { int idx, p, pi; Fate fate; };

  // the generated Lambdas in this event and how they ended
  std::vector<Lambda> Lambdas() {
    std::vector<Lambda> out;
    for (size_t i = 0; i < pdg.GetSize(); ++i) {
      if (pdg[i] != 3122 || status[i] != 1) continue;
      int nd = dEnd[i] - dBeg[i], ip = -1, ipi = -1, in = -1, ipi0 = -1;
      for (unsigned int k = dBeg[i]; k < dEnd[i]; ++k) {
        int d = dIdx[k];
        if (pdg[d] == 2212) ip = d;
        if (pdg[d] == -211) ipi = d;
        if (pdg[d] == 2112) in = d;
        if (pdg[d] == 111) ipi0 = d;
      }
      Fate f = (nd == 2 && ip >= 0 && ipi >= 0) ? kPPi : (nd == 2 && in >= 0 && ipi0 >= 0) ? kNPi0 : kOther;
      out.push_back({(int)i, ip, ipi, f});
    }
    return out;
  }
  TVector3 P(int i) { return TVector3(px[i], py[i], pz[i]); }
  // B0 layers crossed by MC particle i (its own simulated hits)
  int SimLayers(int i) {
    std::vector<double> z;
    for (size_t h = 0; h < hz.GetSize(); ++h) if (hPart[h] == i) z.push_back(hz[h]);
    return CountLayers(z);
  }
  // the generated Lambda that decayed to p pi- and has MC particle m as its proton (wantPdg 2212)
  // or pi- (wantPdg -211); -1 if none
  int LambdaOf(int m, int wantPdg) {
    if (m < 0 || pdg[m] != wantPdg || pEnd[m] == pBeg[m]) return -1;
    int par = pIdx[pBeg[m]];
    return (pdg[par] == 3122 && status[par] == 1 && dEnd[par] - dBeg[par] == 2) ? par : -1;
  }
};

// ---- reconstruction (EICrecon output only) ------------------------------------------------
// collection names of the two kinds of tracking (see "reconstructed daughter" above)
struct Seeding {
  bool truth;
  explicit Seeding(const char* s) : truth(TString(s) == "truth") {
    if (!truth && TString(s) != "hit") { fprintf(stderr, "seeding must be \"truth\" or \"hit\", not \"%s\"\n", s); exit(1); }
  }
  const char* Name() const { return truth ? "truth seeding" : "hit seeding"; }
  TString Particles() const { return truth ? "ReconstructedTruthSeededChargedParticles" : "ReconstructedChargedParticles"; }
  TString ParticleAssociations() const { return truth ? "ReconstructedTruthSeededChargedParticleAssociations" : "ReconstructedChargedParticleAssociations"; }
  // B0 tracks before / after the ambiguity solver, linked to MC particles
  TString B0TrackAssociations(bool unfiltered) const {
    return TString("B0TrackerCKF") + (truth ? "TruthSeeded" : "") + (unfiltered ? "TrackUnfilteredAssociations" : "TrackAssociations");
  }
};

struct RecoEvent {
  TTreeReaderArray<float> px, py, pz, q;
  TTreeReaderArray<int>   aRec, aSim;
  TTreeReaderArray<float> aW;

  explicit RecoEvent(TTreeReader& r, const Seeding& s = Seeding("truth"))
    : px(r, s.Particles() + ".momentum.x"),
      py(r, s.Particles() + ".momentum.y"),
      pz(r, s.Particles() + ".momentum.z"),
      q(r, s.Particles() + ".charge"),
      aRec(r, "_" + s.ParticleAssociations() + "_rec.index"),
      aSim(r, "_" + s.ParticleAssociations() + "_sim.index"),
      aW(r, s.ParticleAssociations() + ".weight") {}

  int N() { return px.GetSize(); }
  TVector3 P(int k) { return TVector3(px[k], py[k], pz[k]); }
  // best truth match (highest weight) of each reconstructed particle; -1 if none
  std::vector<int> BestMC() {
    int n = N();
    std::vector<int> mc(n, -1); std::vector<float> w(n, -1);
    for (size_t a = 0; a < aRec.GetSize(); ++a) {
      int k = aRec[a];
      if (k >= 0 && k < n && aW[a] > w[k]) { w[k] = aW[a]; mc[k] = aSim[a]; }
    }
    return mc;
  }
  // for each MC particle (nMC of them): index of the reconstructed particle matched to it, or -1
  std::vector<int> RecoOfMC(size_t nMC) {
    std::vector<int> out(nMC, -1), mc = BestMC();
    for (int k = 0; k < (int)mc.size(); ++k) if (mc[k] >= 0 && mc[k] < (int)nMC) out[mc[k]] = k;
    return out;
  }
};

// layers with a reconstructed B0 hit made (at least partly) by an MC particle:
// B0TrackerRecHits -> raw hit -> B0TrackerRawHitAssociations -> simulated hit -> MC particle
struct RecHitLayers {
  TTreeReaderArray<float> z;
  TTreeReaderArray<int>   raw, aRaw, aSim;
  std::vector<std::vector<int>> rawToSim;

  explicit RecHitLayers(TTreeReader& r)
    : z(r, "B0TrackerRecHits.position.z"), raw(r, "_B0TrackerRecHits_rawHit.index"),
      aRaw(r, "_B0TrackerRawHitAssociations_rawHit.index"), aSim(r, "_B0TrackerRawHitAssociations_simHit.index") {}
  // call once per event, before Layers()
  void Update() {
    rawToSim.assign(raw.GetSize() ? *std::max_element(raw.begin(), raw.end()) + 1 : 0, {});
    for (size_t a = 0; a < aRaw.GetSize(); ++a)
      if (aRaw[a] >= 0 && aRaw[a] < (int)rawToSim.size()) rawToSim[aRaw[a]].push_back(aSim[a]);
  }
  // was reconstructed hit h made (at least partly) by MC particle mc?
  bool MadeBy(int h, int mc, MCEvent& ev) {
    int rw = raw[h];
    if (rw < 0 || rw >= (int)rawToSim.size()) return false;
    for (int s : rawToSim[rw]) if (s >= 0 && s < (int)ev.hPart.GetSize() && ev.hPart[s] == mc) return true;
    return false;
  }
  int Layers(int mc, MCEvent& ev) {
    std::vector<double> zz;
    for (size_t h = 0; h < z.GetSize(); ++h) if (MadeBy(h, mc, ev)) zz.push_back(z[h]);
    return CountLayers(zz);
  }
};

// ---- plots -----------------------------------------------------------------------------
inline void Style(TEfficiency& e, int col, int marker) { e.SetLineColor(col); e.SetMarkerColor(col); e.SetMarkerStyle(marker); }

// draw efficiencies on the current pad, y in [0, 1.1], x over the full histogram range
// (otherwise the x axis is sized to the first graph only)
inline void DrawEff(std::vector<TEfficiency*> effs) {
  effs[0]->Draw("AP"); gPad->Update();
  TGraphAsymmErrors* g = effs[0]->GetPaintedGraph();
  g->SetMinimum(0); g->SetMaximum(1.1);
  const TAxis* ax = effs[0]->GetTotalHistogram()->GetXaxis();
  g->GetXaxis()->SetLimits(ax->GetXmin(), ax->GetXmax());
  for (size_t k = 1; k < effs.size(); ++k) effs[k]->Draw("P same");
}

// 2D map pass/all on the current pad; cells with entries but zero fraction get a tiny value
// so that colz draws them in the lowest colour (empty cells stay white)
inline void DrawRatioMap(TH2D& pass, const TH2D& all) {
  pass.Divide(&all);
  for (int bx = 1; bx <= all.GetNbinsX(); ++bx) for (int by = 1; by <= all.GetNbinsY(); ++by)
    if (all.GetBinContent(bx, by) > 0 && pass.GetBinContent(bx, by) == 0) pass.SetBinContent(bx, by, 1e-3);
  pass.SetStats(0); pass.SetMinimum(0); pass.SetMaximum(1); pass.Draw("colz");
}

// Fraction of Lambda -> p pi- decays passing a daughter condition (proton / pi- / both),
// vs the LAMBDA direction (theta, phi w.r.t. the proton beam; 2D theta_x, theta_y) and
// decay z. Used for the acceptance and for acceptance x efficiency.
struct FourPanelPlots {
  TString what;
  TEfficiency thP, thPi, thB, phP, phPi, phB, z;
  TH2D all, both;
  FourPanelPlots(const char* tag, const TString& what_)
    : what(what_),
      thP(TString(tag) + "thP", what_ + " per p#pi^{-} decay;#Lambda #theta w.r.t. proton beam [mrad];fraction", 10, 8, 18),
      thPi(TString(tag) + "thPi", "", 10, 8, 18), thB(TString(tag) + "thB", "", 10, 8, 18),
      phP(TString(tag) + "phP", what_ + " per p#pi^{-} decay;#Lambda #phi w.r.t. proton beam [deg];fraction", 12, -180, 180),
      phPi(TString(tag) + "phPi", "", 12, -180, 180), phB(TString(tag) + "phB", "", 12, -180, 180),
      z(TString(tag) + "z", "both daughters " + what_ + ";#Lambda decay z [m];fraction", 14, 0, 7),
      all(TString(tag) + "all", "", 10, -20, 20, 10, -20, 20),
      both(TString(tag) + "both", "both daughters " + what_ + ";#Lambda #theta_{x} [mrad];#Lambda #theta_{y} [mrad]", 10, -20, 20, 10, -20, 20) {}
  // pLambda: Lambda momentum in the beam frame; zDecay in m
  void Fill(bool p, bool pi, const TVector3& pLambda, double zDecay) {
    bool b = p && pi;
    double th = pLambda.Theta() * 1000, ph = pLambda.Phi() * TMath::RadToDeg();
    double tx = pLambda.X() / pLambda.Z() * 1000, ty = pLambda.Y() / pLambda.Z() * 1000;
    thP.Fill(p, th); thPi.Fill(pi, th); thB.Fill(b, th);
    phP.Fill(p, ph); phPi.Fill(pi, ph); phB.Fill(b, ph);
    z.Fill(b, zDecay);
    all.Fill(tx, ty); if (b) both.Fill(tx, ty);
  }
  void Draw(const TString& png) {
    // proton blue, pi- red, both black
    Style(thP, kBlue, 20); Style(thPi, kRed, 21); Style(thB, kBlack, 22);
    Style(phP, kBlue, 20); Style(phPi, kRed, 21); Style(phB, kBlack, 22);
    TCanvas c("c4", "", 1300, 1000); c.Divide(2, 2);
    c.cd(1); DrawEff({&thP, &thPi, &thB});
    TLegend leg(0.15, 0.15, 0.4, 0.35);
    leg.AddEntry(&thP, "proton", "p"); leg.AddEntry(&thPi, "#pi^{-}", "p"); leg.AddEntry(&thB, "both", "p"); leg.Draw();
    c.cd(2); DrawEff({&phP, &phPi, &phB}); leg.Draw();
    c.cd(3); DrawRatioMap(both, all);
    c.cd(4); DrawEff({&z});
    c.SaveAs(png);
  }
};

inline double Pct(double a, double b) { return b > 0 ? 100. * a / b : 0.; }

}  // namespace lb0
