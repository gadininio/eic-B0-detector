# Summary: Λ → p π⁻ in the ePIC B0 tracker (sessions of 2026-11-02/03)

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
  (`-PB0TRK:B0TrackerRawHits:threshold=5e-6`). No effect in the old geometry.
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

## Open points
- Why the CKF loses soft π⁻ (rerun with CKF debug output on a few events).
- Realistic B0 seeding (EICrecon#3028) instead of truth seeding.
- The +2 MeV mass shift in the old geometry.
- Real events (other tracks, beam background) need vertexing / decay-length cuts.
