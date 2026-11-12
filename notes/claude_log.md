# Claude session log

Commands for each step are in `claude_commands.md`.

## 2026-11-02

### Connecting to lxplus
- No SSH key called "lxplus" exists. `~/.ssh/config` has a host alias `lxplus` (user `gninio`) that reuses one 2FA login for 8h (ControlMaster).
- Connected through the existing master connection, landing on `lxplus956.cern.ch`.

### AFS quota at 96%
- Quota is 10 GB; 9.6 GB was used.
- Largest items: `~/.cache/pip` 1.7G, `~/.cache/vscode-cpptools` 1.5G, `~/.vscode-server/cli/servers` 1.9G (3 old versions, plus a failed `.staging` install of a 4th), `private/yytautauAnalysis/Fitters/HGGfitter/run/yytautau` 1.5G, `private/yytautauAnalysis/hgam/source/HGamCore` 922M, `private/yytautauAnalysis/hgam/build` 636M.
- Plan: (1) clear pip and cpptools caches, (2) remove old VS Code servers, (3) move fit outputs and hgam build to EOS.
- Steps 1 and 2: Claude Code's permission check blocked the deletions over ssh. **The user ran them themselves.** This removed all of `~/.vscode-server/cli/servers` (VS Code downloads it again on the next connection). Usage went from 96% to **46%**.
- Step 3: started an `rsync -a` to `/eos/user/g/gninio/afs_offload/yytautauAnalysis/`. **Cancelled at the user's request** (to rerun later). A partial copy (~1.2G) remains on EOS. Nothing was removed from AFS.

### ROOT commands on lambda_b0
- Wanted: the ROOT commands from about two weeks earlier that looked at tracker hits in `~/private/eic/lambda_b0/lambda_b0.edm4hep.root`.
- Not recoverable. `~/.root_hist` has no dates and is truncated; its only lambda_b0 lines are an `MCParticles` scan for PDG 3122. No saved Claude transcript or lxplus script contains them. The file was also regenerated on 2026-11-02 at 10:11.
- Rebuilt the commands instead (see `claude_commands.md`).

### lambda_b0 tracker hits
- The sample is a **single-Λ particle gun**: 100 events, each with one generated Λ (PDG 3122, status 1) with p_z of about 50–100 GeV, pointing forward.
- The central tracker collections (`SiBarrelHits`, `VertexBarrelHits`, `TrackerEndcapHits`) are empty. Tracker hits are in **`B0TrackerHits`** (77 of 100 events, 1478 hits), in layers at z ≈ 5900, 6170, 6440 and 6710 mm.
- Hits by particle type: e⁺ 538, e⁻ 280, π⁻ 269, p 218, π⁺ 95, n 36, γ 21, K± 14, μ± 6, ³He 1. No hit comes from a generator-level particle; all are from particles Geant4 produced.
- Hits traced back through parent particles (macro `~/private/eic/lambda_b0/b0_hit_origins.C`):
  - 392 hits from **direct Λ daughters**: π⁻ 182, p 171, π⁺ 26, K± 11, n 2.
  - 1086 hits from **deeper Λ descendants** (e.g. all 818 e± hits, 87 π⁻, 47 p).
  - 0 hits not from a Λ. Expected, since the Λ is the only generated particle.
  - Direct daughters that are not p or π⁻ (π⁺, K, n) point to Λ hadronic interactions in material rather than decays. Not yet verified.

### X forwarding
- Added `ForwardX11 yes` and `ForwardX11Trusted yes` (equivalent to `ssh -XY`) to the `lxplus` alias. XQuartz is installed on the Mac.
- Takes effect only after the master connection is restarted (`ssh -O exit lxplus`, then `ssh lxplus` with 2FA).

### Open items
- Rerun the EOS offload (`rsync -a` resumes where it stopped); verify; remove AFS originals only with approval.
- Restart the lxplus master connection to get graphics.

## 2026-11-02 — SSH FS workspace for lxplus eic
- Created ~/code/epic.code-workspace (same pattern as jettagger.code-workspace): local `epic` + `ssh://lxplus/afs/cern.ch/user/g/gninio/private/eic` via the SSH FS extension (Kelvin.vscode-sshfs), keyboard-interactive login (password + 2FA).
- Switched: the SSH FS extension (ssh2 library) can't reuse the OpenSSH ControlMaster, so I mounted with the command-line sshfs instead (`lxplus:` alias, reuses the master, no 2FA) at ~/mounts/lxplus-eic. The workspace now opens that path.
- Reverted to the SSH FS extension (ssh://lxplus/... in epic.code-workspace) at the user's request; it is fast but logs in separately (password + 2FA). Unmounted and removed ~/mounts/lxplus-eic.

## 2026-11-02 — Notes sync
- Copied lxplus `~/private/eic/notes` (log.md + 3 screenshots) to local `~/code/epic/notes`.
- Copied `claude_log.md` and `claude_commands.md` into lxplus `~/private/eic/notes`. Both copies are kept in sync after each update.
- One-way copies only (no `--delete`), so nothing is removed on either side.

## 2026-11-02 — What "direct Λ daughter" / "further down the chain" mean
The macro `b0_hit_origins.C` starts at the MC particle that made each B0 hit and steps back through its parents in `MCParticles` until it reaches the Λ. The categories count generations between that particle and the Λ:
- **Direct Λ daughter:** the particle's parent is the Λ itself, so it was created where the Λ ended, in its decay (Λ → p π⁻ or n π⁰) or in a collision of the Λ with detector material. Example: the decay proton crosses the B0 tracker and leaves a hit.
- **Further down the chain** (called "Lambda descendant" in the macro output): the Λ is a grandparent or older ancestor. Example: Λ → p π⁻; the π⁻ hits material and makes a π⁰; π⁰ → γγ; one γ converts to e⁺e⁻; the e⁺ leaves the hit. That e⁺ is four generations below the Λ.
- So the 392 direct hits are mostly the decay p and π⁻ themselves. The 1086 others come from what the Λ and its products went on to produce in the material, mainly e± showers.

Caveats:
- **Saved particles only.** The simulation doesn't save every low-energy secondary. A hit made by an unsaved particle is assigned to its nearest saved ancestor. That's why 36 hits are assigned to neutrons, which leave no charge trail themselves. Some "direct" hits were really made further down the chain.
- **First parent only.** The macro follows only each particle's first parent. Checked: in this file the 100 generated Λs have no parent and all 840,146 other particles have exactly one, so nothing is missed.

## 2026-11-02 — Goal: reconstruct the Λ mass from tracks
Plan, step by step. Step 1: explore the reconstruction file `~/private/eic/lambda_b0/lambda_b0.edm4eic.root`.

### Step 1: explore lambda_b0.edm4eic.root
- Trees: `events` (100 entries), `metadata`, `runs`, `podio_metadata`. Same 100 events as the edm4hep file; `MCParticles` is copied in.
- **B0 tracker:** hits are reconstructed (`B0TrackerRecHits`: 1416 hits in 76 events), but there are only **4 tracks in 3 events** (`B0TrackerCKFTruthSeededTracks`, `ReconstructedTruthSeededChargedParticles`). Only truth-seeded B0 tracking ran; there is no realistic-seeded B0 track collection.
- Truth seeds (`TrackerTruthSeeds`) exist in only **4 events** (39, 53, 58, 85), always 2 per event: one negative with p ≈ 5–14 GeV and one positive with p ≈ 50–65 GeV (from q/p), consistent with Λ → p π⁻. The other 96 events got no seed, so no B0 track could be built. Likely reason, not verified: the truth seeder only takes certain MC particles (e.g. generator-level or near the IP), and the Λ daughters come from Geant4 (generatorStatus 0).
- Reconstructed B0 charged particles: event 39 (+, p_z 57.9 GeV), event 53 (−, 14.0 GeV and +, 65.6 GeV), event 85 (+, 57.6 GeV). PDG = 0 (no PID). **Only event 53 has both a + and a − track**, which is one Λ mass candidate.
- Central tracker: almost only noise hits (`SiBarrel*`, `SiEndcap*` NoiseRawHits); 1 track in the whole file. Expected, since the Λs go forward.
- Where the generated Λ ends (`MCParticles.endpoint.z`, generatorStatus 1): 32 within 2 m, 31 at 2–5 m, 37 at 5–9 m. The B0 tracker layers are at z ≈ 5.9–6.7 m, so many Λs end inside or past it, which leaves too little lever arm for tracking.
- Pitfall: `PDG==3122` without `generatorStatus==1` also counts secondary Λs from material interactions (179 entries instead of 100).

### Implication for the goal
With the reconstruction as it was run, the file gives about one Λ candidate from reco tracks. Options are listed in the chat; nothing has been decided yet.

### Only 4 tracks: bug or physics? (checked against eic/epic PR #1194)
- **PR #1194** ("B0_tracker: realistic geometry with AC-LGAD staves", merged 2026-11-01 as `ec774d8`; read from the GitHub page, not the diff) is a **geometry** change: realistic 4-station B0 tracker, a fix for the B0 exit window built 8 mm thick instead of 4 mm, and a lower B0 digitization threshold in EICrecon (5 keV). It says explicitly that B0 tracking is still **truth-seeded only**; realistic B0 seeding is tracked in eic/EICrecon#2930.
- **Our files predate it.** They were made with epic `43f427e` (from the fieldmap symlinks in `lambda_b0/fieldmaps`). The GitHub compare shows that commit is 1 commit behind `ec774d8`, i.e. main just before the PR merged. So they have the old B0 geometry and the 8 mm window.
- **Why only 4 events have tracks (from the data):** Λs that end before z = 0.5 m: events 15 (z = 61 mm), 39 (13), 85 (94), 53 (190), 58 (197), 90 (269), 82 (317), 95 (407), 63 (443). The seeded events are exactly 39, 53, 58, 85: all 4 Λs ending at z < 200 mm except event 15, and none beyond. This points to the truth seeder only accepting particles produced within about 20 cm of the IP. Event 15 is probably Λ → n π⁰ (no charged daughters). Neither the cut value nor event 15's decay mode has been checked yet.
- **Conclusion:** the 4 tracks are a **reconstruction limitation (truth-seeding vertex cut), not physics**. PR #1194 does not change seeding, so rerunning with it alone would not fix it.

### Is the detector configuration "main" with PR #1194?
- The build used is labelled `main`, but it is main **as of epic commit `43f427e`** (30 Oct 2026; path `/opt/software/.../epic-git.43f427e..._main-...`, seen through the `lambda_b0/fieldmaps` symlinks).
- On GitHub, the PR merge commit `ec774d8` (1 Nov 2026 14:36 UTC) has exactly one parent, `43f427e`. So the build is main from just **before** the merge.
- The B0 hits agree: 4 layers, each about 6 mm wide in z (5896–5902, 6166–6172, 6435–6442, 6706–6712 mm). That fits the old single-plane-per-station geometry; the new one has sensors on both faces of each station. Not proven, but it agrees with the commit evidence.
- To use the PR: a software build from main after 1 Nov 2026 14:36 UTC is needed (e.g. a newer eic-shell nightly). Not yet checked which build is available.

### Step 2: Λ mass from true momenta
Macro `~/private/eic/lambda_b0/lambda_mass_truth.C` (on `lambda_b0.edm4eic.root`): for the generated Λ, looks at its direct daughters and computes M(p π⁻) from their true momenta at production. Plot saved to `lambda_b0/lambda_mass_truth.png`.
- **Λ → p π⁻: 45 events. Λ → n π⁰: 27. Other: 28.** p π⁻ is 62.5% of decays, close to the expected 64%.
- **"Other" = the Λ hit material before decaying** (6–134 daughters). These end at z ≈ 5.8, 6.9–7.1, 7.9, 8.1 and 8.5 m, i.e. in the B0/forward region material.
- **M(p π⁻) = 1.115683 GeV for every event** (RMS 1.5e-8 GeV), exactly the PDG value. Expected: the simulation conserves energy and momentum exactly at the decay, with PDG masses. So this only checks the mass code and the daughter lookup, not the detector.
- The p π⁻ decays are spread from z = 13 mm to 7.3 m. The 4 seeded events (39, 53, 58, 85) are all p π⁻ decays at z < 200 mm. Event 90 (p π⁻ at 269 mm) has no seed, consistent with a ~20 cm cut in the truth seeder. Event 15 (Λ ending at 61 mm, no seed) is in neither the p π⁻ nor the 'other' list, so it is Λ → n π⁰. That confirms the earlier guess.

### Which version am I using? (clarified)
- `main` is a moving branch. PR #1194 was developed on `b0_trackers_realistic_geometry`, merged into main on 1 Nov 2026 14:36 UTC (merge commit `ec774d8`), and that branch was deleted. The PR is now only in main.
- The software comes from `~/private/eic/eic-shell` (ORGANIZATION=eicweb, CONTAINER=eic_xl, VERSION=nightly → `/cvmfs/singularity.opensciencegrid.org/eicweb/eic_xl:nightly`). That container holds epic main at `43f427e` (30 Oct), the commit just before the merge.
- As of 2026-11-02 the nightly container on CVMFS **still** has `43f427e`, so it does not contain the PR yet.
- Ways to get it: wait until the CVMFS nightly shows a commit after `ec774d8`, or build epic main yourself inside eic-shell and source its setup script.

### Local epic build: where, how big, reversible?
- The nightly epic is built by the EIC developers into the `eic_xl:nightly` container on CVMFS (shared and read-only), so it uses no AFS space.
- Installed epic in the container is 114 MB (measured). GitHub repo is 114 MB with full history; a shallow clone is smaller. The build folder is probably a few hundred MB (estimate). Total for a local build is roughly 0.3–0.5 GB. AFS is at 53% (4.9 GB free).
- Reversible: a local build is only used in a shell where `source <install>/bin/thisepic.sh` is run. Without it, eic-shell uses the container's epic. Record which geometry each output file was simulated with.

### Building epic main locally (~/private/eic/epic)
- Shallow-cloned `eic/epic` main into `~/private/eic/epic`. HEAD = `ec774d8` (the PR #1194 merge, 2026-11-01). Clone is 3.4 MB.
- Build and install inside eic-shell (nightly container, which provides DD4hep/Geant4): `cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install`, `cmake --build build -j8`, `cmake --install build`. Log in `~/private/eic/epic/build.log`.
- First attempt with `../eic-shell -- bash -c "..."` did nothing ("Illegal option --"): the container's entrypoint `/opt/local/bin/eic-shell` runs `bash -c "$@"`, so `--` reaches bash. It works by piping the commands on stdin: `printf '%s\n' cmd1 cmd2 | ../eic-shell`.
- To use it: inside eic-shell, `source ~/private/eic/epic/install/bin/thisepic.sh`. Without that line, the container's epic (43f427e) is used.
- **Build OK** (no errors). `build/` 75 MB, `install/` 23 MB. AFS at 54%.
- Checked: after `source install/bin/thisepic.sh`, `DETECTOR_PATH` = `~/private/eic/epic/install/share/epic` (before: the container's `epic-git.43f427e…`). The installed `B0_tracker.xml` has staves, and `beampipe_hadron_B0.xml` passes `B0Win_thickness/2` (the 8→4 mm window fix). So the PR #1194 geometry is installed.
- Pitfall when **piping** commands into eic-shell: it passes `--norc --noprofile` as arguments, so `thisepic.sh` (which takes its first argument as the detector config) set `DETECTOR_CONFIG=--norc`. Fix: run `set --` before sourcing. Then `DETECTOR_CONFIG=epic` → `epic.xml`. Interactive eic-shell has no arguments, so this does not apply there. Piped mode also prints a harmless "Illegal option --".
- Not yet checked: which detector config the original lambda_b0 files used. The rerun should use the same config, to compare like with like.

### Rerun lambda_b0 with epic main (PR #1194 geometry)
- Original commands (from the user): `source /opt/detector/epic-main/bin/thisepic.sh` (= container epic `43f427e`), geometry `epic_craterlake_18x275.xml`, ddsim Λ gun with 100 events, p 50–100 GeV, θ 8–18 mrad uniform, `--crossingAngleBoost -0.025`, seed 42, `Geant4TVUserParticleHandler`; then `eicrecon` with defaults.
- Container EICrecon: `a7bab3f` (main, 30 Oct 2026).
- **B0 hit threshold is still 10 keV** in EICrecon main (`src/detectors/B0TRK/B0TRK.cc`, `B0TrackerRawHits` `.threshold = 10.0 * dd4hep::keV`). The 5 keV change for the thinner PR #1194 sensors is not merged; the last B0TRK commit is 2026-09-02.
- **Relevant open EICrecon PRs:** #3028 "B0TripletSeeding: dedicated hit seeding for the B0 tracker" (realistic B0 seeding, would remove the truth-seed ~20 cm limitation) and #2926 "Add ChargedLambdaReconstruction: far-forward Lambda -> p pi- candidates".
- Started the rerun in a new folder `~/private/eic/lambda_b0_main` (originals untouched): `run_sim.sh` = same ddsim command with the local epic build. Log in `sim.log`.
- **Simulation OK** (~85 s for 100 events); `sim.log` confirms it used `~/private/eic/epic/install/share/epic/epic_craterlake_18x275.xml`.
- **Old vs new (edm4hep):** event 0 has the same generated Λ, but from event 1 on they differ (the new geometry changes how random numbers are used during simulation). So compare statistically, not event by event.
  - B0 layers: old z ≈ 5896, 6166, 6435, 6706 mm; new ≈ 5844, 6266, 6595, 6873 mm (grouped with ≤3 mm gaps; the two faces per station are not resolved yet).
  - Events with B0 hits: 77 → 64. B0 hits: 1478 → 651. Consistent with the PR's smaller active area and open electron side.
- Started reconstruction: `run_reco.sh` (same eicrecon command, local geometry, default 10 keV B0 threshold). Log in `reco.log`.
- **Reconstruction OK** (`RECO_OK`). Errors in `reco.log` such as "No beam protons found" / "No beam electrons found" (ForwardOffM, LOWQ2) are expected for a particle gun with no beam particles.
- **New geometry, old EICrecon:** `B0TrackerRecHits` 593 hits in 64 events (old: 1416 in 76). `TrackerTruthSeeds`: 4 seeds in 2 events. B0 tracks: **1** (old: 4). The seeding cut still limits it.

### EICrecon PR #2926 (ChargedLambdaReconstruction), https://github.com/eic/EICrecon/pull/2926
- Open, by DraTeots, opened 2026-08-30; changes requested (veprbl, wdconinc wants existing secondary vertex finders used); merge conditions listed by mdiefent on 2026-10-03.
- Pairs positive with negative tracking particles under the p/π⁻ mass hypothesis, with cuts on opening angle, pair polar angle, DCA of two straight lines (+ z of the midpoint as decay-vertex proxy), momentum asymmetry, and mass window ±25 MeV. Keeps all passing pairs.
- Inputs: `ReconstructedChargedParticles` (+ RP, OffM) → `ReconstructedChargedLambdas`; `ReconstructedTruthSeededChargedParticles` (+ RP, OffM) → `ReconstructedTruthSeededChargedLambdas`.
- Their validation (9x130, 2000 events): stock truth seeding → 7 Λ; **truth-seed vertex acceptance opened to 30 m → 118 Λ (110 truth-matched)**, peak μ = 1117 MeV, σ = 8.6 MeV. Real seeding → 0 (no B0 real seeding yet).

### Seeding cut confirmed, and already fixed in EICrecon main
- `TrackParamTruthInitConfig.h` defaults: `maxVertexX = 80 mm`, `maxVertexY = 80 mm`, **`maxVertexZ = 200 mm`** (= the ~20 cm cut inferred from data), `minMomentum = 100 MeV`, `useVertexAsPerigee = false`.
- **EICrecon PR #2901** (merged 2026-10-31 14:25 UTC as `3909334`), "TrackParamTruthInit: implement relaxed mode to include secondaries in B0TrackerTruthSeeds": adds a dedicated `B0TrackerTruthSeeds` with `maxVertexX/Y = 240 mm`, `maxVertexZ = 6000 mm`, `maxEtaBackward = 0`, `useVertexAsPerigee = true` (seed anchored at the decay vertex).
- The container's EICrecon `a7bab3f` is from 2026-10-30 22:57 UTC, i.e. **before #2901**. Same situation as epic #1194: fixed in main, not yet in the nightly container. Likely the "tracking fix" the user remembered (#1194 is the geometry).

### Step 3: old vs new geometry: hits, seeds, tracks
Macro `~/private/eic/compare_tracks.C` (run in `~/private/eic`). Both use the container EICrecon `a7bab3f` (old truth seeder, `maxVertexZ` = 200 mm, B0 threshold 10 keV). Old = epic `43f427e`, new = epic `ec774d8` (PR #1194). 100 events each.

| | old: events / total | new: events / total |
|---|---|---|
| B0 sim hits | 77 / 1478 | 64 / 651 |
| B0 rec hits | 76 / 1416 | 64 / 593 |
| truth seeds | 4 / 8 | 2 / 4 |
| B0 CKF tracks | 3 / 4 | 1 / 1 |
| B0 charged particles (+ / −) | 3 / 4 (3 + / 1 −) | 1 / 1 (1 + / 0 −) |
| central CKF tracks | 1 / 1 | 0 / 0 |
| events with both + and − | 1 | 0 |
| Λ ending at z < 200 mm (seedable) | 5 | 3 |
| Λ ending at z < 6 m | 70 | 71 |

- Hits: the new geometry has ~56% fewer B0 sim hits and 16% fewer events with hits (smaller active area, open electron side; fewer secondaries likely).
- Tracks: 4 → 1. **Not a meaningful comparison:** the number of seeds is set by how many Λs end within 200 mm (5 vs 3, a statistical fluctuation, since the events differ after event 0). Track counts of 1–4 carry no information about PR #1194's tracking performance.
- For a real comparison of tracks: relaxed B0 truth seeding (EICrecon #2901) and/or many more events.

### Building EICrecon main locally (~/private/eic/EICrecon)
- Shallow clone of `eic/EICrecon` main: HEAD `8b44cae` (2026-11-01 23:42 UTC), 14 MB. Building inside eic-shell: `cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install -DCMAKE_BUILD_TYPE=Release`, `cmake --build build -j8`, `cmake --install build`. Log in `~/private/eic/EICrecon/build.log`. AFS at 56% before the build.

### Container EICrecon (a7bab3f) vs main (8b44cae): only 3 commits
1. `3909334` **#2901** "TrackParamTruthInit: implement relaxed mode to include secondaries in B0TrackerTruthSeeds", the change that matters:
   - Before: one truth seeder `TrackerTruthSeeds` (vertex |x|,|y| < 80 mm, |z| < 200 mm), split by θ into central and B0 seeds.
   - After: two seeders. `CentralTrackerTruthSeeds` keeps the old cuts. New `B0TrackerTruthSeeds` uses |x|,|y| < 240 mm, z < 6000 mm, forward only, and `UseVertexAsPerigee = true`.
   - `UseVertexAsPerigee`: the seed reference point is the true production vertex, instead of a back-extrapolation to the beam axis near z = 0. For daughters produced metres downstream (with the 25 mrad crossing angle), the old back-extrapolation put the seed far away and made the CKF propagate many metres, which "degrades or eliminates CKF efficiency".
   - `CKFTracking.cc`: the CKF starting surface is now the seed's perigee point instead of always (0,0,0).
   - Known limitation (from the PR): the final track parameters are still extrapolated to a perigee at (0,0,0), which "will not always work for the secondaries".
   - **Breaking change in the output:** `TrackerTruthSeeds` / `TrackerTruthSeedParameters` are no longer written; replaced by `CentralTrackerTruthSeeds(Parameters)` and `B0TrackerTruthSeeds(Parameters)`. `compare_tracks.C` must use the new names for main-reconstructed files.
2. `f509210` #3075: CI only (builds against older stable releases).
3. `8b44cae` #3081: CALOROC digitization (keep first TOA in a 25 ns bin), calorimeter timing only, not relevant here.
- Not in main: the 5 keV B0 threshold, B0 realistic seeding (#3028, open), ChargedLambdaReconstruction (#2926, open).
- **EICrecon build OK:** `build/` 142 MB, `install/` 48 MB (smaller than the 1–2 GB I had guessed). AFS at 58%. Use it inside eic-shell with `source ~/private/eic/EICrecon/install/bin/eicrecon-this.sh` (after `set --` when piping). `eicrecon --version` prints `0.0.0-unknown` because a shallow clone has no tags.

### Step 4: reconstruct both samples with EICrecon main
- Script `~/private/eic/run_reco_eicreconmain.sh` (pipe into eic-shell with `DIR=... GEO=container|local` as the first line). Output `lambda_b0.eicreconmain.edm4eic.root` in each sample folder; the original reco files are untouched. Logs: `reco_eicreconmain.log`.
- Pitfalls hit: (1) `which` inside the container is a small script that fails on the arguments eic-shell passes ("Illegal option --"), which with `set -e` aborted the script; use `command -v`. (2) My first rerun raced with the fix (two runs started in parallel, one read the script before it was edited). (3) My edit of `compare_tracks.C` through nested quoting put a real newline into a `printf` string; ROOT then got stuck at 100% CPU in error recovery instead of failing. I rewrote the macro cleanly. Lesson: write macros from a local heredoc, not through nested ssh/python quoting, and run ROOT under `timeout`.

| | old geom.: container reco → main reco | new geom.: container reco → main reco |
|---|---|---|
| B0 rec hits (events / total) | 76 / 1416 → 76 / 1416 | 64 / 593 → 64 / 593 |
| truth seeds | 4 / 8 → **61 / 650** | 2 / 4 → **59 / 504** |
| B0 tracks | 3 / 4 → **45 / 77** | 1 / 1 → **30 / 44** |
| positive / negative tracks | 3 / 1 → 48 / 29 | 1 / 0 → 27 / 17 |
| events with both + and − | 1 → **18** | 0 → **14** |

- **EICrecon #2901 is the big effect:** about 20× more B0 tracks in the old geometry and 44× in the new. Hits are unchanged (same simulation, same digitization), so all of the gain comes from seeding.
- Seeds per event are many (~10): the relaxed B0 seeder takes every charged MC particle above 100 MeV with a vertex in its window, including shower secondaries, not just p and π⁻. Tracks are therefore not all Λ daughters; truth matching is needed.
- The old geometry still gives more tracks than the new (77 vs 44; 18 vs 14 events with + and −). That fits the PR's lower acceptance, but this is 100 events, different Λs after event 0, and the 10 keV threshold (5 keV intended for the thinner sensors) is not applied. Not a firm conclusion.

### Q: which EICrecon did we use, and is PR #2926 in it?
- Container: EICrecon `a7bab3f`. Local build: plain main `8b44cae`. The only relevant difference is **#2901** (B0 truth seeding, see above).
- **#2926 (ChargedLambdaReconstruction) was NOT used:** it is still open, so it is not in main. The Λ mass below is computed by hand; #2926 could serve as a cross-check later.

### Step 5: truth-matched B0 tracks → M(p π⁻) from reconstructed momenta
- Macro `~/private/eic/lambda_mass_reco.C` (args: input file, output basename). Reads `ReconstructedTruthSeededChargedParticles` (reco momentum, charge) and `ReconstructedTruthSeededChargedParticleAssociations` (`_rec.index` → reco particle, `_sim.index` → MCParticles, `weight` = fraction of shared hits; the highest weight is used).
- A track counts as a **Λ daughter** if its MC particle's first parent is the generated Λ (generatorStatus 1) **and that Λ has exactly 2 daughters**. Without the second condition, Λs that interacted in material near z ≈ 5.8 m (events 38 and 49, old geometry) had their collision products (π±, K⁺) tagged as "π⁻ from Λ". Fixed.
- Mass: p and π⁻ PDG masses with reconstructed 3-momenta. Also histogrammed: all (+,−) pairs without truth (+ = p, − = π⁻ hypothesis). Output: `<sample>/mass_reco.png`, full per-track list in `<sample>/mass_reco.txt`.

| (EICrecon main reco) | old geometry (`lambda_b0`) | new geometry (`lambda_b0_main`) |
|---|---|---|
| reco tracks | 77 | 44 |
| matched p from Λ / π⁻ from Λ / other | 33 / 16 / 28 | 27 / 16 / 1 |
| events with matched p + π⁻ | 12 | 14 |
| M(p π⁻) mean / RMS | 1.1161 / 0.0050 GeV | 1.1189 / 0.0136 GeV (1.1155 / 0.0059 without event 96) |
| Δp/p of matched daughters, mean / RMS | 0.000 / 0.043 | 0.002 / 0.026 |
| all (+,−) pairs | 41 (12 signal + tail up to 1.27 GeV) | 14 (= the signal pairs) |

- **The Λ peak is there** at the PDG value 1.1157 GeV, about 5–6 MeV wide, with 12–14 Λs out of 100 generated.
- Outliers: event 96 (new geometry), π⁻ measured 12.4 GeV vs true 7.3 GeV → M = 1.164 GeV. Event 62 (old geometry): proton 2% low → M = 1.103 GeV.
- **"Other" tracks: 28 in the old geometry vs 1 in the new.** These are shower secondaries (e±, π±, p) starting at z ≈ 5.80–5.97 m, i.e. in the material in front of and inside the B0 tracker. The new geometry (PR #1194, thinner exit window, 8 → 4 mm) makes far fewer, so the untruthed (+,−) pairs in the new geometry are already clean. Likely explanation, not checked in detail.
- Momentum resolution is better in the new geometry (2.6% vs 4.3% RMS), which fits less material. Low statistics.
- Caveat: reco momenta are taken at the track parameters as written; #2901 says these are extrapolated to a perigee at (0,0,0), which may bias secondaries. The good mass peak suggests the effect is small here.

### Q: what does "truth seeds (events / total)" mean?
- **Seed:** a starting guess for a track (position, direction, momentum, charge) that the track finder (ACTS CKF) uses to look for hits layer by layer. The fit that comes out of the CKF is the track; the seed is only where the search starts.
- **Truth seed:** a seed made from the true MC particle (true vertex, momentum and charge from `MCParticles`), not from the hits. EICrecon makes one for each MC particle that passes the seeder's cuts. In main, `B0TrackerTruthSeeds` keeps charged particles with p > 100 MeV, going forward, with a vertex inside |x|,|y| < 240 mm, z < 6000 mm. The realistic alternative builds seeds from hit triplets (#3028, open).
- **events / total:** events = how many of the 100 events have at least one seed; total = number of seeds summed over all events. Example: 61 / 650 = 61 events have seeds, 650 seeds in all (about 11 per seeded event).
- Seeds ≠ tracks: a seed becomes a track only if the CKF finds enough hits along it. 650 seeds → 77 tracks, because most seeded particles are soft shower particles, or never cross enough B0 layers.

### Step 6: geometrical B0 acceptance, θ = 8–18 mrad, 25 mrad crossing angle
- Question was asked as "2.5 mrad"; the sample uses `--crossingAngleBoost -0.025` = **25 mrad**, so 25 mrad was used.
- Method: ddsim **geantino** gun (straight line, no field, no interactions = pure geometry), same angles, `uniform` distribution and crossing-angle boost as the Λ sample, 100 GeV, 5000 events per geometry. Default ddsim tracker filter `edep1kev` drops geantino hits: needs `--filter.tracker ""` (`None` is rejected: "Filter(s) 'None' are not registered", and then no hit collections are written at all).
- Folder `~/private/eic/acceptance/`: `run_acc.sh` (pipe into eic-shell with `GEO=container|local N=5000 PART=geantino` first), `b0_acceptance.C`, plots `acc_container.png` / `acc_local.png`. Files are small (~4 MB per 5000 events).
- Layers = hit z positions grouped (gap > 50 mm starts a new layer). Angles are w.r.t. the proton beam axis (crossing rotation undone). `uniform` in ddsim is flat in θ here (ratio of events 13–18 / 8–13 mrad = 1.02).

| | old geometry (container epic 43f427e) | new geometry (epic main ec774d8, PR #1194) |
|---|---|---|
| ≥ 1 layer | 95.8 % | 83.5 % |
| **≥ 3 layers** (enough for a track) | **95.3 %** | **70.1 %** |
| all 4 layers | 93.2 % | 66.9 % |
(statistical error 0.3–0.7 %)

- Old geometry: 100 % up to θ = 15 mrad. Losses only at θ > 15 mrad on the side towards the electron beam (θx > +15 mrad, φ ≈ 0).
- New geometry: a **cut-out of about ±45° in φ around φ = 0** (θx > ~+8 mrad), i.e. on the side of the electron beam pipe, which passes through the B0 region. Almost nothing is accepted there. Elsewhere ≈ 85–100 %, with extra losses at θ > 16 mrad.
- This is the acceptance for straight lines from the IP. Λ daughters start downstream and the π⁻ is bent by the B0 dipole field, so the Λ → p π⁻ acceptance is lower.
- Kept macros: `b0_acceptance.C` and `run_acc.sh` in `~/private/eic/acceptance/` (header now explains how to make the input). Local copies in `~/code/epic/macros/` (also `lambda_mass_reco.C`). Re-ran after the edit: same numbers.

### Q: is the geantino number the acceptance of the Λ?
- No. It is the acceptance for **straight lines from the IP at the Λ's generated angles**. The Λ itself is neutral and leaves no hits. Its daughters start at the decay point (often metres downstream), at other angles than the Λ, and are bent by the B0 dipole (the π⁻ much more). So the daughter acceptance needs the full simulation.

### Step 7: acceptance for the Λ → p π⁻ daughters (full simulation)
- Macro `~/private/eic/lambda_daughter_acceptance.C` (on edm4hep files). Λ classification as in `lambda_mass_truth.C` (p π⁻ = exactly 2 daughters). A daughter is accepted if its own `B0TrackerHits` cross ≥ 3 B0 layers (same z grouping as `b0_acceptance.C`). Also saves acceptance vs decay z.
- First results, existing 100-event samples:

| | old geometry (`lambda_b0`) | new geometry (`lambda_b0_main`) |
|---|---|---|
| Λ → p π⁻ / n π⁰ / other | 45 / 27 / 28 | 48 / 30 / 22 |
| p π⁻ decays before z = 5.8 m | 41 of 45 | 42 of 48 |
| per p π⁻ decay, ≥ 3 layers: proton / π⁻ / **both** | 93 % / 80 % / **78 %** | 62 % / 46 % / **46 %** |
| per p π⁻ decay, ≥ 1 layer: proton / π⁻ / both | 93 % / 84 % / 82 % | 79 % / 65 % / 62 % |
| both accepted, per generated Λ | 35 % | 22 % |
| for comparison: reconstructed truth-matched p + π⁻ pairs (step 5) | 12 | 14 |

- "Other" (22–28 %) = Λ that interacted in material or did not decay inside the world volume; not yet split.
- Statistical uncertainty ≈ ±7 % on the per-decay numbers. Larger sample (2 × 500 events per geometry, seeds 101/102) running on lxplus9116, output in `/tmp/gninio_lambda_acc` (EOS is not mounted inside eic-shell), to be moved to `/eos/user/g/gninio/eic/lambda_acc`.
- Notable: in the old geometry 35 events have both daughters in acceptance but only 12 were reconstructed as a pair, so tracking efficiency (not acceptance) is the main loss there. In the new geometry 22 accepted vs 14 reconstructed.
- **Reorganized:** everything for the daughter acceptance is now in `~/private/eic/lambda_acc/`: `lambda_daughter_acceptance.C`, `run_lambda_acc.sh`, plots `daughter_acc_{old,new}[_100ev].png`. Local copies mirror the layout: `~/code/epic/macros/acceptance/`, `~/code/epic/macros/lambda_acc/`.
- Macro now takes a file pattern (TChain, wildcards ok) and draws the same plots as `b0_acceptance.C`: acceptance vs Λ θ and φ (proton / π⁻ / both), 2D map vs Λ θx, θy (both; zero-acceptance cells drawn in the lowest colour, empty cells white), and vs Λ decay z.
- Larger samples: 500 events × 2 seeds per geometry, ~6 min each on lxplus9116 even under load. Copied to `/eos/user/g/gninio/eic/lambda_acc/` (~0.5 GB each). Reading `/eos/...` paths in ROOT prints "TNetXNGFile::Open ... [3001] Required argument not present" for the wildcard, but all files are read (1000 events counted).
- Pitfall: the 4 jobs started together in one folder, and each downloads the `calibrations/` files there at start-up. One job read a half-written file ("cannot get file size ... calibrations/bic_edepToNpe_layer.lut") and stopped. Rerun alone; failed log kept as `sim_container_s102.failed_race.log`. Next time: start one job first, or use one folder per job.

**New geometry, 1000 events** (`daughter_acc_new.png`):
- Λ → p π⁻ 485 (48 %), n π⁰ 278 (28 %), other 237 (24 %); 90 % of the p π⁻ decays happen before z = 5.8 m.
- Per p π⁻ decay, ≥ 3 layers: proton **61 %**, π⁻ **49 %**, both **41 %** (±2 %). ≥ 1 layer: 77 / 67 / 60 %. Per generated Λ: 19.9 %.
- vs φ: the same gap around φ = 0 (±45°) as the geantino map, where acceptance is ≈ 0. Elsewhere both ≈ 55–70 %.
- vs decay z: acceptance rises from ≈ 20 % (decay near the IP) to ≈ 60 % (decay at 4–5 m). Early decays give daughters more time to spread away from the Λ direction, and the π⁻ is bent more before reaching B0.
- vs θ: flat ≈ 45–50 % up to 13 mrad, dropping to ≈ 20 % at 17–18 mrad.

**Old geometry, 1000 events** (`daughter_acc_old.png`; container s102 rerun OK, all 4 samples on EOS):
- Λ → p π⁻ 503 (50 %), n π⁰ 250 (25 %), other 247 (25 %); 88 % of p π⁻ decays before z = 5.8 m.
- Per p π⁻ decay, ≥ 3 layers: proton **80 %**, π⁻ **69 %**, both **63 %** (±2 %). ≥ 1 layer: 85 / 80 / 75 %. Per generated Λ: 31.8 %.
- vs φ: only a dip around φ = 0 (both ≈ 30–40 %), not a hole as in the new geometry.
- vs decay z: ≈ 55–60 % for decays near the IP, ≈ 70–85 % at 1–6 m, then a drop for decays beyond 6 m (inside the tracker, too few layers left).

| per p π⁻ decay, ≥ 3 layers | old geometry | new geometry |
|---|---|---|
| proton | 80 % | 61 % |
| π⁻ | 69 % | 49 % |
| both | **63 %** | **41 %** |
| both, per generated Λ | 31.8 % | 19.9 % |
| geantino at Λ angles (step 6) | 95 % | 70 % |
- Side-by-side image (old | new): `~/private/eic/lambda_acc/daughter_acc_old_vs_new.png`, local copy `~/code/epic/notes/daughter_acc_old_vs_new.png` (made on the Mac with Python PIL from the two pngs).

### Step 8: acceptance × tracking with the 1000-event samples, and why the π⁻ efficiency is low
- Reconstructed all 4 samples with EICrecon main: `~/private/eic/lambda_acc/run_reco_lambda_acc.sh` (own work folder per job; optional `EXTRA` options and `TAG` suffix). ~5 min per 500 events. Outputs `lambda_<geo>_s10X.eicreconmain.edm4eic.root` copied to `/eos/user/g/gninio/eic/lambda_acc/`.
- `lambda_daughter_acceptance.C` extended: on EICrecon files it also counts daughters with a truth-matched reconstructed particle and writes `<outbase>_reco.png` (same 4 panels). Checked on the 100-event file: reproduces 33 p / 16 π⁻ / 12 pairs from step 5.

| per p π⁻ decay | old geometry | new geometry |
|---|---|---|
| accepted (≥ 3 layers): p / π⁻ / both | 80 / 69 / 63 % | 61 / 49 / 41 % |
| reconstructed: p / π⁻ / both | 66 / 39 / **30 %** | 53 / 29 / **21 %** |
| tracking eff. for accepted: p / π⁻ / both | 82 / 56 / 47 % | 86 / 58 / 50 % |
| both reconstructed, per generated Λ | 15.0 % | 10.1 % |

- New macro `daughter_stage_efficiency.C`: for accepted daughters, step by step: seed → CKF track (unfiltered) → filtered track → reco particle. Seeds have no MC link; matched by charge sign + direction (seed momentum is smeared by 10 %, `momentumSmear = 0.1` in `TrackParamTruthInit`).
- Result (1000 events): seeds exist for 96–99 % of accepted π⁻; **the loss is entirely in the CKF step** (56–58 % get a track); filtering and particle building lose nothing. Strongly momentum-dependent: accepted π⁻ below 10 GeV are reconstructed 12–36 % of the time, above 15 GeV 75–90 %. 3-layer π⁻ are worse than 4-layer ones.
- Hypothesis tested and **rejected**: seed momentum smearing. Rerun of `local_s101` with `-Ptracking:B0TrackerTruthSeeds:MomentumSmear=0` (verified: seed q/p = true q/p exactly) gives 71 / 120 vs 70 / 120.
  - Parameter key needs the plugin prefix: `tracking:B0TrackerTruthSeeds:...`. Without it eicrecon stops at start-up ("appears to be unused") and writes an empty file (kept as `*.failed_wrongkey.*`).
- The reco log has no CKF warnings at the default log level (only unrelated "No beam protons/electrons found" errors from far-forward detectors).
- Open: why the CKF fails for soft π⁻. Candidates not yet tested: the CKF χ² / measurement cuts, multiple scattering at low momentum, and the #2901 caveat (final parameters extrapolated to a perigee at (0,0,0), which can fail for strongly bent tracks).
- Side-by-side reco plots (old | new): `~/private/eic/lambda_acc/daughter_acc_reco_old_vs_new.png`, local `~/code/epic/notes/daughter_acc_reco_old_vs_new.png`. Hit-based ("accepted") vs track-based ("reconstructed") plots use the same denominator (all p π⁻ decays), so their ratio is the tracking efficiency.

### Step 9: tracking efficiency only (denominator = accepted daughters)
- `daughter_stage_efficiency.C` extended (args: files, outbase): plots `trk_eff_{old,new}.png` = tracking efficiency for proton / π⁻ vs daughter true momentum, daughter θ w.r.t. proton beam, Λ decay z, layers with reconstructed hits. New step "rec hits ok": the daughter's reconstructed hits (B0TrackerRecHits → raw hit → B0TrackerRawHitAssociations → sim hit → MC particle) still cover ≥ 3 layers. Side by side: `~/code/epic/notes/trk_eff_old_vs_new.png` (also in `lambda_acc/`).
- Pitfall: with two TEfficiency graphs on one pad the x axis is sized to the first one (proton, 30–100 GeV) and the π⁻ points (0–30 GeV) were off the plot. Fixed by setting the axis to the histogram range.

| accepted daughters (1000 events) | old geometry: p / π⁻ | new geometry: p / π⁻ |
|---|---|---|
| keep ≥ 3 layers of reconstructed hits | 100 % / 100 % | **93 % / 93 %** |
| tracking efficiency (reco particle / accepted) | 82 % / 56 % | 86 % / 58 % |
| tracking efficiency given ≥ 3 layers of rec hits | 82 % / 56 % | 92 % / 61 % |

- **Digitization:** in the new geometry 7 % of accepted daughters lose hits below the 10 keV threshold (thinner sensors; the 5 keV threshold intended for them is not in EICrecon main). None in the old geometry.
- **The π⁻ loss is a momentum effect, not a π⁻ effect:** π⁻ efficiency is ≈ 15–35 % below 10 GeV and ≈ 75–90 % at 10–25 GeV, comparable to the proton (≈ 80–90 %, flat in 35–95 GeV). The π⁻ is low on average only because most π⁻ are soft. Points to strongly bent tracks in the B0 dipole (CKF cuts, scattering, or the (0,0,0) perigee extrapolation of #2901); not tested yet.
- Other trends: π⁻ efficiency lower at small (< 8 mrad) and large (> 20 mrad) θ, i.e. near the edges; tracks with 4 layers of rec hits do better than with 3 (old: 84 / 58 % vs 33 / 32 %; new: 95 / 67 % vs 86 / 53 %). No clear dependence on decay z.

### Step 10: B0 threshold 5 keV instead of 10 keV
- Parameter `-PB0TRK:B0TrackerRawHits:threshold=5e-6` (GeV; default 1e-5 = 10 keV). All 4 samples re-reconstructed with `TAG=_5keV`; outputs `*.eicreconmain_5keV.edm4eic.root` on EOS. Raw B0 hits: old geometry 10215 → 10297 (+1 %), new geometry 6002 → 6472 (+8 %), so the threshold was applied.
- Old geometry: no change at all (thick sensors; nothing was lost at 10 keV).
- New geometry:

| new geometry, accepted daughters | 10 keV | 5 keV |
|---|---|---|
| keep ≥ 3 layers of rec hits: p / π⁻ | 93 / 93 % | **100 / 100 %** |
| tracking efficiency: p / π⁻ / both | 86 / 58 / 50 % | **93 / 61 / 59 %** |
| reconstructed per p π⁻ decay: p / π⁻ / both | 53 / 29 / 21 % | 57 / 31 / **25 %** |
| both reconstructed, per generated Λ | 10.1 % | **12.0 %** |

- 5 keV recovers all the digitization losses of the thinner sensors. The π⁻ momentum dependence (≈ 12–35 % below 10 GeV) is unchanged: that loss is in the CKF, not in the hits. Old geometry still better overall (both 15.0 % per Λ) because of the larger acceptance.
- Side by side (new geometry, 10 vs 5 keV): `~/code/epic/notes/trk_eff_new_10keV_vs_5keV.png`.

### Q: what is the 5 / 10 keV threshold?
- EICrecon digitization (`SiliconTrackerDigi`, factory `B0TrackerRawHits` in `src/detectors/B0TRK/B0TRK.cc`): the Geant4 energy deposited in each pixel is summed, and only pixels above the threshold become raw hits (hence reconstructed hits). It models the readout's noise cut. Parameter `B0TRK:B0TrackerRawHits:threshold`, default 10 keV.
- Sensor thickness: old geometry 0.3 mm sensitive layer (`B0_tracker.xml` in the container epic 43f427e); new geometry `B0TrackerSensorThickness` = 0.05 mm (epic main, PR #1194).
- Measured deposits for hadron hits (non-electron secondaries, i.e. mostly Λ daughters; s101 samples): old geometry most probable 95 keV, median 103 keV, 2.6 % below 10 keV; new geometry most probable 13.8 keV, median 14.5 keV, **9.3 % below 10 keV**, 0.3 % below 5 keV. A minimum-ionizing particle loses ≈ 0.28 keV/µm (most probable) in silicon: 50 µm → ≈ 14 keV, matching.
- So 10 keV sits just below the signal peak of the thin sensors and cuts its low tail (Landau fluctuations); 5 keV keeps almost all of it. For the 300 µm sensors either threshold is far below the signal.

### Q: why are the proton and π⁻ momenta so different?
- Λ → p π⁻ releases only 38 MeV: p* = 0.101 GeV in the Λ rest frame, so both daughters move with nearly the Λ velocity and share its momentum roughly as their rest-frame energies: proton E* = 0.943 GeV → (0.943 ± 0.101)/1.1157 = 76–94 % of p(Λ); π⁻ E* = 0.172 GeV → (0.172 ± 0.101)/1.1157 = 6–25 %. For p(Λ) = 50–100 GeV: proton 38–94 GeV, π⁻ 3–25 GeV, as in the plots. The π⁻ is always soft, hence bends more in the B0 dipole.

### Step 11: Λ mass with the 1000-event samples, 10 and 5 keV
- `lambda_mass_reco.C` moved to `~/private/eic/lambda_acc/` (old copy in `lambda_acc/old/`). Now takes a file pattern (TChain), `verbose` flag (per-track list off by default), mass histogram 1.06–1.17 GeV in 2 MeV bins, Gaussian fit of the core (1.101–1.131 GeV, only with ≥ 30 pairs). Check on the 100-event file: same 12 pairs, 1.1161 / 0.0050 GeV.
- Combined plot: `~/code/epic/notes/mass_reco_all.png` (rows: old 10 keV, new 10 keV, new 5 keV).

| | old geometry, 10 keV (5 keV identical) | new geometry, 10 keV | new geometry, 5 keV |
|---|---|---|---|
| truth-matched p π⁻ pairs (of 1000 Λ) | 150 | 101 | **120** |
| Gaussian mean | 1.1177 ± 0.0007 GeV | 1.1160 ± 0.0010 GeV | 1.1159 ± 0.0008 GeV |
| Gaussian σ | 6.3 ± 0.6 MeV | 7.4 ± 1.1 MeV | 6.9 ± 1.1 MeV |
| histogram RMS (1.06–1.17) / pairs above 1.17 | 8.0 MeV / 4 | 9.2 MeV / 6 | 9.2 MeV / 6 |
| Δp/p RMS of matched daughters | 4.4 % | 3.2 % | 3.2 % |
| all (+,−) pairs, no truth | 201 | 116 | 139 |

- PDG mass 1.11568 GeV. New geometry: peak at the PDG value. Old geometry: +2 MeV (≈ 3σ of the fit error); not understood, could be a momentum-scale effect; not checked.
- Width ≈ 6–7 MeV in both; the better Δp/p of the new geometry does not show up as a narrower peak at this statistics. A tail to high mass (pairs above 1.13 GeV) in all samples.
- Without truth, all (+,−) combinations: old geometry 201 pairs for 150 signal (shower tracks), new geometry 139 for 120.

### Step 12: Λ mass without truth matching in the selection
- Macro `~/private/eic/lambda_acc/lambda_mass_notruth.C`: every (+,−) pair of reco particles in an event, + = proton, − = π⁻. One cut: p(+)/p(−) > 2.5 (true pairs always have > ~3, from the decay kinematics). Truth only labels candidates afterwards (signal = + matched to the proton and − to the π⁻ of the same Λ → p π⁻). Caveat: tracks still come from truth seeding. Plots `mass_notruth_*.png`; combined `~/code/epic/notes/mass_notruth_all.png`.

| | old geometry, 10 keV | new geometry, 10 keV | new geometry, 5 keV |
|---|---|---|---|
| all (+,−) pairs / signal / purity | 201 / 150 / 75 % | 116 / 101 / 87 % | 139 / 120 / 86 % |
| after p(+)/p(−) > 2.5 / signal / purity | 155 / 147 / **95 %** | 103 / 99 / **96 %** | 122 / 118 / **97 %** |
| signal kept by the cut | 98 % | 98 % | 98 % |
| ±20 MeV window: candidates / signal | 144 / 142 | 92 / 92 | 110 / 110 |
| Gaussian mean / σ | 1.1174 / 6.6 MeV | 1.1164 / 6.4 MeV | 1.1166 / 6.7 MeV |
| events with a candidate (of 1000) | 155 | 102 | 121 |

- The one momentum-ratio cut removes most background and keeps 98 % of the signal; inside ±20 MeV of the PDG mass the sample is ≈ 99–100 % pure. Remaining background is far from the peak (shower tracks, mostly old geometry).
- This is a single-Λ particle gun with nothing else in the event; in real events (other tracks, beam background) the combinatorics will be much worse and vertexing/decay-length cuts will be needed.

### Q: what is purity, and why p(+)/p(−) > 2.5?
- **Purity** = real candidates / all selected candidates, i.e. the fraction of the selected (+,−) pairs that truly are the p and π⁻ of the same Λ (from the truth labels). Example new geometry 5 keV: 118 / 122 = 97 %. Its partner is the **efficiency** of the cut = real candidates kept / real candidates before the cut (118 / 120 = 98 %).
- **Ratio cut from the decay kinematics:** in the Λ rest frame p* = 0.101 GeV, E*(p) = 0.9435 GeV, E*(π) = 0.1723 GeV, back to back. Boosting to the lab (β ≈ 1, γ ≥ 45 for p(Λ) ≥ 50 GeV), with θ* the proton emission angle w.r.t. the Λ direction: p(p) ≈ γ (E*(p) + p* cos θ*), p(π) ≈ γ (E*(π) − p* cos θ*) (transverse p* sin θ* ≤ 0.1 GeV is negligible). γ cancels in the ratio:
  p(p)/p(π) = (0.9435 + 0.101 cos θ*) / (0.1723 − 0.101 cos θ*) → from 0.8425/0.2733 = **3.08** (cos θ* = −1, proton emitted backwards) to 1.0445/0.0713 = **14.6** (cos θ* = +1).
  So every true pair has a ratio between 3.1 and 14.6, independent of the Λ momentum. The cut at 2.5 sits ~20 % below 3.08 to allow for momentum mismeasurement (Δp/p ≈ 3–4 % per track; occasional bad tracks like the π⁻ at 12.4 vs 7.3 GeV). Background pairs (shower tracks, wrong combinations) have no such constraint and often have p(+) ≲ p(−).

### Step 13: check of the momentum-ratio cut
- Macro `~/private/eic/lambda_acc/lambda_ratio_truth.C`: true p(p)/p(π⁻) for all Λ → p π⁻ decays, the expectation for an isotropic decay in the collinear limit, f(r) = (E*p + E*π) / (2 p* (1 + r)²) on 3.09 < r < 14.61, and the reconstructed ratio of truth-matched pairs. Plots `~/code/epic/notes/ratio_new_5keV.png`, `ratio_old_10keV.png`.
- Result: true ratio min/max 3.09/14.58 (new) and 3.10/14.60 (old), exactly the predicted limits; shape follows f(r) (peaked at 3, falling as 1/(1+r)²); mean 6.76 / 6.35 vs 6.43 expected (≈ 485–503 decays, statistical).
- Reconstructed pairs: 2 of 120 (new, 5 keV) and 3 of 150 (old) at or below 2.5, i.e. the cut loses ~2 % of the signal through momentum mismeasurement.
- The reconstructed pairs sit mostly at low ratio (3–6): a high ratio means a soft π⁻, and soft π⁻ are the ones the CKF loses (step 9). Same effect seen from another side.

## 2026-11-03

### Step 14: repository, reorganization, report and slides
- GitHub repo https://github.com/gadininio/eic-B0-detector (public; was empty). Working copy on lxplus: `~/private/eic/eic-B0-detector` (git). Pushed from lxplus once its key (`~/.ssh/id_rsa.pub`) is added to GitHub.
- **Reorganized `~/private/eic`** (paths in older entries of this log are the old ones):
  - code and results → `eic-B0-detector/` (repo); `notes/` moved into it (this log, the commands, the user's `log.md` and images).
  - ROOT files → EOS `/eos/user/g/gninio/eic/`, renamed with old/new geometry and seed:
    `lambda_100ev/lambda_{old,new}_s42.{edm4hep, containerreco.edm4eic, eicreconmain.edm4eic}.root`,
    `lambda_1000ev/lambda_{old,new}_s10{1,2}.{edm4hep, eicreconmain.edm4eic, eicreconmain_5keV.edm4eic}.root` (was `lambda_acc/`, container→old, local→new),
    `lambda_1000ev/tests/lambda_new_s101.eicreconmain_nosmear.edm4eic.root` (copied from lxplus9116 /tmp), `geantino/geantino_{old,new}_5000.edm4hep.root`; logs in `logs/` of each.
  - AFS copies of the ROOT files removed after `cmp` against the EOS copies (AFS 60 % → 54 %).
  - Old work folders (lambda_b0, lambda_b0_main, acceptance, lambda_acc, compare_tracks.C, run_reco_eicreconmain.sh) moved to `~/private/eic/old_workdirs_2026-11-02/` (69 MB, no ROOT files; can be deleted).
  - Builds untouched: `epic/`, `EICrecon/`, `local/`, `eic-shell` (absolute paths baked in).
- **Code reorganized**, shared code in `macros/common/LambdaB0.h`; separate, documented macros for: geometric acceptance (geantino, `macros/acceptance/b0_geometric_acceptance.C`), daughter acceptance (`macros/acceptance/lambda_daughter_acceptance.C`), tracking efficiency (`macros/efficiency/lambda_tracking_efficiency.C`), acceptance × efficiency (`macros/efficiency/lambda_acceptance_x_efficiency.C`, also prints A, ε and checks A·ε), mass (`macros/mass/`), exploration (`macros/exploration/`).
- Scripts: `scripts/sim/ddsim_gun.sh`, `scripts/reco/eicrecon.sh`, `scripts/in_eic_shell.sh`, `scripts/production/{lambda,geantino}_sample.sh` (one folder per job), `scripts/build/`, `scripts/make_results.sh` (all results). Tested end to end with 5 Λ and 50 geantinos.
- `scripts/make_results.sh` regenerated all of `results/` from the EOS files: every number agrees with the earlier steps (A×ε factorization agrees to < 0.6 %).
- Documents: `docs/report/report.pdf` (14 pages), `docs/slides/slides.pdf` (16 slides, beamer), `notes/claude_summary.md` (Λ/B0 work only), `README.md` (definitions, results, layout, reproduction).
- Pitfall: on a new day the ssh master connection was re-made and landed on another node (lxplus930); files in lxplus9116's /tmp were reached with `ssh lxplus9116.cern.ch` from inside lxplus (Kerberos).
- **Pushed to GitHub** (https://github.com/gadininio/eic-B0-detector, branch `main`, commit 54970c3). lxplus key still refused by GitHub ("Permission denied (publickey)"), so the lxplus repo was cloned to the Mac over ssh and pushed from there (the Mac key is on GitHub).
- **Push from lxplus enabled:** GitHub rejected the 2020 RSA key (2048 bit, "Key is too short"); made a new ed25519 key `~/.ssh/id_ed25519` (no passphrase; ~/.ssh AFS ACL: others only `l`), added on GitHub by the user. `ssh -T git@github.com` greets gadininio; pushes now go directly from lxplus. (A token pasted in the chat was not used and should be revoked.)

## 2026-11-11
- The user moved the Mac work folder `~/code/epic` → `~/code/epic-all/epic` (workspace file `~/code/epic-all/epic.code-workspace`). Logs live here now; Claude memory paths updated and the memory copied to the project folder of the new path.
- Clarified the slide "compare the B0 geometry of the nightly with the realistic B0 of eic/epic#1194": old = epic 43f427e (geometry in the eic-shell nightly, just before #1194; 300 µm sensors), new = epic main ec774d8 (#1194; 50 µm, φ gap); same EICrecon main, same seeds.
- Note: the jettagger repo (tau, `gadininio/jettagger-analysis`) belongs to another project and came up here by accident; it is not part of this work and is left alone.
- Checked EICrecon PR status (2026-11-11): #2901 (B0 truth seeding for secondaries) merged 2026-10-31, in our main build 8b44cae. #3028 (B0 triplet seeding) closed 2026-11-05 in favor of **#3116** (B0 seeding with the Acts triplet seeder, open); issue #2930 open. **#3084 merged 2026-11-03: B0 threshold 10 → 5 keV is now the EICrecon default** (after our build) → README/report/slides/summary say "not yet the default": to update.

### Hit (realistic) seeding of the B0 tracks — EICrecon#3116
- The user pointed to #3029 (merged): it is a bot include-fix merged into the *branch* of #3028, not into main; #3028 was closed (2026-11-05) for **#3116** (open), "seed B0 tracks with the Acts triplet seeder": triplets on 3 different B0 stations, hits rotated so the field at the middle hit is along z, no vertex/charge assumption, min p across the field 1 GeV, ≥3 measurements. Its branch = main + 6 commits (has #2901 and #3084 = 5 keV default).
- Built it: `scripts/build/build_eicrecon.sh EICrecon_pr3116 3116` → `~/private/eic/EICrecon_pr3116` at **1ca147b** (206 MB; AFS 77 → 79 %). Build script now takes `<dir> [PR]`.
- **The eic-shell container changed:** now the nightly of 2026-11-07 (epic-main 7023314, already with the realistic B0; EICrecon 7b22d52; also /opt/detector/epic-26.11.0 = ec774d8 = our "new"). So `GEO=container` is no longer the old geometry. epic tags: 26.11.0 = ec774d8 = 43f427e + #1194 only; 43f427e has no tag. → Built **epic 43f427e** locally: `scripts/build/build_epic.sh epic_43f427e 43f427e736223b963d02adefcda44bfa225d577b` → `~/private/eic/epic_43f427e`. `GEO`/`RECO` in the scripts now also accept `/opt/detector/<name>` or `$EIC_TOP/<name>/install`.
- Hit-seeded B0 tracks go to `B0TrackerSeeds` → `B0TrackerCKFTracks[Unfiltered]` (+ `...TrackAssociations`) → `CombinedTracks` → **`ReconstructedChargedParticles`** (+ `ReconstructedChargedParticleAssociations`). Macros got a `seeding` argument ("truth" default | "hit"), via `lb0::Seeding` in LambdaB0.h; hit seeds are matched to MC through their hits (≥ 2 of 3 made by the daughter). Truth-seeding regression on the 5 keV files: identical numbers (92.9 / 60.8 %, A×ε 24.7 %).
- New `scripts/production/lambda_rereco.sh GEO NAME RECO TAG [EXTRA] [DEST]`: re-reconstruct an existing simulation, copy to EOS. Outputs `lambda_{old,new}_s10{1,2}.eicreconpr3116.edm4eic.root` (old with GEO=epic_43f427e, new with GEO=local).
- Pitfall: `-Pjana:nevents=20` in EXTRA is ignored — the script's earlier `-Pjana:nevents=-1` wins in JANA (first value kept). The "20-event test" of s101 was in fact the full 500 events → kept as the final s101 output (noted at the top of its log).
- First look, new geometry s101 (500 ev), same events: B0 tracks 268 hit-seeded vs 240 truth-seeded (old geometry: 568 vs 416). Accepted daughters p / π⁻: truth seeding seed 98/96 %, track 94/60 %; **hit seeding seed 100/100 %, CKF 100/68 %, reco particle 100/64 %**. Soft π⁻ (5–10 GeV) still lost in the CKF (39 %) although every one has a seed ⇒ the π⁻ loss is not a seeding problem.
- **Results, 1000 events per geometry (`results/07_hit_seeding`, same events for truth and hit seeding, EICrecon#3116 build, 5 keV):**
  - Tracking efficiency p / π⁻: new truth 93.2 / 60.8 %, **new hit 100.0 / 64.6 %**; old truth 97.5 / 66.5 %, **old hit 99.5 / 72.5 %**. Hit seeds exist for 100 % of accepted daughters; the π⁻ loss stays in the CKF (5–10 GeV: 39 % new, 48 % old).
  - A×ε both daughters per p π⁻ decay: new 24.7 % (truth) → **27.0 %** (hit); old 41.7 % → **45.5 %**. Per generated Λ: new 12.0 → 13.1 %, old 21.0 → 22.9 %.
  - Mass (truth-matched, Gaussian): new hit 1.1159 GeV, σ 6.3 MeV (131 pairs); old hit 1.1175 GeV, σ 6.8 MeV (229).
  - No truth, p(+)/p(−) > 2.5: purity new 92.8 % (hit) vs 96.7 % (truth); old 64.1 % vs 85.5 %. Hit seeding also tracks other particles (secondaries): "other" tracks old 432 (hit) vs 198 (truth), new 91 vs 68. In ±20 MeV: new 117/117 signal, old 207/220.
  - **Unexplained:** old-geometry *truth-seeded* efficiency with this build (97.5 / 66.5 %, A×ε 41.7 %) is much higher than with EICrecon main 8b44cae on the same simulation (82.2 / 55.6 %, A×ε 29.8 %), while the new geometry is unchanged (93.2 vs 92.9 %). Differences: EICrecon 8b44cae → 1ca147b (main of 2026-11-11 + #3116), container 2026-11-02 → 11-07 (Acts etc.), epic 43f427e from the container → local build of the same commit. Not yet investigated.
- **Mass, truth vs hit seeding overlaid** (new macro `macros/mass/lambda_mass_seeding_compare.C`, both kinds read from the same file; `results/07_hit_seeding/mass_seeding_compare_{old,new}`, also in `make_results.sh 07`):
  - truth-matched: same peak — old 1.1174 / 1.1175 GeV, σ 6.9 / 6.8 MeV (210 / 229 pairs); new 1.1159 / 1.1159, σ 6.8 / 6.3 MeV (120 / 131).
  - decays reconstructed with both seedings: old 209 (only truth 1, only hit 20), new 114 (6, 17); M(hit) − M(truth) mean ≈ 0, RMS 4.4 MeV (the two track sets differ slightly).
  - dp/p of the daughters RMS: old 3.8 → 4.4 %, new 3.4 → 3.7 % (hit seeding slightly worse).
  - no truth: old all pairs 331 → 877 (purity 63 → 26 %), after the cut 86 → 64 %; new 86 → 83 %, 97 → 93 %. ±20 MeV window: old 200/207 vs 206/219, new 110/110 vs 116/116 (exact |M − m_Λ| < 20 MeV; lambda_mass_notruth.C integrates whole bins, hence ±1 differences).
- The user asked to keep `claude_summary.md` (repo `notes/`) updated along with the log and commands: added section 7 (hit seeding) and updated the open points and the 5 keV default (#3084).
- Q&A: how the tracking efficiency is defined. Denominator = "accepted" daughters (p or π⁻ of a Λ → p π⁻ decay whose own simulated B0TrackerHits cover ≥ 3 layers, before digitization), not a fiducial volume; numerator = those with a reconstructed charged particle whose best truth match is the daughter.
- Q&A: very energetic Λ (decays after the B0 tracker). λ_decay = p[GeV] × 7.07 cm; decay before the first B0 layer (z ≈ 5.8 m): 81 % at 50 GeV, 56 % at 100, 34 % at 200, 26 % at 275. Later decays leave no daughter hits → drop out of the acceptance (and of the efficiency denominator), the Λ itself leaves none. For accepted daughters the efficiency should rise with energy (π⁻ carries ~6–24 % of p_Λ, so fewer soft π⁻, which the CKF loses); decays inside the tracker give < 3 layers or, beyond z = 6 m, no truth seed (hit seeding has no vertex cut). Our sample is 50–100 GeV only; a 100–275 GeV sample would quantify it (offered).

### Very energetic Λs: 100–275 GeV samples
- The "decays before the B0" numbers: decay law P = 1 − exp(−L/λ), λ = (p/m)·cτ = p[GeV] × 7.07 cm, L = 5.8 m (path ≈ z at ~10 mrad). Checked on the 50–100 GeV sample, × BR(p π⁻) = 0.641: 50–75 GeV 47.7 % simulated vs 46.8 % expected, 75–100 GeV 39.5 vs 39.0 %.
- `lambda_sample.sh` takes `PMIN`, `PMAX` and `SIM_ONLY=1` from the environment. Started on lxplus9110: lambda_{old,new}_s20{1,2}, 500 events each, 100–275 GeV, simulation then `lambda_rereco.sh ... EICrecon_pr3116 eicreconpr3116` (truth- and hit-seeded tracks in one file) → `$EIC_DATA/lambda_100to275`.
- New macro `macros/efficiency/lambda_vs_momentum.C`: vs p_Λ (50–275 GeV): p π⁻ decays before z = 5.8 m with the decay-law curve, acceptance, tracking efficiency (truth/hit seeding; also vs the daughter's own momentum), A×ε both per generated Λ; acceptance vs decay z.
- Q: why are 90 % of the p π⁻ decays before the B0 when the decay law gives 56–81 %? Checked the fates in lambda_new_s10? (1000 Λ, 50–100 GeV): p π⁻ 485 (438 at z < 5.8 m, max 8.7 m), n π⁰ 278 (246), other 237 (11; all have daughters, i.e. they interacted, 205 of them ending at z = 6–10 m). A Λ that has not decayed by ~6 m nearly always interacts in material before it can decay, so the p π⁻ decays that exist are mostly early ones: 90 % is per p π⁻ decay. Per generated Λ: 438 + 246 = 684 decays (68 %) before 5.8 m, matching the decay-law average (66 %); p π⁻ before 5.8 m 43.8 % vs 0.641 × 66 % = 42 %. ⇒ "per p π⁻ decay" is a conditional denominator; "per generated Λ" is the unbiased one.
- 2026-11-12: the four 100–275 GeV chains had finished (EOS files 18:45–18:50; RECO_OK in all logs); the background wait failed only because the ssh connection dropped (new node lxplus993). ~1.2–1.5 GB per 500-event file.
- **Results vs Λ momentum** (`results/08_momentum/vs_momentum_{old,new}`, make_results.sh 08):
  - p π⁻ decays at z < 5.8 m per generated Λ follow the decay law: 48 % (50–75 GeV) → 17–18 % (250–275 GeV), e.g. new 47.7/39.5/24.2/28.4/25.3/21.6/21.9/19.2/17.8 % vs expected 46.8/39.0/33.2/28.8/25.4/22.7/20.5/18.7/17.2 %.
  - π⁻ tracking efficiency vs its own momentum: ~40 % below 5 GeV, ~100 % above ~20 GeV; vs p_Λ it rises from 52–62 % (50–75 GeV) to 90–100 % above 150 GeV. Proton: hit seeding 100 %, truth seeding 90–97 % (new).
  - Both daughters reconstructed per generated Λ, truth/hit: old 20.9/22.5 % (50–75) → 24.6/26.1 % (150–175) → 16.3/17.0 % (250–275); new 9.8/10.0 % → 8.9/11.6 % → 8.9/9.6 %. Falling decay probability ≈ compensated by the better reconstruction of the early decays.
  - Acceptance vs decay z: nothing accepted for decays beyond ~6.5 m.
- Q&A: "beam-like energies" = Λ momenta close to the 275 GeV proton beam (leading Λs from proton fragmentation, carrying a large fraction of the beam momentum). Yes: at 275 GeV only 26 % of the Λs decay (any mode) before z = 5.8 m, 17 % to p π⁻; the rest pass the B0 position undecayed (in the simulation most then interact in material at z = 6–10 m). Correction to the earlier remark: the reconstructed fraction per generated Λ does not fall as fast as the decay probability, because the early decays are reconstructed better at high momentum (results/08).
