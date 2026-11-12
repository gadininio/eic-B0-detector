# Λ → p π⁻ in the ePIC B0 tracker

Study of how well the ePIC far-forward **B0 tracker** reconstructs Λ hyperons through
Λ → p π⁻, with a single-Λ particle gun: geometric acceptance, acceptance of the Λ daughters,
tracking efficiency, acceptance × efficiency, and the reconstructed Λ mass, for the B0 geometry
before and after [eic/epic#1194](https://github.com/eic/epic/pull/1194); with truth seeding and
with the hit (realistic) seeding of [EICrecon#3116](https://github.com/eic/EICrecon/pull/3116),
and for Λ momenta from 50 to 275 GeV.

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

## Definitions

| Quantity | Definition | Macro |
|---|---|---|
| Λ → p π⁻ decay | generated Λ (generatorStatus 1) with exactly two daughters, p and π⁻. (A Λ that interacts in material is also the parent of its products, but has more daughters.) | `macros/common/LambdaB0.h` |
| **Geometric acceptance** (Λ direction) | fraction of **straight lines** from the IP at the Λ gun angles crossing ≥ 3 B0 layers (geantino gun). Pure detector coverage; *not* the acceptance for Λs, which are neutral and leave no hits | `macros/acceptance/b0_geometric_acceptance.C` |
| **Daughter acceptance** | (daughters whose own simulated B0 hits cover ≥ 3 layers) / (Λ → p π⁻ decays); for p, π⁻ and both. Full simulation (decay point, B0 dipole field, material), no digitization or tracking. Not a fiducial volume: a daughter must really leave hits in ≥ 3 layers | `macros/acceptance/lambda_daughter_acceptance.C` |
| **Tracking efficiency** | (accepted daughters with a reconstructed particle) / (accepted daughters). The geometry cancels. Includes digitization (B0 threshold), seeding, CKF, track filtering | `macros/efficiency/lambda_tracking_efficiency.C` |
| **Acceptance × efficiency** | (daughters with a reconstructed particle) / (Λ → p π⁻ decays) = acceptance × tracking efficiency | `macros/efficiency/lambda_acceptance_x_efficiency.C` |
| Per generated Λ | the same, divided by all generated Λs. The unbiased denominator: a Λ that has not decayed by ~6 m nearly always interacts in material before it can decay, so the Λ → p π⁻ decays are mostly early ones (90 % before the B0 at 50–100 GeV) | `macros/efficiency/lambda_vs_momentum.C` |
| Reconstructed daughter | a reconstructed charged particle whose best truth match (highest association weight) is the daughter; `ReconstructedTruthSeededChargedParticles` (truth seeding, default) or `ReconstructedChargedParticles` (hit seeding, macro argument `"hit"`) | `LambdaB0.h` (`lb0::Seeding`) |
| Seed of a daughter | truth seeding: a `B0TrackerTruthSeedParameters` entry with its charge and direction; hit seeding: a `B0TrackerSeeds` triplet with ≥ 2 of its 3 hits made by the daughter | `lambda_tracking_efficiency.C` |
| B0 layers | hits grouped in z; a gap > 50 mm starts a new layer (4 layers / stations at z ≈ 5.8–6.7 m) | `LambdaB0.h` |
| Angles | w.r.t. the proton beam (the 25 mrad crossing-angle rotation undone) | `LambdaB0.h` |
| Threshold | EICrecon digitization: energy per pixel must exceed it (`B0TRK:B0TrackerRawHits:threshold`; 10 keV in main 8b44cae, **5 keV default since EICrecon#3084**, merged 2026-11-03) | `scripts/reco/eicrecon.sh` |
| Purity / cut efficiency | (signal candidates / selected candidates); (signal kept by the cut / signal before it) | `macros/mass/lambda_mass_notruth.C` |

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
  exploration/b0_hit_origins.C              which particles make the B0 hits
  exploration/compare_tracks.C              hits, seeds, tracks: file vs file
scripts/
  env.sh                 locations (EIC_TOP, EIC_WORK, EIC_DATA)
  in_eic_shell.sh        run a script inside eic-shell with variables
  sim/ddsim_gun.sh       Λ or geantino gun simulation (GEO, PMIN/PMAX)
  reco/eicrecon.sh       reconstruction (RECO = main, container or a local build dir), extra options
  production/            lambda_sample.sh: sim + reco (10 and 5 keV) + copy to EOS (PMIN, PMAX, SIM_ONLY);
                         lambda_rereco.sh: reconstruct an existing sample again; geantino_sample.sh
  build/                 build epic and EICrecon: a commit (build_epic.sh DIR REF) or a PR (build_eicrecon.sh DIR PR)
  make_results.sh        run every macro, write results/
results/                 plots (png) and printouts (txt) of every macro, by topic (01–08)
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
- Two TEfficiency graphs on one pad: the x axis follows the first one; set it to the histogram range
  (`DrawEff` in `LambdaB0.h`).
