// kaon_kinematics.C
//
// Kaon structure function: DIS kinematics (x, Q2, y) and x_K = x / (1 - x_L) for Lambda-tagged DIS
// events (Pythia8, scripts/gen/), EICrecon#3116 output.
//
// Frame. Our samples are Pythia head-on events boosted by ddsim --crossingAngleBoost -0.025, a pure
// Lorentz boost along x that tilts BOTH beams (beam e- px/|pz| = -0.025; in the ePIC campaign files,
// made with the afterburner, the electron beam is along -z). EICrecon's InclusiveKinematics* and
// HadronicFinalState put the beams into the ePIC frame and are therefore wrong for our samples.
// Here every event is boosted back by beta_x = -(px / E) of the summed true beams (MCParticles,
// generatorStatus 4), which restores the head-on frame exactly (beam |px| < 1e-10 GeV afterwards).
//
// Methods (head-on frame, proton along +z, electron along -z, theta = scattered-electron angle):
//   electron  Q2 = 2 Ee E' (1 + cos theta), y = 1 - E' (1 - cos theta) / (2 Ee)
//   JB        y = Sigma_h / (2 Ee), Q2 = pT_h^2 / (1 - y)
//   DA        hadron angle gamma from (pT_h, Sigma_h); y, Q2 from gamma and theta only
//   Sigma     y = Sigma_h / (Sigma_h + E' (1 - cos theta)), Q2 = E'^2 sin^2 theta / (1 - y)
//   eSigma    Q2 from the electron, x from Sigma
//   hybrid    DA if its y < yHybrid (default 0.05), else electron
//   x = Q2 / (s y) in all cases. Sigma_h = sum (E - pz), pT_h = |sum pT| of the hadronic final state.
// Inputs: scattered electron = the reconstructed charged particle EICrecon uses for
// InclusiveKinematicsElectron (ScatteredElectronsTruth: truth-matched track; electron mass); hadronic
// final state = all ReconstructedParticles (tracks + ECal clusters, no HCal) except that electron.
// True kinematics: beams and MCScatteredElectrons (identical to InclusiveKinematicsTruth, checked).
// Closure: the same formulas on the generated final state reproduce the true x (up to -6% at
// y < 0.01: the proton-mass term in sum(E - pz) is neglected).
//
// Lambda: generated = B0-direction Lambda (5.5-20 mrad from the proton beam) with x_L > xLMin;
// reconstructed (no truth) = forward (+,-) pair of hit-seeded tracks (beam-frame theta < 30 mrad),
// + as proton, - as pi-, p(+)/p(-) > minRatio, x_L(pair) > xLMin, |M - M_Lambda| < mWin; per event the
// pair closest to the Lambda mass. Signal = both tracks from the same Lambda -> p pi- (truth label).
// x_L = p_z(beam frame) / E_p, E_p from the file name (9x130, 10x100, 5x41, else 275 GeV).
//
// Output: printout, <outbase>.png (4 x 2) and <outbase>_xK.png (3 x 2: x_K distributions, reco vs true,
// resolution per method, x_K vs Q2).
// Usage (from the repository top):
//   root -l -b -q 'macros/kaon/kaon_kinematics.C+("/eos/user/g/gninio/eic/lambda_dis_9x130/dis_lambda_9x130_old_s40?.eicreconpr3116.edm4eic.root","results/11_kaon_sf/kin_9x130_old")'
#include "../common/LambdaB0.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TLorentzVector.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TStyle.h"
#include "TLatex.h"
#include "TPaveText.h"
#include "TGraph.h"
#include "TF1.h"
#include "TMarker.h"
#include <cstdio>

namespace {
const double kMassElectron = 0.000511;   // GeV
const unsigned int kColRecoCharged = 0x538857ff;   // podio collection ID of ReconstructedChargedParticles
enum { kE, kJB, kDA, kSig, kESig, kHyb, kNM };
const char* kMName[kNM] = {"electron", "JB", "DA", "Sigma", "eSigma", "hybrid"};

struct Kin { double Q2 = -1, x = -1, y = -1; bool ok = false; };
// all methods; e = scattered electron, S = Sigma_h, pt = pT_h (head-on frame); Ee, s of the beams
void Methods(const TLorentzVector& e, double S, double pt, double Ee, double s, double yHybrid, Kin k[kNM]) {
  for (int m = 0; m < kNM; ++m) k[m] = Kin();
  double Ep = e.E(), c = std::cos(e.Theta()), sn = std::sin(e.Theta());
  auto fin = [&](Kin& q) { q.ok = q.ok && q.y > 0 && q.y < 1 && q.Q2 > 0; if (q.ok) q.x = q.Q2 / (s * q.y); };
  k[kE].Q2 = 2 * Ee * Ep * (1 + c); k[kE].y = 1 - Ep * (1 - c) / (2 * Ee); k[kE].ok = true; fin(k[kE]);
  if (S > 0 && pt > 0) {
    k[kJB].y = S / (2 * Ee); k[kJB].Q2 = pt * pt / (1 - k[kJB].y); k[kJB].ok = k[kJB].y < 1; fin(k[kJB]);
    double tg = std::tan(0.5 * std::acos((pt * pt - S * S) / (pt * pt + S * S))), te = std::tan(0.5 * e.Theta());
    k[kDA].y = tg / (tg + te); k[kDA].Q2 = 4 * Ee * Ee / te / (tg + te); k[kDA].ok = true; fin(k[kDA]);
    k[kSig].y = S / (S + Ep * (1 - c)); k[kSig].Q2 = Ep * Ep * sn * sn / (1 - k[kSig].y); k[kSig].ok = true; fin(k[kSig]);
    if (k[kSig].ok && k[kE].Q2 > 0) { k[kESig].Q2 = k[kE].Q2; k[kESig].x = k[kSig].x; k[kESig].y = k[kE].Q2 / (s * k[kSig].x); k[kESig].ok = true; }
  }
  k[kHyb] = (k[kDA].ok && k[kDA].y < yHybrid) ? k[kDA] : k[kE];
}
// "median [16%, 84%]" of a sample
TString Spread(std::vector<double> v) {
  if (v.size() < 3) return TString::Format("n=%zu", v.size());
  std::sort(v.begin(), v.end());
  auto q = [&](double f) { return v[(size_t)(f * (v.size() - 1))]; };
  return TString::Format("%+.2f [%+.2f, %+.2f]", q(0.5), q(0.16), q(0.84));
}
std::vector<double> LogBins(int n, double lo, double hi) {
  std::vector<double> b(n + 1);
  for (int i = 0; i <= n; ++i) b[i] = lo * std::pow(hi / lo, double(i) / n);
  return b;
}
}  // namespace

void kaon_kinematics(const char* files = "/eos/user/g/gninio/eic/lambda_dis_9x130/dis_lambda_9x130_old_s40?.eicreconpr3116.edm4eic.root",
                     const char* outbase = "kin_9x130_old", double xLMin = 0.6, double minRatio = 2.5, double mWin = 0.010,
                     double yHybrid = 0.05, double yMin = 0.01)
{
  using namespace lb0;
  const double thLo = 5.5, thHi = 20, thFwd = 30;   // mrad
  TString fsn(files);
  const double Ep = fsn.Contains("9x130") ? 130 : fsn.Contains("10x100") ? 100 : fsn.Contains("5x41") ? 41 : 275;   // GeV
  auto XL = [&](const TVector3& p) { return BeamFrame(p).Z() / Ep; };

  TChain* ch = new TChain("events");
  TObjArray* list = fsn.Tokenize(",");
  for (auto* o : *list) ch->Add(((TObjString*)o)->GetString());
  TTreeReader r(ch);
  MCEvent ev(r);
  RecoEvent reco(r, Seeding("hit"));   // ReconstructedChargedParticles: also holds the scattered electron
  TTreeReaderArray<double> mcMass(r, "MCParticles.mass");
  TTreeReaderArray<int> mcScat(r, "MCScatteredElectrons_objIdx.index");
  TTreeReaderArray<int> scatIdx(r, "_InclusiveKinematicsElectron_scat.index");
  TTreeReaderArray<unsigned int> scatCol(r, "_InclusiveKinematicsElectron_scat.collectionID");
  TTreeReaderArray<float> rpx(r, "ReconstructedParticles.momentum.x"), rpy(r, "ReconstructedParticles.momentum.y"),
                          rpz(r, "ReconstructedParticles.momentum.z"), rpE(r, "ReconstructedParticles.energy");
  TTreeReaderArray<float> tQ2(r, "InclusiveKinematicsTruth.Q2"), eQ2(r, "InclusiveKinematicsElectron.Q2");

  const int ny = 5; const double yb[ny + 1] = {0, 0.01, 0.05, 0.2, 0.5, 1};
  auto YBin = [&](double y) { for (int k = 0; k < ny; ++k) if (y >= yb[k] && y < yb[k + 1]) return k; return -1; };
  std::vector<double> rX[kNM][ny], rQ[kNM][ny], rXL[kNM][ny], gX[kNM][ny], eicQ[ny];   // all events / high-x_L events / closure
  long nEv = 0, nScat = 0, nScatOther = 0, nTruthMismatch = 0, nEvY[ny] = {0}, nFail[kNM][ny] = {{0}};
  double maxBeamPx = 0;
  // Lambda side
  long nSel = 0, nSelY[ny] = {0}, nCand = 0, nCandSig = 0, nCandY[ny] = {0}, nCandSigY[ny] = {0}, nCandCut = 0, nCandCutSig = 0;
  std::vector<double> dXK[kNM], dXKcut[kNM], dXL;

  auto bx = LogBins(30, 1e-4, 1), bq = LogBins(20, 1, 100), byy = LogBins(32, 1e-4, 1);
  TH2D hQx("hQx", "events with a B0-direction #Lambda, x_{L} > 0.6 (true);x (true);Q^{2} (true) [GeV^{2}]", 30, bx.data(), 20, bq.data());
  TH2D hQxR("hQxR", "", 30, bx.data(), 20, bq.data());
  TH1D hY("hY", "events with a B0-direction #Lambda, x_{L} > 0.6;y (true);events", 32, byy.data());
  TH1D hYR("hYR", "", 32, byy.data());
  TH1D hdQ("hdQ", "Q^{2}, electron method, all events;Q^{2}_{reco} / Q^{2}_{true} - 1;events", 50, -0.5, 0.5), hdQe("hdQe", "", 50, -0.5, 0.5);
  TH2D hdXyE("hdXyE", "x, electron method, all events;log_{10} y (true);x_{reco} / x_{true} - 1", 20, -4, 0, 50, -1, 1);
  TH2D hdXyD("hdXyD", "x, double-angle method, all events;log_{10} y (true);x_{reco} / x_{true} - 1", 20, -4, 0, 50, -1, 1);
  TH1D hXK("hXK", "x_{K} = x / (1 - x_{L});x_{K};events", 20, 0., 1.);
  TH1D hXKR("hXKR", "", 20, 0., 1.), hXKRS("hXKRS", "", 20, 0., 1.);
  // x_K figure
  TH1D hXKg("hXKg", "x_{K}, events with a B0-direction #Lambda (x_{L} > 0.6);x_{K} (true);events", 20, 0., 1.);
  TH1D hXKgY("hXKgY", "", 20, 0., 1.), hXKs("hXKs", "", 20, 0., 1.), hXKsC("hXKsC", "", 20, 0., 1.);
  TH1D hXKrH("hXKrH", "x_{K}, signal candidates with y_{DA} > cut;x_{K};candidates", 20, 0., 1.);
  TH1D hXKrE("hXKrE", "", 20, 0., 1.), hXKrD("hXKrD", "", 20, 0., 1.), hXKtC("hXKtC", "", 20, 0., 1.);
  TH1D hdXKm[3] = {TH1D("hdXKm0", "x_{K} resolution, signal candidates, y_{DA} > cut;x_{K}^{reco} / x_{K}^{true} - 1;candidates", 20, -1, 1),
                   TH1D("hdXKm1", "", 20, -1, 1), TH1D("hdXKm2", "", 20, -1, 1)};   // electron, DA, hybrid
  TH2D hXKQ("hXKQ", "x_{K} vs Q^{2} (true), B0-direction #Lambda, x_{L} > 0.6;x_{K} (true);Q^{2} (true) [GeV^{2}]", 20, 0., 1., 20, bq.data());
  std::vector<double> gT, gR, gTf, gRf, cXK, cQ2;   // reco vs true (pass / fail the y cut); candidate (x_K, Q2) true
  TH1D hdXKh("hdXKh", TString::Format("x_{K}, #Lambda candidates (signal);x_{K}^{reco} / x_{K}^{true} - 1;candidates"), 20, -1, 1), hdXKe("hdXKe", "", 20, -1, 1);

  while (r.Next()) {
    ++nEv;
    TLorentzVector eb, pb;
    for (size_t i = 0; i < ev.pdg.GetSize(); ++i) {
      if (ev.status[i] != 4) continue;
      if (ev.pdg[i] == 11) eb.SetXYZM(ev.px[i], ev.py[i], ev.pz[i], kMassElectron);
      if (ev.pdg[i] == 2212) pb.SetXYZM(ev.px[i], ev.py[i], ev.pz[i], kMassProton);
    }
    TLorentzVector tot = eb + pb;
    const TVector3 toHeadOn(-tot.Px() / tot.E(), 0, 0);
    TLorentzVector ebh = eb, pbh = pb; ebh.Boost(toHeadOn); pbh.Boost(toHeadOn);
    maxBeamPx = std::max({maxBeamPx, std::fabs(ebh.Px()), std::fabs(pbh.Px())});
    const double Ee = ebh.E(), s = tot.M2();
    if (!mcScat.GetSize() || mcScat[0] < 0) continue;
    int im = mcScat[0];
    TLorentzVector et; et.SetXYZM(ev.px[im], ev.py[im], ev.pz[im], kMassElectron);
    TLorentzVector q = eb - et;
    Kin kt; kt.Q2 = -q.M2(); kt.y = pb.Dot(q) / pb.Dot(eb); kt.x = kt.Q2 / (2 * pb.Dot(q)); kt.ok = kt.Q2 > 0 && kt.y > 0;
    if (!kt.ok) continue;
    if (tQ2.GetSize() && std::fabs(tQ2[0] / kt.Q2 - 1) > 1e-3) ++nTruthMismatch;
    int iy = YBin(kt.y);
    if (iy < 0) continue;
    ++nEvY[iy];
    TLorentzVector eth = et; eth.Boost(toHeadOn);

    // closure: the methods on the generated final state
    {
      double S = 0, sx = 0, sy = 0;
      for (size_t i = 0; i < ev.pdg.GetSize(); ++i) {
        if (ev.status[i] != 1 || (int)i == im) continue;
        TLorentzVector v; v.SetXYZM(ev.px[i], ev.py[i], ev.pz[i], mcMass[i]); v.Boost(toHeadOn);
        S += v.E() - v.Pz(); sx += v.Px(); sy += v.Py();
      }
      Kin kg[kNM]; Methods(eth, S, std::hypot(sx, sy), Ee, s, yHybrid, kg);
      for (int m = 0; m < kNM; ++m) if (kg[m].ok) gX[m][iy].push_back(kg[m].x / kt.x - 1);
    }

    // generated B0-direction Lambda with x_L > xLMin
    double xLt = -1;
    for (auto& L : ev.Lambdas()) {
      TVector3 pl = ev.P(L.idx);
      double th = BeamFrame(pl).Theta() * 1000;
      if (th < thLo || th > thHi || XL(pl) < xLMin) continue;
      xLt = std::max(xLt, XL(pl));
    }
    if (xLt > 0) {
      ++nSel; ++nSelY[iy]; hQx.Fill(kt.x, kt.Q2); hY.Fill(kt.y); hXK.Fill(kt.x / (1 - xLt));
      double xKg = kt.x / (1 - xLt);
      hXKg.Fill(xKg); if (kt.y > yMin) hXKgY.Fill(xKg); hXKQ.Fill(xKg, kt.Q2);
    }

    // reconstructed kinematics
    Kin kr[kNM];
    bool haveE = scatIdx.GetSize() && scatIdx[0] >= 0 && scatIdx[0] < reco.N();
    if (haveE && scatCol[0] != kColRecoCharged) { ++nScatOther; haveE = false; }
    if (haveE) {
      ++nScat;
      TVector3 e3 = reco.P(scatIdx[0]);
      double S = 0, sx = 0, sy = 0;
      for (size_t k = 0; k < rpx.GetSize(); ++k) {
        TVector3 p3(rpx[k], rpy[k], rpz[k]);
        if ((p3 - e3).Mag() < 1e-4 * e3.Mag()) continue;   // the scattered electron (same track)
        TLorentzVector v(p3, rpE[k]); v.Boost(toHeadOn);
        S += v.E() - v.Pz(); sx += v.Px(); sy += v.Py();
      }
      TLorentzVector er; er.SetVectM(e3, kMassElectron); er.Boost(toHeadOn);
      Methods(er, S, std::hypot(sx, sy), Ee, s, yHybrid, kr);
      for (int m = 0; m < kNM; ++m) {
        if (!kr[m].ok) { ++nFail[m][iy]; continue; }
        rX[m][iy].push_back(kr[m].x / kt.x - 1); rQ[m][iy].push_back(kr[m].Q2 / kt.Q2 - 1);
        if (xLt > 0) rXL[m][iy].push_back(kr[m].x / kt.x - 1);
      }
      if (kr[kE].ok) { hdQ.Fill(kr[kE].Q2 / kt.Q2 - 1); hdXyE.Fill(std::log10(kt.y), kr[kE].x / kt.x - 1); }
      if (kr[kDA].ok) hdXyD.Fill(std::log10(kt.y), kr[kDA].x / kt.x - 1);
      if (eQ2.GetSize()) { eicQ[iy].push_back(eQ2[0] / kt.Q2 - 1); hdQe.Fill(eQ2[0] / kt.Q2 - 1); }
    }

    // reconstructed Lambda candidate (no truth): best forward (+,-) pair in the mass window
    std::vector<int> mc = reco.BestMC(), fwd;
    for (int k = 0; k < reco.N(); ++k) if (BeamFrame(reco.P(k)).Theta() * 1000 < thFwd) fwd.push_back(k);
    int ba = -1, bb = -1; double bdm = mWin;
    for (int a : fwd) for (int b : fwd) {
      if (!(reco.q[a] > 0 && reco.q[b] < 0)) continue;
      TLorentzVector vp, vpi;
      vp.SetVectM(reco.P(a), kMassProton); vpi.SetVectM(reco.P(b), kMassPion);
      if (vp.P() / vpi.P() <= minRatio || XL(vp.Vect() + vpi.Vect()) < xLMin) continue;
      double dm = std::fabs((vp + vpi).M() - kMassLambda);
      if (dm < bdm) { bdm = dm; ba = a; bb = b; }
    }
    if (ba >= 0) {
      int lp = ev.LambdaOf(mc[ba], 2212), lpi = ev.LambdaOf(mc[bb], -211);
      bool sig = lp >= 0 && lp == lpi;
      ++nCand; nCandSig += sig; ++nCandY[iy]; nCandSigY[iy] += sig;
      hQxR.Fill(kt.x, kt.Q2); hYR.Fill(kt.y);
      double xLr = XL(reco.P(ba) + reco.P(bb));
      bool pass = kr[kDA].ok && kr[kDA].y > yMin;   // y cut on the reconstructed (DA) y
      if (pass) { ++nCandCut; nCandCutSig += sig; }
      if (kr[kHyb].ok) { hXKR.Fill(kr[kHyb].x / (1 - xLr)); if (sig) hXKRS.Fill(kr[kHyb].x / (1 - xLr)); }
      if (sig) {
        double xKt = kt.x / (1 - XL(ev.P(lp)));
        dXL.push_back(xLr - XL(ev.P(lp)));
        for (int m = 0; m < kNM; ++m) if (kr[m].ok) {
          double d = kr[m].x / (1 - xLr) / xKt - 1;
          dXK[m].push_back(d); if (pass) dXKcut[m].push_back(d);
        }
        if (pass && kr[kHyb].ok) hdXKh.Fill(kr[kHyb].x / (1 - xLr) / xKt - 1);
        hXKs.Fill(xKt); cXK.push_back(xKt); cQ2.push_back(kt.Q2);
        if (kr[kHyb].ok) { (pass ? gT : gTf).push_back(xKt); (pass ? gR : gRf).push_back(kr[kHyb].x / (1 - xLr)); }
        if (pass) {
          hXKsC.Fill(xKt); hXKtC.Fill(xKt);
          if (kr[kHyb].ok) { hXKrH.Fill(kr[kHyb].x / (1 - xLr)); hdXKm[2].Fill(kr[kHyb].x / (1 - xLr) / xKt - 1); }
          if (kr[kE].ok)   { hXKrE.Fill(kr[kE].x / (1 - xLr));   hdXKm[0].Fill(kr[kE].x / (1 - xLr) / xKt - 1); }
          if (kr[kDA].ok)  { hXKrD.Fill(kr[kDA].x / (1 - xLr));  hdXKm[1].Fill(kr[kDA].x / (1 - xLr) / xKt - 1); }
        }
        if (pass && kr[kE].ok) hdXKe.Fill(kr[kE].x / (1 - xLr) / xKt - 1);
      }
    }
  }

  printf("%s\n", files);
  printf("events %ld (E_p = %.0f GeV); head-on frame: beam |px| after the inverse boost < %.1e GeV\n", nEv, Ep, maxBeamPx);
  printf("reconstructed scattered electron (ScatteredElectronsTruth track) in %ld (%.1f%%)%s; own truth vs InclusiveKinematicsTruth: %ld events differ\n",
         nScat, Pct(nScat, nEv), nScatOther ? TString::Format(", %ld linked elsewhere (skipped)", nScatOther).Data() : "", nTruthMismatch);
  printf("closure (methods on the generated final state), x/x_true - 1, median:\n");
  for (int k = 0; k < ny; ++k) {
    printf("  y %.2f-%.2f:", yb[k], yb[k + 1]);
    for (int m = 0; m < kNM; ++m) { auto v = gX[m][k]; std::sort(v.begin(), v.end()); printf("  %s %+.3f", kMName[m], v.empty() ? 0. : v[v.size() / 2]); }
    printf("\n");
  }
  printf("x reco/true - 1, median [16%%, 84%%], all events (failed = no valid result):\n");
  printf("  %-10s %6s", "y (true)", "events");
  for (int m = 0; m < kNM; ++m) printf(" | %-23s", kMName[m]);
  printf("\n");
  for (int k = 0; k < ny; ++k) {
    printf("  %.2f-%.2f  %6ld", yb[k], yb[k + 1], nEvY[k]);
    for (int m = 0; m < kNM; ++m) printf(" | %-23s", Spread(rX[m][k]).Data());
    printf("\n");
  }
  printf("  failed:          ");
  for (int m = 0; m < kNM; ++m) { long f = 0; for (int k = 0; k < ny; ++k) f += nFail[m][k]; printf(" | %-23ld", f); }
  printf("\nQ2 reco/true - 1, median [16%%, 84%%], all events (EICrecon = its electron method, ePIC frame assumed):\n");
  printf("  %-10s", "y (true)"); for (int m : {kE, kDA, kSig, kESig}) printf(" | %-23s", kMName[m]); printf(" | %-23s\n", "EICrecon");
  for (int k = 0; k < ny; ++k) {
    printf("  %.2f-%.2f", yb[k], yb[k + 1]);
    for (int m : {kE, kDA, kSig, kESig}) printf(" | %-23s", Spread(rQ[m][k]).Data());
    printf(" | %-23s\n", Spread(eicQ[k]).Data());
  }
  printf("x reco/true - 1 for the events with a B0-direction Lambda at x_L > %.2f:\n", xLMin);
  for (int k = 0; k < ny; ++k) {
    printf("  %.2f-%.2f  %6ld", yb[k], yb[k + 1], nSelY[k]);
    for (int m = 0; m < kNM; ++m) printf(" | %-23s", Spread(rXL[m][k]).Data());
    printf("\n");
  }
  printf("Lambda candidates (hit seeding, +-%.0f MeV, x_L > %.2f): %ld, signal %ld; with y_DA > %.2f: %ld, signal %ld\n",
         mWin * 1000, xLMin, nCand, nCandSig, yMin, nCandCut, nCandCutSig);
  printf("  per true y bin:"); for (int k = 0; k < ny; ++k) printf("  %.2f-%.2f %ld (%ld)", yb[k], yb[k + 1], nCandY[k], nCandSigY[k]); printf("\n");
  printf("  x_K reco/true - 1 (signal), all / with y_DA > %.2f:\n", yMin);
  for (int m = 0; m < kNM; ++m) printf("    %-8s %-23s / %s\n", kMName[m], Spread(dXK[m]).Data(), Spread(dXKcut[m]).Data());
  printf("  x_L reco - true (signal): %s\n", Spread(dXL).Data());

  // ---- plots ----
  gStyle->SetOptStat(0);
  TString beam = Ep == 130 ? "ep 9#times130 GeV" : Ep == 275 ? "ep 18#times275 GeV" : TString::Format("E_{p} = %.0f GeV", Ep);
  TString title = "#Lambda-tagged DIS (Pythia8), " + beam + (fsn.Contains("_old_") ? ", old B0 geometry" : fsn.Contains("_new_") ? ", new B0 geometry" : "")
                + TString::Format(", x_{L} > %.2g: DIS kinematics and x_{K}", xLMin);
  TCanvas c("c", "", 2600, 1250);
  TLatex tl; tl.SetNDC(); tl.SetTextAlign(22); tl.SetTextSize(0.028); tl.SetTextFont(62); tl.DrawLatex(0.5, 0.97, title);
  TPad pm("pm", "", 0, 0, 1, 0.94); pm.Draw(); pm.Divide(4, 2);
  pm.cd(1); gPad->SetLogx(); gPad->SetLogy(); gPad->SetRightMargin(0.13);
  hQx.Draw("colz"); hQxR.SetLineColor(kRed); hQxR.Draw("box same");
  TLegend l1(0.14, 0.8, 0.6, 0.88); l1.SetBorderSize(0); l1.AddEntry(&hQxR, "#Lambda candidate reconstructed", "f"); l1.Draw();
  pm.cd(2); gPad->SetLogx();
  hY.SetLineColor(kBlack); hY.SetLineWidth(2); hYR.SetLineColor(kRed); hYR.SetLineWidth(2);
  hY.SetMinimum(0); hY.Draw("hist"); hYR.Draw("hist same");
  TLegend l2(0.14, 0.75, 0.6, 0.88); l2.SetBorderSize(0);
  l2.AddEntry(&hY, "generated", "l"); l2.AddEntry(&hYR, "#Lambda candidate reconstructed", "l"); l2.Draw();
  pm.cd(3);
  hXK.SetLineColor(kBlack); hXK.SetLineWidth(2); hXKR.SetLineColor(kRed); hXKR.SetLineWidth(2); hXKRS.SetLineColor(kGreen + 2); hXKRS.SetLineWidth(2);
  hXK.SetMinimum(0); hXK.Draw("hist"); hXKR.Draw("hist same"); hXKRS.Draw("hist same");
  TLegend l3(0.14, 0.72, 0.62, 0.88); l3.SetBorderSize(0);
  l3.AddEntry(&hXK, "true (generated #Lambda)", "l"); l3.AddEntry(&hXKR, "candidates, hybrid x", "l"); l3.AddEntry(&hXKRS, "  of which signal", "l"); l3.Draw();
  pm.cd(4);
  hdXKh.SetLineColor(kViolet + 1); hdXKh.SetLineWidth(2); hdXKe.SetLineColor(kBlue); hdXKe.SetLineWidth(2);
  hdXKh.SetMaximum(1.3 * std::max(hdXKh.GetMaximum(), hdXKe.GetMaximum()) + 1); hdXKh.Draw("hist"); hdXKe.Draw("hist same");
  TLegend l4(0.12, 0.75, 0.62, 0.88); l4.SetBorderSize(0);
  l4.AddEntry(&hdXKh, TString::Format("hybrid, y_{DA} > %.2g", yMin), "l"); l4.AddEntry(&hdXKe, TString::Format("electron, y_{DA} > %.2g", yMin), "l"); l4.Draw();
  pm.cd(5);
  hdQ.SetLineColor(kBlue); hdQ.SetLineWidth(2); hdQe.SetLineColor(kGray + 2); hdQe.SetLineWidth(2);
  hdQ.SetMaximum(1.15 * std::max(hdQ.GetMaximum(), hdQe.GetMaximum())); hdQ.Draw("hist"); hdQe.Draw("hist same");
  TLegend l5(0.12, 0.75, 0.6, 0.88); l5.SetBorderSize(0);
  l5.AddEntry(&hdQ, "own (head-on frame from the true beams)", "l"); l5.AddEntry(&hdQe, "EICrecon (ePIC beam frame assumed)", "l"); l5.Draw();
  pm.cd(6); gPad->SetRightMargin(0.13); hdXyE.Draw("colz");
  pm.cd(7); gPad->SetRightMargin(0.13); hdXyD.Draw("colz");
  pm.cd(8);
  TPaveText t(0.03, 0.03, 0.97, 0.97, "NDC"); t.SetFillColor(0); t.SetBorderSize(0); t.SetTextAlign(12); t.SetTextFont(42); t.SetTextSize(0.034);
  t.AddText(TString::Format("%ld events, scattered e^{-} found in %.1f%%", nEv, Pct(nScat, nEv)));
  t.AddText("x reco/true - 1, median [16%, 84%]:");
  for (int k = 0; k < ny; ++k)
    t.AddText(TString::Format("  y %.2f-%.2f: e %s, DA %s", yb[k], yb[k + 1], Spread(rX[kE][k]).Data(), Spread(rX[kDA][k]).Data()));
  t.AddText(TString::Format("x_{L} > %.2g #Lambda: %ld events, %ld with y < 0.01", xLMin, nSel, nSelY[0]));
  t.AddText(TString::Format("candidates %ld (signal %ld); y_{DA} > %.2g: %ld (%ld)", nCand, nCandSig, yMin, nCandCut, nCandCutSig));
  t.AddText(TString::Format("x_{K} hybrid, y_{DA} > %.2g: %s", yMin, Spread(dXKcut[kHyb]).Data()));
  t.Draw();
  c.SaveAs(TString(outbase) + ".png");

  // ---- x_K figure ----
  TCanvas c2("c2", "", 2100, 1250);
  TLatex t2; t2.SetNDC(); t2.SetTextAlign(22); t2.SetTextSize(0.028); t2.SetTextFont(62);
  t2.DrawLatex(0.5, 0.97, "#Lambda-tagged DIS (Pythia8), " + beam + (fsn.Contains("_old_") ? ", old B0 geometry" : fsn.Contains("_new_") ? ", new B0 geometry" : "")
               + TString::Format(", x_{L} > %.2g: x_{K} = x / (1 - x_{L})", xLMin));
  TPad p2("p2", "", 0, 0, 1, 0.94); p2.Draw(); p2.Divide(3, 2);
  auto LineH = [](TH1D& h, int col, int sty = 1) { h.SetLineColor(col); h.SetLineWidth(2); h.SetLineStyle(sty); };
  // 1: generated and reconstructed (at true x_K)
  p2.cd(1);
  LineH(hXKg, kBlack); LineH(hXKgY, kGray + 2, 2); LineH(hXKs, kRed); LineH(hXKsC, kGreen + 2);
  hXKg.SetMinimum(0); hXKg.Draw("hist"); hXKgY.Draw("hist same"); hXKs.Draw("hist same"); hXKsC.Draw("hist same");
  TLegend k1(0.4, 0.66, 0.88, 0.88); k1.SetBorderSize(0); k1.SetTextSize(0.035);
  k1.AddEntry(&hXKg, "generated", "l"); k1.AddEntry(&hXKgY, TString::Format("generated, y > %.2g", yMin), "l");
  k1.AddEntry(&hXKs, "#Lambda reconstructed (signal), true x_{K}", "l");
  k1.AddEntry(&hXKsC, TString::Format("  and y_{DA} > %.2g", yMin), "l"); k1.Draw();
  // 2: reconstructed x_K per method vs true, same candidates
  p2.cd(2);
  LineH(hXKtC, kGreen + 2); LineH(hXKrH, kViolet + 1); LineH(hXKrE, kBlue, 2); LineH(hXKrD, kOrange + 7, 2);
  hXKrH.SetMaximum(1.4 * std::max({hXKtC.GetMaximum(), hXKrH.GetMaximum(), hXKrE.GetMaximum(), hXKrD.GetMaximum()}) + 1);
  hXKrH.SetTitle(TString::Format("x_{K}, signal candidates with y_{DA} > %.2g;x_{K};candidates", yMin));
  hXKrH.Draw("hist"); hXKtC.Draw("hist same"); hXKrE.Draw("hist same"); hXKrD.Draw("hist same");
  TLegend k2(0.13, 0.68, 0.55, 0.88); k2.SetBorderSize(0); k2.SetTextSize(0.035);
  k2.AddEntry(&hXKtC, "true", "l"); k2.AddEntry(&hXKrH, "reco, hybrid", "l"); k2.AddEntry(&hXKrE, "reco, electron", "l"); k2.AddEntry(&hXKrD, "reco, DA", "l"); k2.Draw();
  // 3: reco vs true, event by event
  p2.cd(3);
  TH2D fr("fr", "x_{K}, signal candidates;x_{K} (true);x_{K} (reco, hybrid)", 10, 0, 1, 10, 0, 1.5); fr.SetStats(0); fr.Draw();
  TGraph g1(gT.size(), gT.data(), gR.data()), g2(gTf.size(), gTf.data(), gRf.data());
  g1.SetMarkerStyle(20); g1.SetMarkerColor(kViolet + 1); g2.SetMarkerStyle(24); g2.SetMarkerColor(kGray + 2);
  if (g2.GetN()) g2.Draw("P same"); if (g1.GetN()) g1.Draw("P same");
  TF1 diag("diag", "x", 0, 1); diag.SetLineColor(kBlack); diag.SetLineStyle(2); diag.Draw("same");
  TLegend k3(0.13, 0.76, 0.55, 0.88); k3.SetBorderSize(0); k3.SetTextSize(0.035);
  k3.AddEntry(&g1, TString::Format("y_{DA} > %.2g", yMin), "p"); k3.AddEntry(&g2, TString::Format("y_{DA} #leq %.2g", yMin), "p"); k3.Draw();
  // 4: resolution per method
  p2.cd(4);
  LineH(hdXKm[0], kBlue); LineH(hdXKm[1], kOrange + 7); LineH(hdXKm[2], kViolet + 1);
  hdXKm[0].SetMaximum(1.4 * std::max({hdXKm[0].GetMaximum(), hdXKm[1].GetMaximum(), hdXKm[2].GetMaximum()}) + 1);
  hdXKm[0].Draw("hist"); hdXKm[1].Draw("hist same"); hdXKm[2].Draw("hist same");
  TLegend k4(0.13, 0.7, 0.62, 0.88); k4.SetBorderSize(0); k4.SetTextSize(0.035);
  k4.AddEntry(&hdXKm[0], "electron " + Spread(dXKcut[kE]), "l"); k4.AddEntry(&hdXKm[1], "DA " + Spread(dXKcut[kDA]), "l");
  k4.AddEntry(&hdXKm[2], "hybrid " + Spread(dXKcut[kHyb]), "l"); k4.Draw();
  // 5: x_K vs Q2 (true) with the reconstructed candidates
  p2.cd(5); gPad->SetLogy(); gPad->SetRightMargin(0.13);
  hXKQ.SetStats(0); hXKQ.Draw("colz");
  TGraph g3(cXK.size(), cXK.data(), cQ2.data()); g3.SetMarkerStyle(24); g3.SetMarkerColor(kRed); if (g3.GetN()) g3.Draw("P same");
  TLegend k5(0.13, 0.8, 0.6, 0.88); k5.SetBorderSize(0); k5.SetTextSize(0.035); k5.AddEntry(&g3, "#Lambda reconstructed (signal)", "p"); k5.Draw();
  // 6: numbers
  p2.cd(6);
  TPaveText t3(0.03, 0.03, 0.97, 0.97, "NDC"); t3.SetFillColor(0); t3.SetBorderSize(0); t3.SetTextAlign(12); t3.SetTextFont(42); t3.SetTextSize(0.045);
  t3.AddText(TString::Format("%ld events, %ld with a B0-direction #Lambda at x_{L} > %.2g", nEv, nSel, xLMin));
  t3.AddText(TString::Format("  of which y > %.2g (true): %.0f", yMin, hXKgY.GetEntries()));
  t3.AddText(TString::Format("signal candidates %.0f; with y_{DA} > %.2g: %.0f", hXKs.GetEntries(), yMin, hXKsC.GetEntries()));
  t3.AddText(TString::Format("x_{K} reco/true - 1 (y_{DA} > %.2g), median [16%%, 84%%]:", yMin));
  t3.AddText("  electron " + Spread(dXKcut[kE])); t3.AddText("  DA " + Spread(dXKcut[kDA])); t3.AddText("  hybrid " + Spread(dXKcut[kHyb]));
  t3.AddText(TString::Format("x_{K} true: generated median %.2f", [&] { std::vector<double> v; for (int b = 1; b <= hXKg.GetNbinsX(); ++b) for (int i = 0; i < hXKg.GetBinContent(b); ++i) v.push_back(hXKg.GetBinCenter(b)); std::sort(v.begin(), v.end()); return v.empty() ? 0. : v[v.size() / 2]; }()));
  t3.Draw();
  c2.SaveAs(TString(outbase) + "_xK.png");
}
