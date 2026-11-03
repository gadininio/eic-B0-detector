// b0_hit_origins.C
//
// Which particles make the B0 tracker hits? For each simulated B0TrackerHits hit, trace the
// MC particle that made it back through its (first) parents and classify it:
//   Lambda daughter     the particle's parent is the Lambda (decay products, or products of
//                       a Lambda interaction in material)
//   Lambda descendant   the Lambda is further up the chain (e.g. electrons from a shower
//                       started by a daughter)
//   not from Lambda     no Lambda among its ancestors
// and print a table of hits per PDG code. (Every MC particle except the gun Lambda has exactly
// one parent, so following the first parent is enough.)
//
// Input: one ddsim (edm4hep) or EICrecon (edm4eic) file.
// Usage (from the repository top):
//   root -l -b -q 'macros/exploration/b0_hit_origins.C("/eos/user/g/gninio/eic/lambda_100ev/lambda_old_s42.edm4hep.root")'
#include <map>
#include <cstdio>
#include <cstdlib>
#include "TString.h"
#include "TFile.h"
#include "TTreeReader.h"
#include "TTreeReaderArray.h"

void b0_hit_origins(const char* fname = "/eos/user/g/gninio/eic/lambda_100ev/lambda_old_s42.edm4hep.root")
{
  TFile f(fname);
  TTreeReader r("events", &f);
  TTreeReaderArray<int>          hitPart(r, "_B0TrackerHits_particle.index");
  TTreeReaderArray<int>          pdg(r, "MCParticles.PDG");
  TTreeReaderArray<unsigned int> parBeg(r, "MCParticles.parents_begin");
  TTreeReaderArray<unsigned int> parEnd(r, "MCParticles.parents_end");
  TTreeReaderArray<int>          parIdx(r, "_MCParticles_parents.index");

  std::map<int, std::map<TString, int>> table;  // pdg -> category -> hits
  std::map<TString, int> total;
  while (r.Next()) {
    for (int ip : hitPart) {
      TString cat = "not from Lambda";
      int cur = ip, depth = 0;
      while (parBeg[cur] < parEnd[cur] && depth < 50) {
        cur = parIdx[parBeg[cur]];  // follow first parent
        ++depth;
        if (std::abs(pdg[cur]) == 3122) {
          cat = depth == 1 ? "Lambda daughter" : "Lambda descendant";
          break;
        }
      }
      table[pdg[ip]][cat]++;
      total[cat]++;
    }
  }
  const char* cats[] = {"Lambda daughter", "Lambda descendant", "not from Lambda"};
  printf("%8s %16s %18s %16s\n", "PDG", cats[0], cats[1], cats[2]);
  for (auto& [p, m] : table)
    printf("%8d %16d %18d %16d\n", p, m[cats[0]], m[cats[1]], m[cats[2]]);
  printf("%8s %16d %18d %16d\n", "total", total[cats[0]], total[cats[1]], total[cats[2]]);
}
