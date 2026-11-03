# Commands run in Claude sessions

What each step was for is in `claude_log.md`. Commands run from the Mac go through `ssh lxplus '...'`; they are shown here as you'd type them on lxplus.

## 2026-11-02

### Connect / setup
```bash
ssh lxplus                      # 2FA once; connection reused for 8h
ssh -O exit lxplus              # close the master connection (e.g. to pick up X forwarding)
# ROOT on lxplus:
source /cvmfs/sft.cern.ch/lcg/views/LCG_107/x86_64-el9-gcc13-opt/setup.sh
```

### AFS quota investigation
```bash
fs listquota ~
cd ~ && du -sh .[!.]* * 2>/dev/null | sort -rh | head -30
du -sh private/* .cache/* .vscode-server/cli/servers/* | sort -rh
cat ~/.vscode-server/cli/servers/lru.json      # most recently used VS Code server first
```

### Cleanup (run by the user)
```bash
pip cache purge
rm -rf ~/.cache/pip ~/.cache/vscode-cpptools
rm -rf ~/.vscode-server/cli/servers            # user removed the whole folder
```

### EOS offload (cancelled; rerun later; rsync resumes where it stopped)
```bash
D=/eos/user/g/gninio/afs_offload/yytautauAnalysis
mkdir -p $D/Fitters/HGGfitter/run $D/hgam
rsync -a ~/private/yytautauAnalysis/Fitters/HGGfitter/run/yytautau $D/Fitters/HGGfitter/run/
rsync -a ~/private/yytautauAnalysis/hgam/build $D/hgam/
# verify before removing anything from AFS:
rsync -a --dry-run --itemize-changes ~/private/yytautauAnalysis/Fitters/HGGfitter/run/yytautau $D/Fitters/HGGfitter/run/
```

### lambda_b0: ROOT history found in ~/.root_hist
```cpp
TFile *_file0 = TFile::Open("lambda_b0.edm4hep.root")
.ls
events->Scan("MCParticles.PDG:MCParticles.endpoint.z", "MCParticles.PDG==3122", "", 20)
```

### lambda_b0: B0 tracker hits (rebuilt and tested)
In `~/private/eic/lambda_b0`, `root -l lambda_b0.edm4hep.root`:
```cpp
// which hit collections have hits, and in how many events
for (auto b : *events->GetListOfBranches()) { TString n = b->GetName(); if (n.Contains("Hit") && !n.BeginsWith("_")) cout << n << "  " << events->GetEntries(n + "@.size()>0") << endl; }

events->Print("*B0TrackerHits*")
events->GetEntries("B0TrackerHits@.size()>0")
events->Scan("B0TrackerHits.position.x:B0TrackerHits.position.y:B0TrackerHits.position.z:B0TrackerHits.eDep:_B0TrackerHits_particle.index", "", "", 20)
events->Draw("Length$(B0TrackerHits.position.z)")                  // hits per event
events->Draw("B0TrackerHits.position.z")                           // layer z positions
events->Draw("B0TrackerHits.position.y:B0TrackerHits.position.x", "", "colz")

// particle type that made each hit
events->Scan("B0TrackerHits.position.z:MCParticles.PDG[_B0TrackerHits_particle.index]", "", "", 20)
events->Draw("MCParticles.PDG[_B0TrackerHits_particle.index]>>h(5000,-2500,2500)")
events->Draw("MCParticles.generatorStatus[_B0TrackerHits_particle.index]")

// generator-level content (one Lambda per event)
events->Scan("MCParticles.PDG:MCParticles.generatorStatus:MCParticles.momentum.z", "MCParticles.generatorStatus!=0")
```

### lambda_b0: which B0 hits come from the Λ
Macro `~/private/eic/lambda_b0/b0_hit_origins.C`: follows each hit's MC particle up through its first parent and sorts hits into Λ daughter / deeper Λ descendant / not from Λ, by PDG code.
```bash
cd ~/private/eic/lambda_b0
root -l -b -q b0_hit_origins.C
```

# SSH FS workspace for lxplus eic
code ~/code/epic.code-workspace
# mount lxplus eic via sshfs, reusing the `ssh lxplus` master (no 2FA)
mkdir -p ~/mounts/lxplus-eic && sshfs -o volname=lxplus-eic,reconnect,ServerAliveInterval=30 lxplus:/afs/cern.ch/user/g/gninio/private/eic ~/mounts/lxplus-eic
umount ~/mounts/lxplus-eic
# (reverted) using SSH FS extension instead of the sshfs mount
umount ~/mounts/lxplus-eic && rmdir ~/mounts/lxplus-eic

# Notes sync (run on the Mac from ~/code/epic)
rsync -av lxplus:/afs/cern.ch/user/g/gninio/private/eic/notes/ notes/                  # lxplus notes -> local
rsync -av claude_log.md claude_commands.md lxplus:/afs/cern.ch/user/g/gninio/private/eic/notes/   # Claude logs -> lxplus

# lambda_b0: number of parents per MC particle (checks that following the first parent is enough)
# in ~/private/eic/lambda_b0, root -l lambda_b0.edm4hep.root
events->Draw("MCParticles.parents_end-MCParticles.parents_begin")

# lambda_b0: explore edm4eic file (in ~/private/eic/lambda_b0, root -l lambda_b0.edm4eic.root)
.ls
// non-empty collections, with number of events and total entries
for (auto b : *events->GetListOfBranches()) { TString n = b->GetName(); if (n.BeginsWith("_") || n.Contains("Contributions")) continue; Long64_t ne = events->GetEntries(n + "@.size()>0"); if (ne > 0) cout << n << "  events=" << ne << endl; }
events->SetScanField(0)
events->Scan("Entry$:ReconstructedTruthSeededChargedParticles.charge:ReconstructedTruthSeededChargedParticles.momentum.z", "ReconstructedTruthSeededChargedParticles@.size()>0")
events->Scan("Entry$:TrackerTruthSeedParameters.qOverP:TrackerTruthSeedParameters.theta", "TrackerTruthSeedParameters@.size()>0")
// where the generated Lambda ends (m)
events->Draw("MCParticles.endpoint.z/1000", "MCParticles.PDG==3122 && MCParticles.generatorStatus==1")

# check which epic commit the PR merge sits on (from the Mac)
curl -s https://api.github.com/repos/eic/epic/commits/ec774d8     # "parents" lists 43f427e
# B0 hit z layers (in ~/private/eic/lambda_b0, root -l lambda_b0.edm4hep.root)
events->Draw("B0TrackerHits.position.z>>hz(1200,5800,7000)")

# lambda_b0: Lambda mass from true daughter momenta
cd ~/private/eic/lambda_b0
root -l -b -q lambda_mass_truth.C        # prints per-event M(p pi-), decay counts; writes lambda_mass_truth.png

# which epic commit is inside the nightly eic-shell container
ls -d /cvmfs/singularity.opensciencegrid.org/eicweb/eic_xl:nightly/opt/software/linux-x86_64_v2/epic-git.*
ls -d /cvmfs/singularity.opensciencegrid.org/eicweb/eic_xl:*       # available container versions

# build epic main locally (on lxplus)
cd ~/private/eic
git clone --depth 1 https://github.com/eic/epic.git epic
cd epic
# (eic-shell -- bash -c "..." fails with "Illegal option --"; pipe the commands in instead)
printf "%s\n" "cd ~/private/eic/epic" "cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install && cmake --build build -j8 && cmake --install build" | ../eic-shell > build.log 2>&1
# use it (inside eic-shell); skip this line to use the container's nightly epic instead
source ~/private/eic/epic/install/bin/thisepic.sh       # when piping commands into eic-shell, run `set --` first
echo $DETECTOR_PATH $DETECTOR_CONFIG        # should point to ~/private/eic/epic/install/share/epic

# rerun lambda_b0 with local epic main (on lxplus); script ~/private/eic/lambda_b0_main/run_sim.sh
cd ~/private/eic/lambda_b0_main
cat run_sim.sh | ../eic-shell > sim.log 2>&1
# EICrecon B0 hit threshold in main:
curl -s https://raw.githubusercontent.com/eic/EICrecon/main/src/detectors/B0TRK/B0TRK.cc | grep -n threshold
cat run_reco.sh | ../eic-shell > reco.log 2>&1        # eicrecon on the new simulation

# compare old vs new geometry: hits, seeds, tracks (on lxplus)
cd ~/private/eic
root -l -b -q compare_tracks.C          # defaults: lambda_b0/ vs lambda_b0_main/ edm4eic files

# build EICrecon main locally (on lxplus)
cd ~/private/eic
git clone --depth 1 https://github.com/eic/EICrecon.git EICrecon
cd EICrecon
printf "%s\n" "cd ~/private/eic/EICrecon" "cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install -DCMAKE_BUILD_TYPE=Release && cmake --build build -j8 && cmake --install build" | ../eic-shell > build.log 2>&1
# what changed between the container EICrecon and main (from the Mac)
curl -s https://api.github.com/repos/eic/EICrecon/compare/a7bab3fad5b6b30d077e276137ddf1a58ef268e1...main

# reconstruct with local EICrecon main (on lxplus, in ~/private/eic)
(echo "DIR=lambda_b0 GEO=container"; cat run_reco_eicreconmain.sh) | ./eic-shell > lambda_b0/reco_eicreconmain.log 2>&1
(echo "DIR=lambda_b0_main GEO=local"; cat run_reco_eicreconmain.sh) | ./eic-shell > lambda_b0_main/reco_eicreconmain.log 2>&1
# compare container EICrecon vs main, per geometry (seed collection was renamed in main)
root -l -b -q 'compare_tracks.C("lambda_b0/lambda_b0.edm4eic.root","lambda_b0/lambda_b0.eicreconmain.edm4eic.root","TrackerTruthSeeds","B0TrackerTruthSeeds")'
root -l -b -q 'compare_tracks.C("lambda_b0_main/lambda_b0.edm4eic.root","lambda_b0_main/lambda_b0.eicreconmain.edm4eic.root","TrackerTruthSeeds","B0TrackerTruthSeeds")'

# Lambda mass from reconstructed B0 tracks, truth-matched (on lxplus, in ~/private/eic)
root -l -b -q 'lambda_mass_reco.C+("lambda_b0/lambda_b0.eicreconmain.edm4eic.root","lambda_b0/mass_reco")'          # old geometry
root -l -b -q 'lambda_mass_reco.C+("lambda_b0_main/lambda_b0.eicreconmain.edm4eic.root","lambda_b0_main/mass_reco")'  # new geometry
# per-track list (charge, p_reco, p_true, PDG, vertex z, match weight, tag) is printed; saved as <sample>/mass_reco.txt
# interactively, reco-to-MC link (in root -l <file>.edm4eic.root):
events->Scan("_ReconstructedTruthSeededChargedParticleAssociations_rec.index:_ReconstructedTruthSeededChargedParticleAssociations_sim.index:ReconstructedTruthSeededChargedParticleAssociations.weight", "ReconstructedTruthSeededChargedParticleAssociations@.size()>0")

# Geometrical B0 acceptance with a geantino gun (on lxplus, in ~/private/eic)
(echo "GEO=container N=5000 PART=geantino"; cat acceptance/run_acc.sh) | ./eic-shell > acceptance/sim_container.log 2>&1   # old geometry
(echo "GEO=local N=5000 PART=geantino"; cat acceptance/run_acc.sh) | ./eic-shell > acceptance/sim_local.log 2>&1           # new geometry
#   run_acc.sh = ddsim with the lambda_b0 gun settings, --gun.particle geantino, 100 GeV, and --filter.tracker "" (keep zero-energy hits)
cd acceptance
root -l -b -q 'b0_acceptance.C+("acc_geantino_container_5000.edm4hep.root","acc_container")'
root -l -b -q 'b0_acceptance.C+("acc_geantino_local_5000.edm4hep.root","acc_local")'
# local backup copies of the macros (on the Mac): ~/code/epic/macros/{b0_acceptance.C,run_acc.sh,lambda_mass_reco.C}
scp lxplus:private/eic/acceptance/{b0_acceptance.C,run_acc.sh} lxplus:private/eic/lambda_mass_reco.C macros/

# Lambda daughter acceptance (full simulation; on lxplus, in ~/private/eic)
root -l -b -q 'lambda_daughter_acceptance.C+("lambda_b0/lambda_b0.edm4hep.root","lambda_b0/daughter_acc")'
root -l -b -q 'lambda_daughter_acceptance.C+("lambda_b0_main/lambda_b0.edm4hep.root","lambda_b0_main/daughter_acc")'
# larger samples: 4 detached jobs (EOS is not visible inside eic-shell, so write to the node's /tmp; stay on the same lxplus node!)
mkdir -p /tmp/gninio_lambda_acc
for G in container local; do for S in 101 102; do
  nohup setsid bash -c "(echo \"GEO=$G N=500 SEED=$S\"; cat lambda_acc/run_lambda_acc.sh) | ./eic-shell > /tmp/gninio_lambda_acc/sim_${G}_s${S}.log 2>&1" >/dev/null 2>&1 < /dev/null &
done; done
grep -l SIM_OK /tmp/gninio_lambda_acc/sim_*.log      # which jobs finished
# daughter acceptance, now in ~/private/eic/lambda_acc (file pattern = several files combined)
cd ~/private/eic/lambda_acc
root -l -b -q 'lambda_daughter_acceptance.C+("../lambda_b0/lambda_b0.edm4hep.root","daughter_acc_old_100ev")'
root -l -b -q 'lambda_daughter_acceptance.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_local_s*.edm4hep.root","daughter_acc_new")'
root -l -b -q 'lambda_daughter_acceptance.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_container_s*.edm4hep.root","daughter_acc_old")'
# move finished samples from the node's /tmp to EOS (outside eic-shell)
cp /tmp/gninio_lambda_acc/lambda_*.edm4hep.root /eos/user/g/gninio/eic/lambda_acc/

# reconstruct the 1000-event samples (on lxplus, in ~/private/eic; detached, one work folder per job)
for N in container_s101 container_s102 local_s101 local_s102; do G=${N%_s*}
  nohup setsid bash -c "(echo \"GEO=$G NAME=$N\"; cat lambda_acc/run_reco_lambda_acc.sh) | ./eic-shell > /tmp/gninio_lambda_acc/reco_$N.log 2>&1" >/dev/null 2>&1 < /dev/null &
done
# same, with extra eicrecon options, e.g. no seed momentum smearing (key needs the "tracking:" plugin prefix)
nohup setsid bash -c "(echo \"GEO=local NAME=local_s101 TAG=_nosmear EXTRA=-Ptracking:B0TrackerTruthSeeds:MomentumSmear=0\"; cat lambda_acc/run_reco_lambda_acc.sh) | ./eic-shell > /tmp/gninio_lambda_acc/reco_local_s101_nosmear.log 2>&1" >/dev/null 2>&1 < /dev/null &
# acceptance + reconstruction efficiency (in ~/private/eic/lambda_acc)
root -l -b -q 'lambda_daughter_acceptance.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_local_s*.eicreconmain.edm4eic.root","daughter_acc_new")'
# where daughters are lost: seed -> CKF -> filter -> reco particle
root -l -b -q 'daughter_stage_efficiency.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_local_s*.eicreconmain.edm4eic.root")'
# tracking efficiency only (denominator = accepted daughters), plots trk_eff_*.png (in ~/private/eic/lambda_acc)
root -l -b -q 'daughter_stage_efficiency.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_container_s10?.eicreconmain.edm4eic.root","trk_eff_old")'
root -l -b -q 'daughter_stage_efficiency.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_local_s10?.eicreconmain.edm4eic.root","trk_eff_new")'
# 5 keV B0 threshold (on lxplus, in ~/private/eic); then the same macros on *_5keV files
for N in container_s101 container_s102 local_s101 local_s102; do G=${N%_s*}
  nohup setsid bash -c "(echo \"GEO=$G NAME=$N TAG=_5keV EXTRA=-PB0TRK:B0TrackerRawHits:threshold=5e-6\"; cat lambda_acc/run_reco_lambda_acc.sh) | ./eic-shell > /tmp/gninio_lambda_acc/reco_${N}_5keV.log 2>&1" >/dev/null 2>&1 < /dev/null &
done
# Lambda mass, 1000-event samples (in ~/private/eic/lambda_acc; macro moved here)
root -l -b -q 'lambda_mass_reco.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_container_s10?.eicreconmain.edm4eic.root","mass_reco_old_10keV")'
root -l -b -q 'lambda_mass_reco.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_local_s10?.eicreconmain.edm4eic.root","mass_reco_new_10keV")'
root -l -b -q 'lambda_mass_reco.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_local_s10?.eicreconmain_5keV.edm4eic.root","mass_reco_new_5keV")'
root -l -b -q 'lambda_mass_reco.C+("../lambda_b0/lambda_b0.eicreconmain.edm4eic.root","mass_reco_old_100ev",1)'   # 1 = print every track
# Lambda mass without truth matching (in ~/private/eic/lambda_acc)
root -l -b -q 'lambda_mass_notruth.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_local_s10?.eicreconmain_5keV.edm4eic.root","mass_notruth_new_5keV")'
root -l -b -q 'lambda_mass_notruth.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_container_s10?.eicreconmain.edm4eic.root","mass_notruth_old_10keV")'
# 3rd argument = min p(+)/p(-), default 2.5
# check of the p(+)/p(-) cut: true and reco momentum ratio (in ~/private/eic/lambda_acc)
root -l -b -q 'lambda_ratio_truth.C+("/eos/user/g/gninio/eic/lambda_acc/lambda_local_s10?.eicreconmain_5keV.edm4eic.root","ratio_new_5keV")'

## 2026-11-03 (paths above this line are the OLD layout; see README.md of the repository)

# repository working copy on lxplus
cd ~/private/eic/eic-B0-detector
scripts/make_results.sh                 # all results from /eos/user/g/gninio/eic (or e.g. scripts/make_results.sh 04 06)
nohup setsid scripts/production/lambda_sample.sh local lambda_new_s101 101 500 > /tmp/$USER/new_s101.log 2>&1 < /dev/null &
scripts/production/geantino_sample.sh local geantino_new_5000 5000
cd docs/report && latexmk -pdf report.tex; cd ../slides && latexmk -pdf slides.tex
# reach another lxplus node (e.g. its /tmp) from inside lxplus
ssh lxplus9116.cern.ch

# push the lxplus repo to GitHub through the Mac (lxplus key not on GitHub)
git clone lxplus:private/eic/eic-B0-detector pushclone && cd pushclone
git push git@github.com:gadininio/eic-B0-detector.git main
# on lxplus: record the pushed state without contacting GitHub
git update-ref refs/remotes/origin/main 54970c3 && git branch -u origin/main main

# lxplus -> GitHub over ssh (GitHub requires RSA >= 3072 bit; use ed25519)
ssh-keygen -t ed25519 -N "" -C "gadi.ninio@cern.ch lxplus" -f ~/.ssh/id_ed25519
cat ~/.ssh/id_ed25519.pub     # paste at https://github.com/settings/keys (Authentication Key)
ssh -T git@github.com         # "Hi gadininio! ..."
cd ~/private/eic/eic-B0-detector && git push
