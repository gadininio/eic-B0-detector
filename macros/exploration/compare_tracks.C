// compare_tracks.C
//
// Compare B0 hits, truth seeds and tracks between two EICrecon output files (counts of events
// with entries and of entries in total). Used to compare the old and new B0 geometry, and the
// EICrecon of the eic-shell container (a7bab3f) with EICrecon main (B0 truth seeding of #2901).
// seedsOld/seedsNew: truth-seed collection in each file. EICrecon main (#2901) renamed
// TrackerTruthSeeds -> CentralTrackerTruthSeeds + B0TrackerTruthSeeds.
// Usage (from the repository top):
//   D=/eos/user/g/gninio/eic/lambda_100ev
//   root -l -b -q "macros/exploration/compare_tracks.C(\"$D/lambda_old_s42.containerreco.edm4eic.root\",\"$D/lambda_old_s42.eicreconmain.edm4eic.root\",\"TrackerTruthSeeds\",\"B0TrackerTruthSeeds\")"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"

// nev = events with at least one entry; ntot = entries summed over events.
// sel: optional per-element selection, e.g. "Coll.charge>0".
static void count(TTree* t, const char* coll, const char* sel, double& nev, double& ntot)
{
  TString size = TString(coll) + "@.size()";
  TString perEvent = sel[0] ? TString::Format("Sum$(%s)", sel) : size;
  nev = t->GetEntries(perEvent + ">0");
  t->Draw(perEvent, "", "goff");
  ntot = 0;
  for (Long64_t i = 0; i < t->GetSelectedRows(); ++i) ntot += t->GetV1()[i];
}

void compare_tracks(const char* fold = "/eos/user/g/gninio/eic/lambda_100ev/lambda_old_s42.containerreco.edm4eic.root",
                    const char* fnew = "/eos/user/g/gninio/eic/lambda_100ev/lambda_new_s42.containerreco.edm4eic.root",
                    const char* seedsOld = "TrackerTruthSeeds",
                    const char* seedsNew = "TrackerTruthSeeds")
{
  TFile fo(fold), fn(fnew);
  TTree* to = (TTree*)fo.Get("events");
  TTree* tn = (TTree*)fn.Get("events");
  const char* R = "ReconstructedTruthSeededChargedParticles";
  TString pos = TString::Format("%s.charge>0", R), neg = TString::Format("%s.charge<0", R);
  struct Row { const char* label; const char* collOld; const char* collNew; const char* sel; };
  Row rows[] = {
    {"B0 sim hits",          "B0TrackerHits",                 "B0TrackerHits",                 ""},
    {"B0 rec hits",          "B0TrackerRecHits",              "B0TrackerRecHits",              ""},
    {"truth seeds",          seedsOld,                        seedsNew,                        ""},
    {"B0 CKF tracks",        "B0TrackerCKFTruthSeededTracks", "B0TrackerCKFTruthSeededTracks", ""},
    {"B0 charged particles", R,                               R,                               ""},
    {"  positive",           R,                               R,                               pos.Data()},
    {"  negative",           R,                               R,                               neg.Data()},
    {"central CKF tracks",   "CentralCKFTracks",              "CentralCKFTracks",              ""},
  };
  printf("old: %s\nnew: %s\n", fold, fnew);
  printf("%-24s %22s %22s\n", "", "old: events / total", "new: events / total");
  for (auto& r : rows) {
    double eo, no, en, nn;
    count(to, r.collOld, r.sel, eo, no);
    count(tn, r.collNew, r.sel, en, nn);
    printf("%-24s %10.0f / %-10.0f %10.0f / %-10.0f\n", r.label, eo, no, en, nn);
  }
  TString both = TString::Format("Sum$(%s)>0 && Sum$(%s)>0", pos.Data(), neg.Data());
  printf("%-24s %10lld %22lld\n", "events with + and -", to->GetEntries(both), tn->GetEntries(both));
  // truth: generated Lambda ending before 200 mm (old seeder cut) / 6 m (new B0 seeder cut)
  const char* L = "MCParticles.PDG==3122 && MCParticles.generatorStatus==1";
  for (int zc : {200, 6000}) {
    TString s = TString::Format("Sum$(%s && MCParticles.endpoint.z<%d)>0", L, zc);
    printf("%-24s %10lld %22lld\n", TString::Format("Lambda ends z<%d mm", zc).Data(),
           to->GetEntries(s), tn->GetEntries(s));
  }
}
