# Λ → p π⁻ in the ePIC B0 tracker

Study of how well the ePIC far-forward **B0 tracker** reconstructs Λ hyperons through
Λ → p π⁻, with a single-Λ particle gun: geometric acceptance, acceptance of the Λ daughters,
tracking efficiency, acceptance × efficiency, and the reconstructed Λ mass, for the B0 geometry
before and after [eic/epic#1194](https://github.com/eic/epic/pull/1194); with truth seeding and
with the hit (realistic) seeding of [EICrecon#3116](https://github.com/eic/EICrecon/pull/3116),
and for Λ momenta from 50 to 275 GeV. Then with a physics process, **Λ-tagged DIS** (Pythia8 ep at
18×275 and 9×130 GeV, and the ePIC campaign SIDIS 9×130 sample), and a comparison of the two B0
geometries' **sensitive area and coverage** with the beamline around them.

- Detailed write-up (3 Nov 2026, truth seeding, 50–100 GeV): [`docs/report/report.pdf`](docs/report/report.pdf),
  slides [`docs/slides/slides.pdf`](docs/slides/slides.pdf)
- **Update** (12 Nov 2026: hit seeding, 5 keV default, 100–275 GeV, changes w.r.t. the first report):
  [`docs/report_update/report_update.pdf`](docs/report_update/report_update.pdf),
  slides [`docs/slides_update/slides_update.pdf`](docs/slides_update/slides_update.pdf)
- Short summary of the work: [`notes/claude_summary.md`](notes/claude_summary.md); full session log:
  [`notes/claude_log.md`](notes/claude_log.md), all commands: [`notes/claude_commands.md`](notes/claude_commands.md)

## Setup

| | |
|---|---|
| Sample | Λ gun, θ = 8–18 mrad (uniform), φ uniform, 25 mrad crossing angle (`--crossingAngleBoost -0.025`), 18×275 GeV craterlake geometry; p = 50–100 GeV (main samples) and 100–275 GeV |
| Simulation | `ddsim` (DD4hep/Geant4) in eic-shell, `Geant4TVUserParticleHandler` |
| Old geometry | epic `43f427e` (was the geometry of the eic-shell nightly of 2026-11-02; the nightlies since have the new B0, so it is now built locally: `epic_43f427e`): B0 sensors 300 µm |
| New geometry | epic `ec774d8` = merge of eic/epic#1194 (realistic B0; = epic tag 26.11.0): B0 sensors 50 µm, built locally |
| Reconstruction | (a) EICrecon main `8b44cae` (built locally; contains EICrecon#2901, B0 truth seeding for displaced vertices); first report. (b) **EICrecon#3116 branch at `1ca147b`** (= main of 2026-11-11 + #3116; also contains #2901 and #3084, the 5 keV B0 threshold as default); update. The nightly EICrecon `a7bab3f` of 2026-11-02 has only 4 B0 tracks per 100 Λ |
| Tracking | ACTS CKF. **Truth seeding** (`B0TrackerTruthSeeds`, #2901) → `ReconstructedTruthSeededChargedParticles`; **hit seeding** (Acts triplet seeder on 3 B0 stations, #3116, open) → `ReconstructedChargedParticles`. A #3116 build writes both, so they are compared on the same events |
| Samples | 100 events per geometry (seed 42), 1000 events per geometry at 50–100 GeV (seeds 101, 102 × 500), 1000 events per geometry at 100–275 GeV (seeds 201, 202 × 500), 5000 geantinos per geometry |
| DIS samples | Pythia8 8.316 (in eic-shell) NC DIS, Q² > 1 GeV², ePIC campaign settings, Λ stable in Pythia (Geant4 decays it); only events with a Λ at θ < 25 mrad are kept (`scripts/gen/`); no beam effects, ddsim applies the crossing angle. 18×275 (seeds 301, 302) and 9×130 (seeds 401, 402), 2 × 500 kept events each, the same events in both geometries, reconstructed with the #3116 build. Cross-check: ePIC campaign 26.07.3 SIDIS 9×130 (pythia6, 100 RECO files, 19 900 events, BNL xrootd) |

## Definitions

The same definitions, grouped the same way, are on the project website (Definitions section).

### Samples and detector

| Quantity | Definition | Code |
|---|---|---|
| Old / new B0 | epic `43f427e` (silicon disks) and epic `ec774d8` (AC-LGAD stations, eic/epic#1194) | `scripts/build/build_epic.sh` |
| Beam frame (angles) | angles w.r.t. the proton beam, the 25 mrad crossing-angle rotation undone | `LambdaB0.h` |
| B0-direction Λ (DIS) | generated Λ at θ = 5.5–20 mrad from the proton beam. An angular proxy for "pointing at the B0", not checked against the layout: Λs pointing into the new geometry's φ gap still count | `macros/dis/lambda_dis.C` |
| Λ → p π⁻ decay | generated Λ (generatorStatus 1, or 2 in campaign files where the generator decays it) with exactly two daughters, p and π⁻. A Λ that interacts in material is also the parent of its products, but has more daughters, and does not count | `macros/common/LambdaB0.h` |
| B0 station (layer) | hits grouped in z; a gap > 50 mm starts a new station (4 stations at z ≈ 5.8–6.9 m; the front and back faces of a new-geometry station count as one) | `LambdaB0.h` |
| Decay before the B0 | decay vertex at z < 5.8 m; probability per Λ BR · (1 − e^(−L/λ)) with λ = p[GeV] × 7.07 cm and L = 5.8 m | `macros/efficiency/lambda_vs_momentum.C` |
| Truth / hit seeding | B0 tracks seeded from the true vertex and momentum (`B0TrackerTruthSeeds`, EICrecon#2901) or from triplets of hits on 3 stations (EICrecon#3116) | `scripts/reco/eicrecon.sh` |
| Seed of a daughter | truth seeding: a `B0TrackerTruthSeedParameters` entry with its charge and direction; hit seeding: a `B0TrackerSeeds` triplet with ≥ 2 of its 3 hits made by the daughter | `lambda_tracking_efficiency.C` |
| Reconstructed daughter | a reconstructed charged particle whose best truth match (highest association weight) is the daughter; `ReconstructedTruthSeededChargedParticles` (truth seeding, default) or `ReconstructedChargedParticles` (hit seeding, macro argument `"hit"`) | `LambdaB0.h` (`lb0::Seeding`) |
| Threshold | EICrecon digitization: energy per pixel must exceed it (`B0TRK:B0TrackerRawHits:threshold`; 10 keV in main 8b44cae, **5 keV default since EICrecon#3084**, merged 2026-11-03) | `scripts/reco/eicrecon.sh` |

### Acceptance and efficiency

| Quantity | Definition | Code |
|---|---|---|
| Accepted daughter | its own simulated B0 hits cover ≥ 3 stations. Full simulation (decay point, B0 dipole field, material), no digitization or tracking. Not a fiducial volume: a daughter must really leave hits in ≥ 3 stations | `LambdaB0.h` |
| **Acceptance A** | A = N(both daughters accepted) / N(p π⁻), per p π⁻ decay, wherever the Λ decays (decays behind the first station are in the denominator, so A mixes "decays early enough" with "daughters reach the stations"); also for p and π⁻ separately | `macros/acceptance/lambda_daughter_acceptance.C`, `lambda_dis.C` |
| **Tracking efficiency ε** | ε_p = N(p reconstructed) / N(p accepted), the same for π⁻, over every accepted daughter. The geometry cancels. Includes digitization (B0 threshold), seeding, CKF, track filtering | `macros/efficiency/lambda_tracking_efficiency.C` |
| **Fiducial efficiency** | ε_p, ε_π and ε_both measured only on decays with **both** daughters accepted, so A · ε_both = fiducial A×ε exactly. With the per-daughter ε above, A · ε_p · ε_π does not reproduce A×ε: those use all accepted protons (pions), a softer set | `macros/dis/lambda_dis.C` |
| **A×ε** | N(both reconstructed) / N(p π⁻), per p π⁻ decay. Also counts the few decays reconstructed although a daughter has < 3 stations of its own hits, so it is 0–2 points above the fiducial A · ε_both | `macros/efficiency/lambda_acceptance_x_efficiency.C`, `lambda_dis.C` |
| **Yield Y** (DIS) | N(both reconstructed) / N(B0-direction Λ) = f_pπ · A×ε, with f_pπ ≈ 0.5 the fraction decaying to p π⁻ (BR 64 %, and ~20 % of the Λs never decay but interact in material) | `macros/dis/lambda_dis.C` |
| Per generated Λ (gun) | the same counts divided by all generated Λs. The unbiased denominator: a Λ that has not decayed by ~6 m nearly always interacts in material before it can decay, so the Λ → p π⁻ decays are mostly early ones (90 % before the B0 at 50–100 GeV) | `macros/efficiency/lambda_vs_momentum.C` |
| Geometric acceptance | fraction of **straight lines** from the IP at the Λ gun angles crossing ≥ 3 stations (geantino gun). Pure detector coverage; *not* a Λ acceptance: the Λ is neutral, its daughters start at a displaced vertex and the π⁻ is bent by the B0 dipole | `macros/acceptance/b0_geometric_acceptance.C` |
| Sensitive area / coverage | silicon area per station (cm²), vs the fraction of straight lines from the IP in the 5.5–20 mrad cone (solid-angle weighted) that cross sensors in ≥ 3 stations. Area counts all silicon; coverage asks where it is relative to the cone and whether the stations line up | `scripts/geometry/b0area.py` |

### Mass and selection

| Quantity | Definition | Code |
|---|---|---|
| Forward tracks | reconstructed charged particles at θ < 30 mrad in the beam frame (the B0 tracks; the central tracker starts at ~40 mrad) | `lambda_dis.C`, `lambda_mass_notruth.C` |
| Λ candidate | every (+,−) pair of forward tracks, + as proton and − as π⁻, with p(+)/p(−) > 2.5 (for 50–100 GeV Λs the true ratio is 3.1–14.6). No truth in the selection | `macros/mass/lambda_mass_notruth.C`, `lambda_dis.C` |
| Signal / background | signal: both tracks truth-matched to the daughters of the same Λ → p π⁻, used only as a label. S and B counted within ±10 MeV of the PDG mass | `lambda_dis.C` |
| Purity / cut efficiency | (signal candidates / selected candidates); (signal kept by the cut / signal before it) | `macros/mass/lambda_mass_notruth.C` |

### Kaon kinematics (kaon structure function study)

| Quantity | Definition | Code |
|---|---|---|
| Sullivan process | e p → e′ Λ X: the proton fluctuates into Λ + virtual K⁺, the photon probes the K⁺ | |
| x_L | Λ momentum along the proton beam / beam momentum | `macros/exploration/lambda_xl_theta.C`, `lambda_dis.C` |
| \|t\| | −(P_beam − P_Λ)², the four-momentum transfer to the Λ | `lambda_dis.C` |
| x_K | x / (1 − x_L), the momentum fraction of the struck quark in the kaon | `macros/kaon/kaon_kinematics.C` |
| x, Q², y | from q = e_beam − e′: Q² = −q², y = P·q / P·e_beam, x = Q² / (2 P·q) | `kaon_kinematics.C` |
| Σ_h, p_T,h | Σ(E − p_z) and \|Σ p_T\| of the hadronic final state in the head-on frame | `kaon_kinematics.C` |
| DA, JB, Σ, eΣ, hybrid | double-angle, Jacquet–Blondel, Σ and eΣ methods; the hybrid uses DA when y_DA < 0.05 and the electron otherwise | `kaon_kinematics.C` |

## Main results

### First report: truth seeding, EICrecon main 8b44cae (1000 Λ per geometry, 50–100 GeV)

| | old geometry | new geometry, 10 keV | new geometry, 5 keV |
|---|---|---|---|
| Geometric acceptance, ≥ 3 layers (geantino) | 95.3 % | 70.1 % | 70.1 % |
| Λ → p π⁻ / n π⁰ / other | 50 / 25 / 25 % | 48 / 28 / 24 % | same sample |
| Daughter acceptance p / π⁻ / **both** (per p π⁻ decay) | 80 / 69 / **63 %** | 61 / 49 / **41 %** | 61 / 49 / **41 %** |
| Tracking efficiency p / π⁻ / both | 82 / 56 / 47 % | 86 / 58 / 50 % | 93 / 61 / 59 % |
| Acceptance × efficiency p / π⁻ / **both** | 66 / 39 / **30 %** | 53 / 29 / **21 %** | 57 / 31 / **25 %** |
| Both daughters reconstructed, per generated Λ | 15.0 % | 10.1 % | **12.0 %** |
| M(p π⁻) truth-matched: Gaussian mean / σ | 1.1177 GeV / 6.3 MeV | 1.1160 GeV / 7.4 MeV | 1.1159 GeV / 6.9 MeV |
| No truth in selection, p(+)/p(−) > 2.5: candidates / purity | 155 / 95 % | 103 / 96 % | 122 / 97 % |

### Update: truth vs hit seeding, EICrecon#3116 build, 5 keV (same 1000 Λ per geometry; `results/07_hit_seeding`)

| | old, truth | old, **hit** | new, truth | new, **hit** |
|---|---|---|---|---|
| Seed found for accepted p / π⁻ | 99 / 99 % | 100 / 100 % | 97 / 96 % | 100 / 100 % |
| Tracking efficiency p / π⁻ | 97.5 / 66.5 % | 99.5 / 72.5 % | 93.2 / 60.8 % | 100 / 64.6 % |
| Acceptance × efficiency, both, per p π⁻ decay | 41.7 % | 45.5 % | 24.7 % | 27.0 % |
| Both daughters reconstructed, per generated Λ | 21.0 % | **22.9 %** | 12.0 % | **13.1 %** |
| M truth-matched: pairs, Gaussian mean, σ | 210, 1.1174 GeV, 6.9 MeV | 229, 1.1175, 6.8 | 120, 1.1159, 6.8 | 131, 1.1159, 6.3 |
| Δp/p of the daughters (RMS) | 3.8 % | 4.4 % | 3.4 % | 3.7 % |
| No truth, p(+)/p(−) > 2.5: candidates / purity | 242 / 86 % | 348 / 64 % | 122 / 97 % | 138 / 93 % |
| ±20 MeV window: signal / candidates | 200 / 207 | 206 / 219 | 110 / 110 | 116 / 116 |

### Update: vs Λ momentum, 50–275 GeV (EICrecon#3116 build; `results/08_momentum`)

| p(Λ) | p π⁻ decay before the B0, per generated Λ (decay law) | π⁻ tracking eff., truth / hit (new) | both reconstructed per generated Λ, old (truth / hit) | same, new |
|---|---|---|---|---|
| 50–75 GeV | 48 % (47 %) | 52 / 53 % | 20.9 / 22.5 % | 9.8 / 10.0 % |
| 150–175 GeV | 25–30 % (25 %) | 86 / 95 % | 24.6 / 26.1 % | 8.9 / 11.6 % |
| 250–275 GeV | 18–23 % (17 %) | 93 / 100 % | 16.3 / 17.0 % | 8.9 / 9.6 % |

### Update: Λ-tagged DIS (EICrecon#3116 build; `results/09_dis`)

1000 events per beam energy and geometry. Efficiencies in the fiducial volume (both daughters accepted).

| | 18×275 old | 18×275 new | 9×130 old | 9×130 new |
|---|---|---|---|---|
| B0-direction Λ (5.5–20 mrad) | 417 | 417 | 703 | 703 |
| p π⁻ decays | 216 | 206 | 378 | 372 |
| Both daughters accepted, per p π⁻ decay | 53.2 % | 16.0 % | 51.9 % | 21.8 % |
| ε p / π⁻ / both, truth seeding | 92 / 55 / 52 % | 79 / 52 / 42 % | 96 / 53 / 50 % | 98 / 47 / 47 % |
| ε p / π⁻ / both, hit seeding | 99 / 65 / 64 % | 100 / 61 / 61 % | 100 / 59 / 59 % | 100 / 57 / 57 % |
| A×ε both, per p π⁻ decay, truth / hit | 27.8 / 34.3 % | 6.8 / 9.7 % | 25.9 / 30.7 % | 10.2 / 12.4 % |
| Both reconstructed per B0-direction Λ, truth / hit | 14.9 / 18.7 % | 3.6 / 5.0 % | 13.9 / 16.8 % | 5.5 / 6.7 % |
| Mass, forward (+,−) pairs, p(+)/p(−) > 2.5, ±10 MeV, hit: S / B | 54 / 54 | 18 / 10 | 89 / 24 | 34 / 8 |

- Only ~50 % of the B0-direction Λs decay to p π⁻: 64 % branching ratio, and ~20 % never decay
  (they interact in material, mostly at z = 5.8–20 m).
- Inside the fiducial volume both geometries track about equally well; the difference is the
  acceptance (3× lower in the new geometry at 18×275, 2.4× at 9×130).
- Campaign SIDIS 9×130: 3.4 % of events have a B0-direction Λ, both daughters accepted for 48 % of
  p π⁻ decays, but B0 tracking of the daughters is ~0 (truth-seeded ε p 6 %, π⁻ 3 %): the campaign
  EICrecon predates #2901. Good for rates, spectra and acceptance only.

### Update: B0 geometry, sensitive area and coverage (`results/10_b0_geometry`)

From the B0 tracker xml of both epic commits (`scripts/geometry/b0area.py`).

| | old (43f427e) | new (ec774d8) |
|---|---|---|
| Layout | 4 layers of full silicon disks, 36 trapezoids of 10°, r = 35–150 mm, shorter on the electron side | 4 stations of 16 × 16 mm AC-LGAD sensors on staves, both faces of a support plate |
| Sensitive area per station | 585–605 cm² | 307–335 cm² (~52 %) |
| Straight lines crossing ≥ 3 stations, θ = 5.5–20 mrad | 92 % | 56 % |
| … θ = 5.5–8 / 8–17 / 17–20 mrad | 94 / ~97 / 83 % | 32 / ~72 / 33 % |
| … φ = −45…+45° (electron side) | 70 % | 0 % |

- In the B0 frame the proton beam pipe (r = 31 mm) goes through the centre; the electron beam runs
  inside the final-focus quadrupole Q0EF (outer r = 61 mm), at x = +146 to +172 mm along the
  tracker, and both sit inside the B0 magnet bore (r = 200 mm). The old disks are cut back just
  enough for Q0EF; the new sensors stop ~40 mm before it, and a slot next to the proton pipe removes
  the rest of the electron side.
- Both daughters must be covered, so the loss roughly squares (0.92² ≈ 0.85 vs 0.56² ≈ 0.31), and
  DIS Λs pile up at small θ, where the new coverage is lowest.

Main findings
- EICrecon#2901 is essential for truth seeding: with the nightly EICrecon (a7bab3f) the B0 truth
  seeder only seeds particles from |z| < 200 mm, giving 4 B0 tracks per 100 Λ; with main, 77 (old) / 44 (new geometry).
- The new geometry (eic/epic#1194) has a gap of about ±45° in φ on the electron-beam side
  (geometric acceptance 95 % → 70 %).
- Its 50 µm sensors give ≈ 14 keV per hit (most probable), so a 10 keV threshold loses 9 % of
  hits and 7 % of accepted daughters; 5 keV recovers them. 5 keV is the EICrecon default since #3084.
- **Hit seeding (#3116) works for displaced Λ daughters:** a seed for 100 % of the accepted
  daughters, every accepted proton reconstructed, and 1–2 percentage points more Λs fully reconstructed than with
  truth seeding; the same mass peak (M(hit) − M(truth) RMS 4.4 MeV on common decays). It also
  tracks secondaries from material: in the old, thick geometry the background without truth grows
  (purity after the cut 86 → 64 %); in the new geometry little (97 → 93 %).
- The π⁻ tracking efficiency is low because it is soft (3–25 GeV at p_Λ = 50–100 GeV):
  only 35–48 % of the accepted π⁻ of 5–10 GeV get a track. The loss is in the CKF, not in seeding (hit
  seeding finds a seed for all of them; removing the truth-seed momentum smearing changes nothing).
  Cause not yet identified.
- **Energetic Λs:** the probability to decay before the B0 follows the decay law,
  BR · (1 − e^(−L/λ)) with λ = p/m · cτ = p[GeV] × 7.07 cm and L = 5.8 m: 48 % at 50–75 GeV,
  17 % at 250–275 GeV; the rest reach the B0 undecayed. But the early decays are reconstructed much
  better (harder π⁻), so the fraction of Λs fully reconstructed stays roughly flat: 15–25 % (old),
  9–14 % (new) up to 275 GeV.
- The reconstructed Λ mass is at the PDG value (1.1157 GeV) with σ ≈ 6–7 MeV; one momentum-ratio
  cut, p(+)/p(−) > 2.5 from the two-body kinematics (true ratio 3.09–14.6), gives 93–97 % purity
  in the new geometry.
- **Not understood:** with the #3116 build, the *truth-seeded* efficiency in the old geometry is
  much higher than with main 8b44cae on the same simulation (proton 97.5 vs 82.2 %, A×ε both
  41.7 vs 29.8 %); the new geometry is unchanged (93.2 vs 92.9 %). Also the +2 MeV mass shift in the
  old geometry.

## Repository layout

```
macros/
  common/LambdaB0.h                         shared code: Λ decays, layers, truth matching, seeding choice, plots
  acceptance/b0_geometric_acceptance.C      geometric acceptance along the Λ direction (geantino)
  acceptance/lambda_daughter_acceptance.C   acceptance of the Λ daughters
  efficiency/lambda_tracking_efficiency.C   tracking efficiency for accepted daughters, step by step
  efficiency/lambda_acceptance_x_efficiency.C  acceptance x efficiency (and the factorization)
  efficiency/lambda_vs_momentum.C           decay before B0, acceptance, efficiency vs Λ momentum, both seedings
  mass/lambda_mass_truth.C                  M(p π⁻) from true momenta (sample check)
  mass/lambda_mass_reco.C                   M(p π⁻) from reconstructed tracks, truth-matched
  mass/lambda_mass_notruth.C                M(p π⁻) without truth in the selection
  mass/lambda_ratio_truth.C                 check of the p(+)/p(−) cut against two-body kinematics
  mass/lambda_mass_seeding_compare.C        M(p π⁻) with truth-seeded vs hit-seeded tracks, overlaid
  dis/lambda_dis.C                          Λ-tagged DIS: spectrum, acceptance, (fiducial) efficiency, A×ε, yield, mass
  exploration/b0_hit_origins.C              which particles make the B0 hits
  exploration/compare_tracks.C              hits, seeds, tracks: file vs file
scripts/
  env.sh                 locations (EIC_TOP, EIC_WORK, EIC_DATA)
  in_eic_shell.sh        run a script inside eic-shell with variables
  sim/ddsim_gun.sh       Λ or geantino gun simulation (GEO, PMIN/PMAX)
  sim/ddsim_hepmc.sh     simulation of a HepMC3 file (GEO, BEAM = 18x275 or 9x130)
  gen/                   Pythia8 NC DIS with a forward Λ: pythia8_dis_lambda.cc, gen_dis_lambda.sh
  reco/eicrecon.sh       reconstruction (RECO = main, container or a local build dir), extra options
  production/            lambda_sample.sh: sim + reco (10 and 5 keV) + copy to EOS (PMIN, PMAX, SIM_ONLY);
                         lambda_rereco.sh: reconstruct an existing sample again; geantino_sample.sh;
                         dis_lambda_sample.sh: DIS gen + sim + reco in both geometries (BEAM);
                         campaign_fetch.sh: copy campaign files from the BNL xrootd server
  condor/                HTCondor submission of a DIS sample (dis_lambda.sub, dis_lambda_job.sh)
  geometry/b0area.py     B0 sensitive area and coverage maps, old vs new, from the B0 tracker xml
  build/                 build epic and EICrecon: a commit (build_epic.sh DIR REF) or a PR (build_eicrecon.sh DIR PR)
  make_results.sh        run every macro, write results/
results/                 plots (png) and printouts (txt) of every macro, by topic (01–10)
docs/report, docs/slides                first report and slides (3 Nov 2026)
docs/report_update, docs/slides_update  update (12 Nov 2026)
notes/                   session logs, commands, summary
```

## Reproducing

On lxplus, with this repository in `~/private/eic/eic-B0-detector` and `eic-shell` in `~/private/eic`
(paths in `scripts/env.sh`). A local build must be built in the same eic-shell image it runs in.

```bash
# software (once)
scripts/build/build_epic.sh                 # epic main -> ~/private/eic/epic/install (new geometry: ec774d8)
scripts/build/build_epic.sh epic_43f427e 43f427e736223b963d02adefcda44bfa225d577b   # old geometry
scripts/build/build_eicrecon.sh             # EICrecon main -> ~/private/eic/EICrecon/install
scripts/build/build_eicrecon.sh EICrecon_pr3116 3116                                  # hit seeding
# samples (stay on the same lxplus node: jobs write to its /tmp, then copy to EOS)
nohup setsid scripts/production/lambda_sample.sh local lambda_new_s101 101 500 > /tmp/$USER/new_s101.log 2>&1 < /dev/null &
scripts/production/lambda_rereco.sh local lambda_new_s101 EICrecon_pr3116 eicreconpr3116
PMIN=100 PMAX=275 SIM_ONLY=1 scripts/production/lambda_sample.sh local lambda_new_s201 201 500 $EIC_DATA/lambda_100to275
scripts/production/lambda_rereco.sh local lambda_new_s201 EICrecon_pr3116 eicreconpr3116 "" $EIC_DATA/lambda_100to275
scripts/production/geantino_sample.sh local geantino_new_5000 5000
# old geometry: GEO=epic_43f427e (GEO=container is the old geometry only for the nightlies up to 2026-11-02)
# Λ-tagged DIS: generation once, then old and new geometry (BEAM=9x130 for 9x130), or on HTCondor
nohup setsid scripts/production/dis_lambda_sample.sh 301 500 > /tmp/$USER/dis_s301.log 2>&1 < /dev/null &
condor_submit -a "queue BEAM,SEED,N,DEST from (9x130 402 500 default)" scripts/condor/dis_lambda.sub
scripts/production/campaign_fetch.sh <campaign RECO folder> 100 $EIC_DATA/campaign/sidis_9x130
# B0 geometry files for results/10 (once): B0_tracker.xml of both commits from GitHub into $EIC_TOP/b0geo
#   (commands in notes/claude_commands.md, 2026-11-19)
# analysis
scripts/make_results.sh                     # all of results/ (or e.g. scripts/make_results.sh 07 08)
# documents
for d in report slides report_update slides_update; do (cd docs/$d && latexmk -pdf $d.tex); done
```

Data (ROOT files, not in git) are in `/eos/user/g/gninio/eic/`, each folder with `logs/`:
- `lambda_100ev/`: old/new, seed 42; reconstructed with the container EICrecon (`containerreco`) and
  with EICrecon main (`eicreconmain`).
- `lambda_1000ev/`: old/new, seeds 101, 102, 50–100 GeV; `eicreconmain` (10 keV),
  `eicreconmain_5keV`, `eicreconpr3116` (#3116 build, truth and hit seeding); `tests/` has the run
  without seed momentum smearing.
- `lambda_100to275/`: old/new, seeds 201, 202, 100–275 GeV; `eicreconpr3116`.
- `geantino/`.
- `lambda_dis/`, `lambda_dis_9x130/`: Λ-tagged DIS, old/new, seeds 301, 302 (18×275) and 401, 402
  (9×130), with the HepMC files; `eicreconpr3116`.
- `campaign/sidis_9x130/`: 100 RECO files of the campaign 26.07.3 SIDIS 9×130 (`files.txt`).

## Pitfalls met

- `eic-shell -- bash -c ...` fails ("Illegal option --"): pipe the script into eic-shell instead
  (`scripts/in_eic_shell.sh`), and run `set --` before sourcing `thisepic.sh`.
- `which` inside the container fails under `set -e`: use `command -v`.
- EOS is not visible inside eic-shell: write to the node's `/tmp`, copy afterwards.
- Parallel jobs in one folder: the geometry downloads `calibrations/` at start-up and a job can read
  a half-written file; give each job its own folder (the scripts do).
- ddsim drops tracker hits below 1 keV by default, i.e. all geantino hits: `--filter.tracker ""`.
- EICrecon parameter keys need the plugin prefix (`tracking:B0TrackerTruthSeeds:MomentumSmear`,
  `B0TRK:B0TrackerRawHits:threshold`); an unknown key stops eicrecon at start-up.
- JANA keeps the **first** value of a repeated `-P` option: `-Pjana:nevents=20` after
  `-Pjana:nevents=-1` is ignored.
- The eic-shell nightly changes under you: `GEO=container` stopped being the old geometry after
  2026-11-02; pin geometries to a commit (`build_epic.sh DIR REF`).
- `pythia8-config` reads the environment variable `OUT`: run it as `env -u OUT pythia8-config ...`.
- Background jobs on lxplus live only as long as the login session on that node; for long
  productions use HTCondor (`scripts/condor/`).
- ePIC campaign reconstruction (26.07.x) predates EICrecon#2901: no B0 tracks for displaced Λ
  daughters; the campaign geometry is the old B0 and there is no 18×275 ep sample.
- Two TEfficiency graphs on one pad: the x axis follows the first one; set it to the histogram range
  (`DrawEff` in `LambdaB0.h`).
