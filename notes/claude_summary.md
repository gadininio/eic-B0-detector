# Summary: Λ → p π⁻ in the ePIC B0 tracker (sessions of 2026-11-02 – 11-19)

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

## 10. Λ-tagged DIS (2026-11-12 – 11-19)
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
  where the rest of the DIS event now adds combinatorial background. One PNG per sample (4 × 2
  panels: spectrum, acceptance, efficiency, A×ε per p π⁻ decay, yield per B0-direction Λ, mass with
  truth and hit seeding, main numbers), titled with the beam energies and the geometry.
- Definitions. B0-direction Λ: generated Λ at θ = 5.5–20 mrad from the proton beam (an angular
  proxy; Λs pointing into the new geometry's φ gap are still counted). Acceptance: both daughters with
  their own simulated hits in ≥ 3 stations, per p π⁻ decay of a B0-direction Λ wherever it decays (decays
  behind the first station are in the denominator; per decay before z = 5.8 m it would be 60 / 18 %
  at 18×275 and 58 / 24 % at 9×130, old / new). Fiducial efficiency: p, π⁻, both reconstructed for
  decays with both daughters accepted.
- Only ~50 % of the B0-direction Λs decay to p π⁻ (18×275: old 216, new 206 of 417): n π⁰ 117 / 130,
  and ~20 % never decay (84 / 81), mostly interacting at z = 5.8–20 m (B0 and downstream material).
  Among the decays, p π⁻ / (p π⁻ + n π⁰) = 65 / 61 % ≈ BR 64 %. Old vs new differ only statistically
  (Geant4 draws decays independently in each geometry run).

- Also at **9×130** (own production, same chain, `BEAM=9x130`), and as a cross-check the ePIC campaign
  26.07.3 **SIDIS 9×130** (pythia6-lhapdf ep, x 1e-3–1e-2, 100 RECO files from the BNL xrootd server;
  `scripts/production/campaign_fetch.sh`). Campaign files: Λ decayed by the generator (status 2, accepted
  by `LambdaB0.h` now), no material interactions of the Λ; the campaign reconstruction predates
  EICrecon#2901 (B0 truth seeds for secondaries, merged 2026-10-31) and has no hit seeding, so the B0
  tracking efficiency of the daughters is ~0 there: usable for rates, spectra and acceptance only.
  No campaign has the new B0 geometry or 18×275 ep DIS.

- Results (results/09_dis; own samples 1000 events each, the same events in both geometries;
  9×130 seed 402 ran on HTCondor, `scripts/condor/dis_lambda.sub`):

  | | 18×275 old | 18×275 new | 9×130 old | 9×130 new |
  |---|---|---|---|---|
  | B0-direction Λ (5.5–20 mrad) | 417 | 417 | 703 | 703 |
  | p π⁻ decays / before z = 5.8 m | 216 / 192 | 206 / 183 | 378 / 339 | 372 / 341 |
  | both daughters accepted, per p π⁻ | 53.2 % | 16.0 % | 51.9 % | 21.8 % |
  | eff p, truth / hit seeding | 93.5 / 98.8 % | 87.2 / 100 % | 95.1 / 99.7 % | 97.1 / 100 % |
  | eff π⁻, truth / hit | 55.7 / 67.9 % | 52.6 / 56.1 % | 51.1 / 58.1 % | 45.2 / 53.2 % |
  | A×ε both, per p π⁻ decay, truth / hit | 28.7 / 36.1 % | 7.3 / 10.2 % | 25.9 / 31.2 % | 10.5 / 12.6 % |
  | both reco per B0-direction Λ, truth / hit | 14.9 / 18.7 % | 3.6 / 5.0 % | 13.9 / 16.8 % | 5.5 / 6.7 % |
  | mass ±10 MeV, hit seeding, S / B | 54 / 54 | 18 / 10 | 89 / 24 | 34 / 8 |

  Fiducial volume = p π⁻ decays with both daughters accepted; the efficiencies below are measured on
  these decays only, so acceptance × ε(both) = fiducial A×ε exactly. The plain A×ε also counts the
  few decays reconstructed although a daughter has < 3 layers of its own hits.

  | beam | geometry | seeding | acceptance (both) | ε p | ε π⁻ | ε both | A×ε (fiducial) | A×ε (all) |
  |---|---|---|---|---|---|---|---|---|
  | 18×275 | old | truth | 53.2 % (115/216) | 92.2 % | 54.8 % | 52.2 % | 27.8 % | 28.7 % |
  | 18×275 | old | hit   | 53.2 % | 99.1 % | 65.2 % | 64.3 % | 34.3 % | 36.1 % |
  | 18×275 | new | truth | 16.0 % (33/206) | 78.8 % | 51.5 % | 42.4 % | 6.8 % | 7.3 % |
  | 18×275 | new | hit   | 16.0 % | 100 % | 60.6 % | 60.6 % | 9.7 % | 10.2 % |
  | 9×130 | old | truth | 51.9 % (196/378) | 95.9 % | 53.1 % | 50.0 % | 25.9 % | 25.9 % |
  | 9×130 | old | hit   | 51.9 % | 100 % | 59.2 % | 59.2 % | 30.7 % | 31.2 % |
  | 9×130 | new | truth | 21.8 % (81/372) | 97.5 % | 46.9 % | 46.9 % | 10.2 % | 10.5 % |
  | 9×130 | new | hit   | 21.8 % | 100 % | 56.8 % | 56.8 % | 12.4 % | 12.6 % |

  Campaign SIDIS 9×130 (19 900 events): 3.4 % of events have a B0-direction Λ; both daughters
  accepted 48.3 % of p π⁻ (as in our samples); truth-seeded eff p 6.4 %, π⁻ 3.0 %, A×ε 2.0 %.
- Inside the fiducial volume both geometries track about equally well (π⁻ 50–65 %, proton ~100 %
  with hit seeding); the difference in A×ε is almost all acceptance (old / new: 3.3× at 18×275,
  2.4× at 9×130). The per-daughter efficiencies of the first table use all accepted protons
  (pions), not only decays with both accepted, so acceptance × ε p × ε π⁻ does not reproduce A×ε;
  the fiducial ones do (up to the small p–π⁻ correlation).
- The new geometry loses acceptance mainly through its ±45° φ gap (beam frame, electron side:
  0 % there, 27 % of the decays) and at small θ (5.5–8 mrad: 11 vs 51 % at 18×275), where DIS Λs
  pile up; hence a larger drop than for the gun. A×ε rises with p_Λ (π⁻ efficiency).

- Why the new acceptance is so much lower: see section 11.

## 11. B0 geometry: sensitive area, coverage and beamline (2026-11-18/19)
- `scripts/geometry/b0area.py` (results/10_b0_geometry, `make_results.sh 10`) rasterizes the sensors
  of both epic commits from their `B0_tracker.xml` (copied from GitHub into `$EIC_TOP/b0geo`) and
  maps straight lines from the IP onto the stations (ion-axis frame, +x = electron side).
- Old B0: 4 layers of full silicon disks, 36 trapezoids of 10°, r = 35–150 mm, shorter (outer
  r = 85–105 mm) on the electron side: 585–605 cm² per layer. New B0: 4 stations of 16 × 16 mm
  AC-LGAD sensors, 3 per stave, staves on both faces of a support plate (44/44/44/48 staves):
  307–335 cm² per station (~52 %). Beam hole r = 36.75 mm (old inner r 35 mm), but the sensor grid
  leaves an effective ~±45 mm square hole at stations 3–4 (θ ≈ 7 mrad); a slot next to the beam pipe
  removes the sensors on the electron side.
- Straight lines from the IP crossing ≥ 3 stations (solid-angle weighted), old / new: θ = 5.5–20 mrad
  92 / 56 %; 5.5–8 mrad 94 / 32 %; 8–17 mrad ~97 / ~72 %; 17–20 mrad 83 / 33 %; φ = −45…+45° 70 / 0 %,
  other φ 99 / 70–79 %. Consistent with the geantino scan (95.3 / 70.1 %, 8–18 mrad). Sensitive area
  and coverage differ: area counts all silicon (the old disks reach beyond 20 mrad), coverage asks
  where the silicon is relative to the cone and whether the stations line up (new: 52 % of the area,
  61 % of the coverage).
- Beamline (same in both geometries): the proton beam pipe (Be tube, r = 31 mm) runs through the centre
  of the stations; the electron beam runs inside the final-focus quadrupole Q0EF (z = 5.8–7.0 m, outer
  support r = 61 mm) at x = +146 to +172 mm from the ion axis (25 mrad crossing angle); both are inside
  the B0 dipole bore (r = 200 mm, axis at global x = −145.8 mm). The old disks are cut back just to
  Q0EF's edge (85 → 111 mm); the new sensors stop ~40 mm before it. Second figure
  `b0_coverage_maps_beamline.png` draws them on the station maps.
- Consequence for Λs: both daughters must be covered, so the loss roughly squares
  (0.92² ≈ 0.85 vs 0.56² ≈ 0.31), and DIS Λs pile up at small θ, where the new coverage is lowest.
  (Straight lines only: displaced decays and the B0 dipole bending the π⁻ are not in these numbers.)

## 12. Kaon structure function (2026-11-19 – 11-25)
Sullivan process e p → e′ Λ X: the photon probes a virtual K⁺ of the proton, the fast Λ is the tag.
Variables: x_L = p_z(Λ, beam frame) / E_p, |t| = −(P_beam − P_Λ)², x_K = x / (1 − x_L). Measurement region:
high x_L (here > 0.6) and small |t|.

**Which Λs go where** (`macros/exploration/lambda_xl_theta.C`; generated Λs, angle from the proton beam).
- Signal model: ePIC campaign 26.07.3 `EXCLUSIVE/MESON_SF_ABCONV/eicMesonSFGen-1.0.0` (Sullivan K⁺Λ, RECO only,
  2–4 files × 100 events per beam energy, BNL xrootd). Λ pT ≈ 0.4–1.2 GeV at every energy, x_L ≥ 0.4,
  |t| ≲ 1.9 GeV², so θ ≈ pT / (x_L E_p) ∝ 1/E_p; hard edge θ_max · E_p ≈ 1.5 GeV (generator phase space).
- High-x_L (> 0.6) Λs in the B0 (5.5–20 mrad): Sullivan 9x130 239 / 299, 10x100 134 / 147, 18x275 **0 / 160**
  (all < 5.5 mrad: ZDC / Roman pots / off-momentum detectors), 5x41 75 / 306 (mostly 20–40 mrad).
  Our Pythia Λ-tagged DIS: 9x130 162 / 244 in the B0; 18x275 only 13 / 144 (at |t| ≈ 2 GeV²).
- The B0-direction Λs of the 18x275 DIS sample (section 10) are real but 404 / 417 have x_L < 0.6
  (remnant fragmentation). ⇒ **The B0 measurement of the kaon SF is a 9x130 / 10x100 measurement.**
- The Pythia sample is the right final state and can be used to start; Pythia has no kaon-exchange
  mechanism, so the Sullivan generator is needed later as the signal model (flux factor, t dependence).
  The campaign MESON_SF reconstruction predates EICrecon#2901 (no B0 tracking of Λ daughters) and has no
  FULL files ⇒ for B0 tracking it must be regenerated and simulated with our chain.

**Step 1: x_L > 0.6 on the 9x130 DIS samples** (`lambda_dis.C`, new last argument `xLMin`;
`make_results.sh 11` → results/11_kaon_sf/dis_9x130_{old,new}_xl06; with xLMin = 0 the old results are
reproduced exactly). 162 B0-direction Λs with x_L > 0.6 in 1000 events; mean x_L 0.71, mean |t| 1.38 GeV²,
none below |t| ≈ 0.5 GeV² (the B0 starts at 5.5 mrad ⇒ pT ≳ 0.5 GeV: the smallest |t| is out of reach).

| 9x130, x_L > 0.6 | old B0 | new B0 |
|---|---|---|
| both daughters accepted, per p π⁻ decay | 61.9 % | 30.1 % |
| A×ε per p π⁻ decay, truth / hit seeding | 42.9 / 51.2 % | 19.2 / 24.7 % |
| both reconstructed per B0-direction Λ, truth / hit | 22.2 / 26.5 % | 8.6 / 11.1 % |
| mass ±10 MeV, hit seeding, S / B | 31 / 2 | 11 / 0 |
| x_L reco − true (hit): mean, RMS | +0.012, 0.033 | −0.010, 0.014 |
| \|t\| reco − true (hit): mean, RMS [GeV²] | −0.025, 0.080 | −0.003, 0.049 |

Higher acceptance and π⁻ efficiency than without the cut (harder π⁻), and almost no combinatorial
background. Reconstructed Λs have larger |t| (acceptance favours larger angles). No beam effects
in the simulation: the |t| resolution is optimistic.

**Step 2: DIS kinematics from the scattered electron** (`macros/kaon/kaon_kinematics.C`,
results/11_kaon_sf/kin_9x130_{old,new}).
- **Crossing angle in our simulation:** ddsim `--crossingAngleBoost -0.025` is a pure Lorentz boost; it
  gives *both* beams px/E = −0.025 (beam e⁻ px/|pz| = −0.025). The campaign afterburner files have the
  electron beam along −z (px/|pz| ~ 1e-4) and the proton at −25 mrad. Proton side identical ⇒ the B0 / Λ
  results are not affected. Electron side tilted by 25 mrad ⇒ EICrecon's InclusiveKinematics* (which put
  the beams into the ePIC frame) are wrong for our samples: Q² reco/true spread ±25 %.
- Fix used: Lorentz invariants Q² = −q², y = P·q / P·e, x = Q² / 2P·q from the true beam four-vectors of
  the event (status 4) and the reconstructed scattered electron (the track EICrecon uses,
  ScatteredElectronsTruth = truth-matched; found in 97–98 % of events). Own truth = InclusiveKinematicsTruth.
- Electron method, old B0 (new similar), reco/true − 1, median [16 %, 84 %]: Q² −0.03 [−0.09, −0.01] at
  y < 0.01 and within a few % above; x useless at y < 0.01 (−0.90; reco y ≤ 0 in 1/3 of these events),
  −0.33 [−0.73, +0.74] at 0.01–0.05, ±25 % at 0.05–0.2, few % above 0.2.
- The high-x_L events sit at low y: of the 162 events with a B0-direction Λ at x_L > 0.6, 111 have
  y < 0.01, 29 have 0.01–0.05, 22 have y > 0.05. Reconstructed Λ candidates (no truth, best pair in
  ±10 MeV): old 32 (30 signal), 21 at y < 0.01; new 9 (9), 5 at y < 0.01. x_L of the candidate:
  +0.012 [−0.009, +0.031] (old).
- ⇒ with the electron method only the few y > 0.05 events give a usable x_K. Next: hadronic-final-state
  methods (Σ, eΣ, double angle) computed in the right frame, and the usual y cuts.

## Open points
- Why the CKF loses soft π⁻ (rerun with CKF debug output on a few events).
- Realistic B0 seeding: works with #3116 (section 7); follow it until merged.
- The jump of the old-geometry truth-seeded efficiency between EICrecon builds (section 7).
- The +2 MeV mass shift in the old geometry.
- Real events (other tracks, beam background) need vertexing / decay-length cuts.
- A strict Λ acceptance (only Λs whose straight line would enter ≥ 3 B0 stations) and the effect of
  the B0 dipole on the π⁻ coverage.
- Why the new support plates have a slot on the electron side next to the beam pipe (not stated in
  the xml).
- Electron side of our simulation: ddsim `--crossingAngleBoost` tilts the electron beam by 25 mrad
  (section 12). Redo with the ePIC afterburner (abconv) before electron acceptance or EICrecon
  kinematics are used; B0 / Λ results unaffected.
- Kaon SF: low-y kinematics (Σ / eΣ / DA with the hadronic final state), Sullivan signal sample through
  our chain, kaon flux / t-extrapolation, beam effects for |t|.

## Next studies (planned 2026-11-19, each in its own session, in this repository)
All three build on the forward Λ reconstructed through Λ → p π⁻ in the B0 (`macros/common/LambdaB0.h`,
the DIS production and condor scripts, `make_results.sh`); each gets its own macro folder and
`results/1x_…` section. Start a session with "read notes/claude_summary.md and start the … study".
1. **Λ polarization** (first: can start from the existing DIS samples). Needs the proton angle in
   the Λ rest frame, i.e. the full p π⁻ reconstruction and its angular acceptance × efficiency.
   Pythia does not polarize the Λ: reweight the decay angle, or use a generator with polarization.
2. **Kaon structure function** — started, see section 12 and results/11_kaon_sf. (Sullivan process, p → Λ K⁺ with the Λ as the forward tag): needs the
   tagged-Λ acceptance and efficiency in a kaon-exchange (TDIS-type) sample, probably a dedicated
   generator (check what ePIC uses).
3. **Kaon EM form factor** (exclusive e p → e′ K⁺ Λ): the same Λ tag plus the exclusive K⁺ and the
   t reconstruction; needs an exclusive generator.
