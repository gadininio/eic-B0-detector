# Summary: Λ → p π⁻ in the ePIC B0 tracker (sessions of 2026-11-02/03 and 2026-11-11)

Only the Λ / B0 work; the full chronological log is `claude_log.md`, the commands
`claude_commands.md`. Macros and scripts referred to below are in this repository
(file names as reorganized on 2026-11-03).

## Goal
Reconstruct the Λ mass from tracks in the far-forward B0 tracker, step by step, with a single-Λ
particle gun (p = 50–100 GeV, θ = 8–18 mrad, 25 mrad crossing angle, 18×275 GeV).

## 1. First look at the 100-event sample (old geometry, eic-shell nightly)
- B0 hits are in `B0TrackerHits`, at four layers z ≈ 5.9–6.7 m. Each hit's MC particle was traced
  up the parent chain (`b0_hit_origins.C`): hits come from the Λ daughters (p, π⁻) directly, from
  products of Λ interactions in material, and from shower particles ("Λ descendants").
- Only **4 B0 tracks in 3 of 100 events**. Cause: the truth seeder (`TrackParamTruthInit`) only
  seeded particles produced at |z| < 200 mm, so daughters of Λs decaying metres downstream had no
  seed. The 4 seeded events were exactly the p π⁻ decays at z < 200 mm.
- eic/epic#1194 (realistic B0 geometry) is a geometry change only and could not explain this; the
  nightly container had epic 43f427e, the commit just before its merge (ec774d8).
- Truth-level check: Λ → p π⁻ 45, n π⁰ 27, other 28 (interactions); M(p π⁻) from true momenta
  = 1.115683 GeV exactly (`lambda_mass_truth.C`).

## 2. New geometry and EICrecon main
- epic main (ec774d8, with #1194) built locally; the same 100 events re-simulated with it: B0 hits
  1478 → 651, layers at 5844/6266/6595/6873 mm.
- EICrecon main (8b44cae) built locally. Relevant difference to the nightly (a7bab3f): **#2901**, a
  dedicated `B0TrackerTruthSeeds` (vertex |x|,|y| < 240 mm, z < 6 m, perigee at the true vertex).
  PR #2926 (ChargedLambdaReconstruction) is still open and was not used.
- B0 tracks with EICrecon main: **4 → 77** (old geometry), **1 → 44** (new geometry); events with
  both a + and a − track: 1 → 18 and 0 → 14 (`compare_tracks.C`). Hits unchanged: all from seeding.

## 3. Truth-matched mass, 100 events
- 12 (old) / 14 (new) events with a reconstructed, truth-matched p + π⁻; M(p π⁻) ≈ 1.116 GeV,
  σ ≈ 5–6 MeV. A Λ daughter must come from a Λ with exactly two daughters (Λs interacting in
  material produce π±, K⁺ with the Λ as parent).

## 4. Acceptance
- **Geometric acceptance** along the Λ direction (geantino gun, straight lines; not a Λ
  acceptance): ≥ 3 layers 95.3 % (old) vs 70.1 % (new). The new geometry has a gap of ±45° in φ on
  the electron-beam side. ddsim needs `--filter.tracker ""` to keep geantino hits.
- **Daughter acceptance** (simulated hits of the daughter in ≥ 3 layers, per p π⁻ decay; 1000
  events): proton / π⁻ / both = 80 / 69 / **63 %** (old), 61 / 49 / **41 %** (new). Lower than the
  geometric number: decay point, opening angle, and the B0 dipole bending the π⁻.
- Kinematics: the proton carries 76–94 % of the Λ momentum (38–94 GeV), the π⁻ 6–25 % (3–25 GeV).

## 5. Tracking efficiency and acceptance × efficiency (1000 events)
- Tracking efficiency for accepted daughters, p / π⁻ / both: 82 / 56 / 47 % (old), 86 / 58 / 50 %
  (new, 10 keV), 93 / 61 / 59 % (new, 5 keV).
- Step by step (`lambda_tracking_efficiency.C`): seeds exist for 96–99 % of accepted daughters;
  the loss is in the CKF; track filtering and particle building lose nothing.
- π⁻ efficiency depends on momentum: 12–36 % below 10 GeV, 75–90 % at 10–25 GeV (as good as the
  proton). Soft tracks, not pions, are the problem. Seed momentum smearing (10 %) ruled out:
  without it 71 / 120 vs 70 / 120 accepted π⁻ reconstructed. Cause not identified (candidates: CKF
  χ² cuts, multiple scattering, (0,0,0) perigee extrapolation of #2901).
- **B0 threshold:** the new 50 µm sensors give ≈ 14 keV per hit (old 300 µm: ≈ 95 keV); 10 keV
  cuts 9 % of the hits and 7 % of the accepted daughters; **5 keV recovers them**
  (`-PB0TRK:B0TrackerRawHits:threshold=5e-6`). No effect in the old geometry. Since
  EICrecon#3084 (merged 2026-11-03, after our main build) 5 keV is the default.
- **Acceptance × efficiency**, both daughters, per p π⁻ decay: 30 % (old), 21 % (new, 10 keV),
  25 % (new, 5 keV); per generated Λ: 15.0 / 10.1 / 12.0 %.

## 6. Λ mass, 1000 events
- Truth-matched: 150 (old) / 101 (new, 10 keV) / 120 (new, 5 keV) pairs; Gaussian mean / σ:
  1.1177 GeV / 6.3 MeV, 1.1160 / 7.4, 1.1159 / 6.9. The old-geometry peak is ~2 MeV high (≈ 3σ of
  the fit error), not understood.
- **Without truth in the selection:** all (+,−) pairs, + = proton, cut p(+)/p(−) > 2.5. Two-body
  kinematics give 3.09 < p(p)/p(π⁻) < 14.6 for any Λ momentum (checked with true momenta:
  3.09–14.58). Purity 95 / 96 / 97 %, 98 % of the signal kept; within ±20 MeV of the PDG mass
  ≈ 100 % pure (single-Λ events; tracks still truth-seeded).

## 7. Hit (realistic) seeding — EICrecon#3116 (2026-11-11)
- Realistic B0 seeding is not in EICrecon main: #3028 (and the bot fix #3029, merged into the
  #3028 branch only) was closed for **#3116** (open): triplets of hits on 3 B0 stations with the
  Acts seeders, hits rotated so the field at the middle hit is along z; no vertex or charge
  assumption. Built from its branch (1ca147b = main of 2026-11-11 + #3116; includes #2901 and
  **#3084, merged 2026-11-03: B0 threshold 5 keV is now the EICrecon default**).
- The eic-shell container moved to the nightly of 2026-11-07, which already has the realistic B0;
  the old geometry is now a local build of epic 43f427e (epic tag 26.11.0 = ec774d8 = new).
- The same 1000 events per geometry re-reconstructed (`lambda_rereco.sh`); truth- and hit-seeded
  tracks come from the same files (`ReconstructedTruthSeededChargedParticles` vs
  `ReconstructedChargedParticles`); macros take `seeding = "truth" | "hit"`; hit seeds are matched
  to MC through their hits (≥ 2 of 3).

| (1000 Λ) | old, truth | old, hit | new, truth | new, hit |
|---|---|---|---|---|
| seed for accepted p / π⁻ | 99 / 99 % | 100 / 100 % | 97 / 96 % | 100 / 100 % |
| tracking efficiency p / π⁻ | 97.5 / 66.5 % | 99.5 / 72.5 % | 93.2 / 60.8 % | 100 / 64.6 % |
| A×ε both, per p π⁻ decay | 41.7 % | 45.5 % | 24.7 % | 27.0 % |
| both reconstructed, per generated Λ | 21.0 % | 22.9 % | 12.0 % | 13.1 % |
| M truth-matched: pairs, mean, σ | 210, 1.1174 GeV, 6.9 MeV | 229, 1.1175, 6.8 | 120, 1.1159, 6.8 | 131, 1.1159, 6.3 |
| dp/p of the daughters (RMS) | 3.8 % | 4.4 % | 3.4 % | 3.7 % |
| no truth: all (+,−) pairs, purity | 331, 63 % | 877, 26 % | 139, 86 % | 158, 83 % |
| after p(+)/p(−) > 2.5, purity | 242, 86 % | 348, 64 % | 122, 97 % | 138, 93 % |
| ±20 MeV window: signal / candidates | 200 / 207 | 206 / 219 | 110 / 110 | 116 / 116 |

- Hit seeding finds a seed for every accepted daughter and reconstructs every proton; the π⁻ are
  still lost in the CKF (5–10 GeV: 39 % new, 48 % old) ⇒ **the soft-π⁻ loss is not a seeding
  problem**.
- The mass peak is the same with both seedings (`lambda_mass_seeding_compare.C`): of the decays
  reconstructed with both, M(hit) − M(truth) has mean 0.0 and RMS 4.4 MeV; hit seeding adds 17–20
  decays and loses 1–6. Momentum resolution slightly worse with hit seeding (3.7 vs 3.4 % new).
- Hit seeding also tracks other particles (secondaries of interactions in material): in the old,
  thick geometry 432 "other" tracks vs 198, so a large combinatorial background without truth
  (purity after the cut 64 %); in the new, thin geometry little (93 %). The ±20 MeV window stays
  ≥ 94 % pure.
- **Unexplained:** with this build the *truth-seeded* old-geometry efficiency is much higher than
  with EICrecon main 8b44cae on the same simulation (proton 97.5 vs 82.2 %, A×ε both 41.7 vs
  29.8 %); new geometry unchanged (93.2 vs 92.9 %). Candidates: EICrecon changes 8b44cae → 1ca147b,
  the newer container (Acts), or the epic build (same commit 43f427e, container vs local).

## 8. Very energetic Λs: 50–275 GeV (2026-11-12)
- New samples 100–275 GeV, 2 × 500 Λ per geometry (`$EIC_DATA/lambda_100to275`, `PMIN/PMAX/SIM_ONLY`
  in `lambda_sample.sh`), reconstructed with #3116; `macros/efficiency/lambda_vs_momentum.C`
  (results/08_momentum) on 50–100 + 100–275 GeV.
- **Decay before the B0** follows BR·(1 − e^{−L/λ}), λ = p/m·cτ = p[GeV]·7.07 cm, L = 5.8 m:
  p π⁻ decays at z < 5.8 m per generated Λ fall from 48 % (50–75 GeV) to 17–18 % (250–275 GeV).
  A Λ not decayed by ~6 m nearly always interacts in material (fate "other", ending at z = 6–10 m),
  so "per p π⁻ decay" is a conditional denominator (90 % of p π⁻ decays are before 5.8 m at
  50–100 GeV) and "per generated Λ" is the unbiased one.
- For decays that do happen before the B0, things **improve** with momentum: the π⁻ is harder
  (6–24 % of p_Λ), so the soft-π⁻ CKF loss disappears: π⁻ tracking efficiency vs its own
  momentum rises from ~40 % below 5 GeV to ~100 % above ~20 GeV (both seedings). Hit seeding:
  protons 100 % at all momenta; truth seeding loses 5–10 % of the protons (new geometry).
- Net, both daughters reconstructed per generated Λ (truth / hit seeding): old 21 / 23 % at
  50–75 GeV, ~25 % at 125–175, 13–17 % above 175 GeV; new 10 / 10 % at 50–75, 9–14 % up to 275 GeV.
  The falling decay probability is largely compensated by the rising acceptance × efficiency of
  the early decays (new: 21 % of early p π⁻ decays at 50–75 GeV → ~50 % above 150 GeV).
- Decays inside/after the tracker (z > 6 m) are not accepted (≥ 3 layers needed). Statistics
  above 100 GeV are small (30–55 p π⁻ decays per 25 GeV bin).

## 9. Documents (2026-11-12)
- README updated for hit seeding, the 5 keV default, the 100–275 GeV samples and the results of 07/08.
- The first report/slides (3 Nov) are kept as they were; the changes and new results are in
  `docs/report_update/report_update.pdf` and `docs/slides_update/slides_update.pdf`. They also correct
  the first report: the "other" Λs interacted (none left the world volume), and "88–90 % of p π⁻ decays
  before the B0" is conditional; per generated Λ the decay law holds.

## 10. Λ-tagged DIS (2026-11-12, running)
- From the particle gun to a physics process: Pythia8 8.316 (container) NC DIS, ep 18×275,
  Q² > 1 GeV², ePIC campaign settings; Λ stable in Pythia (Geant4 decays it); only events with a Λ
  at θ < 25 mrad are kept (`scripts/gen/`). No beam effects; ddsim applies the crossing angle
  (`scripts/sim/ddsim_hepmc.sh`). The official campaign files (JLab xrootd) are not reachable from
  lxplus.
- 7 % of DIS events have a Λ at < 25 mrad; 3.2 % one in the B0 range (5.5–20 mrad). Most forward
  Λs are at 2.5–5 mrad (ZDC / Roman-pot angles). B0-range Λs are softer than the gun samples:
  momentum peak 25–100 GeV, few above 175 GeV.
- Samples: seeds 301, 302 × 500 kept events, the same events in the old and new geometry,
  reconstructed with #3116 (`scripts/production/dis_lambda_sample.sh`, `$EIC_DATA/lambda_dis`).
  Full DIS events: ~15 s/event in ddsim.
- Analysis `macros/dis/lambda_dis.C` (results/09_dis): same chain per B0-direction Λ (decay before
  the B0, acceptance, tracking efficiency, yield), and the mass from all forward (+,−) track pairs,
  where the rest of the DIS event now adds combinatorial background.

## Open points
- Why the CKF loses soft π⁻ (rerun with CKF debug output on a few events).
- Realistic B0 seeding: works with #3116 (section 7); follow it until merged.
- The jump of the old-geometry truth-seeded efficiency between EICrecon builds (section 7).
- The +2 MeV mass shift in the old geometry.
- Real events (other tracks, beam background) need vertexing / decay-length cuts.
