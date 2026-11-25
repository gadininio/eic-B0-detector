// kaon_kinematics.C
//
// Kaon structure function, step 2: DIS kinematics from the scattered electron and x_K = x / (1 - x_L)
// for Lambda-tagged DIS events (Pythia8, scripts/gen/), EICrecon#3116 output.
//
// DIS kinematics (electron method), Lorentz invariants from the beam four-vectors of the event
// (MCParticles with generatorStatus 4) and the scattered electron:
//   q = e_beam - e',  Q2 = -q^2,  y = P.q / P.e_beam,  x = Q2 / (2 P.q)
// true:  e' = MCScatteredElectrons (same as InclusiveKinematicsTruth, checked)
// reco:  e' = the reconstructed charged particle that EICrecon uses for InclusiveKinematicsElectron
//        (ScatteredElectronsTruth: the track truth-matched to the scattered electron; electron mass).
// Why not InclusiveKinematicsElectron itself: EICrecon rounds the beams to the ePIC frame (electron
// beam along -z, proton at -25 mrad). Our samples are boosted by ddsim --crossingAngleBoost -0.025,
// which also tilts the electron beam by 25 mrad (beam e- px/|pz| = -0.025; campaign afterburner
// files: ~1e-4), so EICrecon's reconstructed kinematics are wrong for them. The invariants
// computed here do not depend on the frame. EICrecon's numbers are printed for comparison.
//
// Lambda: generated = B0-direction Lambda (5.5-20 mrad from the proton beam) with x_L > xLMin;
// reconstructed (no truth) = forward (+,-) pair of hit-seeded tracks (beam-frame theta < 30 mrad),
// + as proton, - as pi-, p(+)/p(-) > minRatio, x_L(pair) > xLMin, |M - M_Lambda| < mWin; per event the
// pair closest to the Lambda mass. Signal = both tracks from the same Lambda -> p pi- (truth label).
// x_L = p_z(beam frame) / E_p, E_p from the file name (9x130, 10x100, 5x41, else 275 GeV).
//
// Candidates are counted with or without electron kinematics; "no e- kinematics" = no reconstructed
// scattered electron, or the electron method gives y <= 0 (frequent at y < 0.01: there y_reco =
// 1 - E'(1 - cos theta)/(2 E_e) is a small difference of large numbers). x_K needs them.
//
// Output: printout and <outbase>.png (3 x 2).
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
#include <cstdio>

namespace {
const double kMassElectron = 0.000511;   // GeV
const unsigned int kColRecoCharged = 0x538857ff;   // podio collection ID of ReconstructedChargedParticles

struct Kin { double Q2 = -1, x = -1, y = -1; bool ok = false; };
// Lorentz-invariant DIS kinematics from the beams and the scattered electron (any frame)
Kin DisKin(const TLorentzVector& eb, const TLorentzVector& pb, const TLorentzVector& es) {
  TLorentzVector q = eb - es;
  Kin k; k.Q2 = -q.M2();
  double pq = pb.Dot(q);
  if (pq <= 0 || k.Q2 <= 0) return k;
  k.x = k.Q2 / (2 * pq); k.y = pq / pb.Dot(eb); k.ok = true;
  return k;
}
// "median [16%, 84%]" of a sample
TString Spread(std::vector<double> v) {
  if (v.size() < 3) return TString::Format("n=%zu", v.size());
  std::sort(v.begin(), v.end());
  auto q = [&](double f) { return v[(size_t)(f * (v.size() - 1))]; };
  return TString::Format("%+.3f [%+.3f, %+.3f]", q(0.5), q(0.16), q(0.84));
}
std::vector<double> LogBins(int n, double lo, double hi) {
  std::vector<double> b(n + 1);
  for (int i = 0; i <= n; ++i) b[i] = lo * std::pow(hi / lo, double(i) / n);
  return b;
}
}  // namespace

void kaon_kinematics(const char* files = "/eos/user/g/gninio/eic/lambda_dis_9x130/dis_lambda_9x130_old_s40?.eicreconpr3116.edm4eic.root",
                     const char* outbase = "kin_9x130_old", double xLMin = 0.6, double minRatio = 2.5, double mWin = 0.010)
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
  TTreeReaderArray<int> mcScat(r, "MCScatteredElectrons_objIdx.index");
  TTreeReaderArray<int> scatIdx(r, "_InclusiveKinematicsElectron_scat.index");
  TTreeReaderArray<unsigned int> scatCol(r, "_InclusiveKinematicsElectron_scat.collectionID");
  TTreeReaderArray<float> tQ2(r, "InclusiveKinematicsTruth.Q2"), tx(r, "InclusiveKinematicsTruth.x"), ty(r, "InclusiveKinematicsTruth.y");
  TTreeReaderArray<float> eQ2(r, "InclusiveKinematicsElectron.Q2"), ex(r, "InclusiveKinematicsElectron.x"), ey(r, "InclusiveKinematicsElectron.y");

  // y bins for the resolution table
  const int ny = 5; const double yb[ny + 1] = {0, 0.01, 0.05, 0.2, 0.5, 1};
  std::vector<double> rQ[ny], rX[ny], rY[ny], eQ[ny], eX[ny];   // own electron method / EICrecon electron method
  long nEv = 0, nScat = 0, nScatOther = 0, nTruthMismatch = 0, nEvY[ny] = {0}, nBadY[ny] = {0};   // nBad: reco y <= 0 (P.q <= 0) or Q2 <= 0
  // Lambda side
  long nSel = 0, nSelY[ny] = {0}, nCand = 0, nCandSig = 0, nCandY[ny] = {0}, nCandSigY[ny] = {0}, nCandNoKinY[ny] = {0};
  std::vector<double> dXK[ny], dXL;

  auto bx = LogBins(30, 1e-4, 1), bq = LogBins(20, 1, 100), byy = LogBins(32, 1e-4, 1);
  TH2D hQx("hQx", "events with a B0-direction #Lambda, x_{L} > 0.6 (true);x (true);Q^{2} (true) [GeV^{2}]", 30, bx.data(), 20, bq.data());
  TH2D hQxR("hQxR", "", 30, bx.data(), 20, bq.data());
  TH1D hY("hY", "events with a B0-direction #Lambda, x_{L} > 0.6;y (true);events", 32, byy.data());
  TH1D hYR("hYR", "", 32, byy.data());
  TH1D hdQ("hdQ", "Q^{2} electron method, all events;Q^{2}_{reco} / Q^{2}_{true} - 1;events", 50, -0.5, 0.5);
  TH1D hdQe("hdQe", "", 50, -0.5, 0.5);
  auto blogy = std::vector<double>(); for (int i = 0; i <= 20; ++i) blogy.push_back(-4 + 0.2 * i);
  TH2D hdXy("hdXy", "x electron method (own, true beams), all events;log_{10} y (true);x_{reco} / x_{true} - 1", 20, -4, 0, 50, -1, 1);
  TH1D hXK("hXK", "x_{K} = x / (1 - x_{L});x_{K};events", 30, bx.data());
  TH1D hXKR("hXKR", "", 30, bx.data()), hXKRS("hXKRS", "", 30, bx.data());

  while (r.Next()) {
    ++nEv;
    TLorentzVector eb, pb;
    for (size_t i = 0; i < ev.pdg.GetSize(); ++i) {
      if (ev.status[i] != 4) continue;
      if (ev.pdg[i] == 11) eb.SetXYZM(ev.px[i], ev.py[i], ev.pz[i], kMassElectron);
      if (ev.pdg[i] == 2212) pb.SetXYZM(ev.px[i], ev.py[i], ev.pz[i], kMassProton);
    }
    if (!mcScat.GetSize() || mcScat[0] < 0) continue;
    int im = mcScat[0];
    TLorentzVector et; et.SetXYZM(ev.px[im], ev.py[im], ev.pz[im], kMassElectron);
    Kin kt = DisKin(eb, pb, et);
    if (!kt.ok) continue;
    if (tQ2.GetSize() && std::fabs(tQ2[0] / kt.Q2 - 1) > 1e-3) ++nTruthMismatch;
    int iy = -1; for (int k = 0; k < ny; ++k) if (kt.y >= yb[k] && kt.y < yb[k + 1]) iy = k;
    if (iy < 0) continue;
    ++nEvY[iy];

    // reconstructed scattered electron
    Kin kr;
    bool haveE = scatIdx.GetSize() && scatIdx[0] >= 0 && scatIdx[0] < reco.N();
    if (haveE && scatCol[0] != kColRecoCharged) { ++nScatOther; haveE = false; }
    if (haveE) {
      ++nScat;
      TLorentzVector er; er.SetVectM(reco.P(scatIdx[0]), kMassElectron);
      kr = DisKin(eb, pb, er);
      if (!kr.ok) ++nBadY[iy];
      if (kr.ok) {
        rQ[iy].push_back(kr.Q2 / kt.Q2 - 1); rX[iy].push_back(kr.x / kt.x - 1); rY[iy].push_back(kr.y / kt.y - 1);
        hdQ.Fill(kr.Q2 / kt.Q2 - 1); hdXy.Fill(std::log10(kt.y), kr.x / kt.x - 1);
      }
      if (eQ2.GetSize()) {
        eQ[iy].push_back(eQ2[0] / kt.Q2 - 1); eX[iy].push_back(ex[0] / kt.x - 1); hdQe.Fill(eQ2[0] / kt.Q2 - 1);
      }
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
      ++nSel; ++nSelY[iy];
      hQx.Fill(kt.x, kt.Q2); hY.Fill(kt.y); hXK.Fill(kt.x / (1 - xLt));
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
      if (!kr.ok) ++nCandNoKinY[iy];   // no reconstructed electron, or electron method gives y <= 0
    }
    if (ba >= 0 && kr.ok) {
      int lp = ev.LambdaOf(mc[ba], 2212), lpi = ev.LambdaOf(mc[bb], -211);
      bool sig = lp >= 0 && lp == lpi;
      double xLr = XL(reco.P(ba) + reco.P(bb)), xKr = kr.x / (1 - xLr);
      hXKR.Fill(xKr);
      if (sig) {
        hXKRS.Fill(xKr);
        double xKt = kt.x / (1 - XL(ev.P(lp)));
        dXK[iy].push_back(xKr / xKt - 1); dXL.push_back(xLr - XL(ev.P(lp)));
      }
    }
  }

  printf("%s\n", files);
  printf("events %ld (E_p = %.0f GeV); reconstructed scattered electron (ScatteredElectronsTruth track) in %ld (%.1f%%)%s\n",
         nEv, Ep, nScat, Pct(nScat, nEv), nScatOther ? TString::Format(", %ld linked to another collection (skipped)", nScatOther).Data() : "");
  printf("own true kinematics vs InclusiveKinematicsTruth: %ld events differ by > 0.1%% in Q2\n", nTruthMismatch);
  printf("electron method, reco/true - 1: median [16%%, 84%%]; own = true beams, EICrecon = InclusiveKinematicsElectron\n");
  printf("  y (true)     events  y_reco<=0 | own Q2                     own x                      own y                     | EICrecon Q2                EICrecon x\n");
  for (int k = 0; k < ny; ++k)
    printf("  %.2f-%.2f   %6ld  %9ld | %-26s %-26s %-26s| %-26s %-26s\n", yb[k], yb[k + 1], nEvY[k], nBadY[k],
           Spread(rQ[k]).Data(), Spread(rX[k]).Data(), Spread(rY[k]).Data(), Spread(eQ[k]).Data(), Spread(eX[k]).Data());
  printf("Lambda side (x_L > %.2f): events with a generated B0-direction Lambda %ld; with a reconstructed candidate (hit seeding, +-%.0f MeV) %ld, signal %ld (%.0f%%)\n",
         xLMin, nSel, mWin * 1000, nCand, nCandSig, Pct(nCandSig, nCand));
  printf("  y (true)     generated  candidates (signal)  no e- kinematics | x_K reco/true - 1 (signal)\n");
  for (int k = 0; k < ny; ++k)
    printf("  %.2f-%.2f   %9ld  %10ld (%3ld)    %10ld       | %s\n", yb[k], yb[k + 1], nSelY[k], nCandY[k], nCandSigY[k], nCandNoKinY[k], Spread(dXK[k]).Data());
  printf("  x_L reco - true (signal): %s\n", Spread(dXL).Data());

  // ---- plots ----
  gStyle->SetOptStat(0);
  TString beam = Ep == 130 ? "ep 9#times130 GeV" : Ep == 275 ? "ep 18#times275 GeV" : TString::Format("E_{p} = %.0f GeV", Ep);
  TString title = "#Lambda-tagged DIS (Pythia8), " + beam + (fsn.Contains("_old_") ? ", old B0 geometry" : fsn.Contains("_new_") ? ", new B0 geometry" : "")
                + TString::Format(", x_{L} > %.2g: DIS kinematics from the scattered electron", xLMin);
  TCanvas c("c", "", 2100, 1250);
  TLatex tl; tl.SetNDC(); tl.SetTextAlign(22); tl.SetTextSize(0.028); tl.SetTextFont(62); tl.DrawLatex(0.5, 0.97, title);
  TPad pm("pm", "", 0, 0, 1, 0.94); pm.Draw(); pm.Divide(3, 2);
  pm.cd(1); gPad->SetLogx(); gPad->SetLogy(); gPad->SetRightMargin(0.13);
  hQx.Draw("colz"); hQxR.SetMarkerStyle(24); hQxR.SetMarkerColor(kRed); hQxR.Draw("box same");
  TLegend l1(0.14, 0.78, 0.6, 0.88); l1.SetBorderSize(0); l1.AddEntry(&hQxR, "#Lambda candidate reconstructed", "f"); hQxR.SetLineColor(kRed); l1.Draw();
  pm.cd(2); gPad->SetLogx();
  hY.SetLineColor(kBlack); hY.SetLineWidth(2); hYR.SetLineColor(kRed); hYR.SetLineWidth(2);
  hY.SetMinimum(0); hY.Draw("hist"); hYR.Draw("hist same");
  TLegend l2(0.14, 0.75, 0.6, 0.88); l2.SetBorderSize(0);
  l2.AddEntry(&hY, "generated", "l"); l2.AddEntry(&hYR, "#Lambda candidate reconstructed", "l"); l2.Draw();
  pm.cd(3); gPad->SetLogx();
  hXK.SetLineColor(kBlack); hXK.SetLineWidth(2); hXKR.SetLineColor(kRed); hXKR.SetLineWidth(2); hXKRS.SetLineColor(kGreen + 2); hXKRS.SetLineWidth(2);
  hXK.SetMinimum(0); hXK.Draw("hist"); hXKR.Draw("hist same"); hXKRS.Draw("hist same");
  TLegend l3(0.14, 0.72, 0.62, 0.88); l3.SetBorderSize(0);
  l3.AddEntry(&hXK, "true (generated #Lambda)", "l"); l3.AddEntry(&hXKR, "reconstructed candidates (with e^{-} kinematics)", "l"); l3.AddEntry(&hXKRS, "  of which signal", "l"); l3.Draw();
  pm.cd(4);
  hdQ.SetLineColor(kBlue); hdQ.SetLineWidth(2); hdQe.SetLineColor(kGray + 2); hdQe.SetLineWidth(2);
  hdQ.SetMaximum(1.15 * std::max(hdQ.GetMaximum(), hdQe.GetMaximum())); hdQ.Draw("hist"); hdQe.Draw("hist same");
  TLegend l4(0.12, 0.75, 0.6, 0.88); l4.SetBorderSize(0);
  l4.AddEntry(&hdQ, "own (true beam four-vectors)", "l"); l4.AddEntry(&hdQe, "EICrecon (ePIC beam frame assumed)", "l"); l4.Draw();
  pm.cd(5); gPad->SetRightMargin(0.13); hdXy.Draw("colz");
  pm.cd(6);
  TPaveText t(0.05, 0.05, 0.95, 0.95, "NDC"); t.SetFillColor(0); t.SetBorderSize(0); t.SetTextAlign(12); t.SetTextFont(42);
  t.AddText(TString::Format("%ld events, scattered e^{-} reconstructed in %.1f%%", nEv, Pct(nScat, nEv)));
  t.AddText("electron method, reco/true - 1, median [16%, 84%]:");
  for (int k = 0; k < ny; ++k)
    t.AddText(TString::Format("  y %.2f-%.2f (%ld): Q^{2} %s, x %s", yb[k], yb[k + 1], nEvY[k], Spread(rQ[k]).Data(), Spread(rX[k]).Data()));
  t.AddText(TString::Format("B0-direction #Lambda, x_{L} > %.2g: %ld events; candidates %ld (signal %ld)", xLMin, nSel, nCand, nCandSig));
  t.Draw();
  c.SaveAs(TString(outbase) + ".png");
}
