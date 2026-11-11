# Λ → p π⁻ in the ePIC B0 tracker

Study of how well the ePIC far-forward **B0 tracker** reconstructs Λ hyperons through
Λ → p π⁻, with a single-Λ particle gun: geometric acceptance, acceptance of the Λ daughters,
tracking efficiency, acceptance × efficiency, and the reconstructed Λ mass, for the B0 geometry
before and after [eic/epic#1194](https://github.com/eic/epic/pull/1194).

- Detailed write-up: [`docs/report/report.pdf`](docs/report/report.pdf)
- Slides: [`docs/slides/slides.pdf`](docs/slides/slides.pdf)
- Short summary of the work: [`notes/claude_summary.md`](notes/claude_summary.md); full session log:
  [`notes/claude_log.md`](notes/claude_log.md), all commands: [`notes/claude_commands.md`](notes/claude_commands.md)

## Setup

| | |
|---|---|
| Sample | Λ gun, p = 50–100 GeV, θ = 8–18 mrad (uniform), φ uniform, 25 mrad crossing angle (`--crossingAngleBoost -0.025`), 18×275 GeV craterlake geometry |
| Simulation | `ddsim` (DD4hep/Geant4) in eic-shell, `Geant4TVUserParticleHandler` |
| Old geometry | epic `43f427e` (eic-shell nightly of 2026-11-02): B0 sensors 300 µm |
| New geometry | epic main `ec774d8` = merge of eic/epic#1194 (realistic B0): B0 sensors 50 µm, built locally |
| Reconstruction | EICrecon main `8b44cae` (built locally); contains EICrecon#2901 (B0 truth seeding for displaced vertices). The nightly EICrecon `a7bab3f` has only 4 B0 tracks per 100 Λ |
| Tracking | ACTS CKF with **truth seeding** (`B0TrackerTruthSeeds`); realistic B0 seeding is EICrecon#3028 (open) |
| Samples | 100 events per geometry (seed 42), 1000 events per geometry (seeds 101, 102 × 500), 5000 geantinos per geometry |

## Definitions

| Quantity | Definition | Macro |
|---|---|---|
| Λ → p π⁻ decay | generated Λ (generatorStatus 1) with exactly two daughters, p and π⁻. (A Λ that interacts in material is also the parent of its products, but has more daughters.) | `macros/common/LambdaB0.h` |
| **Geometric acceptance** (Λ direction) | fraction of **straight lines** from the IP at the Λ gun angles crossing ≥ 3 B0 layers (geantino gun). Pure detector coverage; *not* the acceptance for Λs, which are neutral and leave no hits | `macros/acceptance/b0_geometric_acceptance.C` |
| **Daughter acceptance** | (daughters whose own simulated B0 hits cover ≥ 3 layers) / (Λ → p π⁻ decays); for p, π⁻ and both. Full simulation (decay point, B0 dipole field, material), no digitization or tracking | `macros/acceptance/lambda_daughter_acceptance.C` |
| **Tracking efficiency** | (accepted daughters with a reconstructed particle) / (accepted daughters). The geometry cancels. Includes digitization (B0 threshold), seeding, CKF, track filtering | `macros/efficiency/lambda_tracking_efficiency.C` |
| **Acceptance × efficiency** | (daughters with a reconstructed particle) / (Λ → p π⁻ decays) = acceptance × tracking efficiency | `macros/efficiency/lambda_acceptance_x_efficiency.C` |
| Reconstructed daughter | a `ReconstructedTruthSeededChargedParticle` whose best truth match (highest weight in `ReconstructedTruthSeededChargedParticleAssociations`) is the daughter | `LambdaB0.h` |
| B0 layers | hits grouped in z; a gap > 50 mm starts a new layer (4 layers, ~300–400 mm apart) | `LambdaB0.h` |
| Angles | w.r.t. the proton beam (the 25 mrad crossing-angle rotation undone) | `LambdaB0.h` |
| Threshold | EICrecon digitization: energy per pixel must exceed it (`B0TRK:B0TrackerRawHits:threshold`, default 10 keV; 5 keV suits the 50 µm sensors) | `scripts/reco/eicrecon.sh` |
| Purity / cut efficiency | (signal candidates / selected candidates); (signal kept by the cut / signal before it) | `macros/mass/lambda_mass_notruth.C` |

## Main results (1000 Λ per geometry)

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

Main findings
- EICrecon#2901 is essential: with the nightly EICrecon (a7bab3f) the B0 truth seeder only seeds
  particles from |z| < 200 mm, giving 4 B0 tracks per 100 Λ; with main, 77 (old) / 44 (new geometry).
- The new geometry (eic/epic#1194) has a gap of about ±45° in φ on the electron-beam side
  (geometric acceptance 95 % → 70 %).
- Its 50 µm sensors give ≈ 14 keV per hit (most probable), so the 10 keV threshold loses 9 % of
  hits and 7 % of accepted daughters; 5 keV recovers them (not yet the EICrecon default).
- The π⁻ tracking efficiency is low because it is soft (3–25 GeV vs 38–94 GeV for the proton):
  below 10 GeV only 12–36 % of accepted π⁻ get a track; the loss is in the CKF, not in seeding
  (removing the 10 % seed momentum smearing changes nothing). Cause not yet identified.
- The reconstructed Λ mass is at the PDG value (1.1157 GeV) with σ ≈ 6–7 MeV; one momentum-ratio
  cut, p(+)/p(−) > 2.5 from the two-body kinematics (true ratio 3.09–14.6), gives ≈ 97 % purity.

## Repository layout

```
macros/
  common/LambdaB0.h                         shared code: Λ decays, layers, truth matching, plots
  acceptance/b0_geometric_acceptance.C      geometric acceptance along the Λ direction (geantino)
  acceptance/lambda_daughter_acceptance.C   acceptance of the Λ daughters
  efficiency/lambda_tracking_efficiency.C   tracking efficiency for accepted daughters, step by step
  efficiency/lambda_acceptance_x_efficiency.C  acceptance x efficiency (and the factorization)
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
  sim/ddsim_gun.sh       Λ or geantino gun simulation
  reco/eicrecon.sh       reconstruction (EICrecon main or container), extra options
  production/            one complete sample: sim + reco (10 and 5 keV) + copy to EOS
  build/                 build epic main and EICrecon main
  make_results.sh        run every macro, write results/
results/                 plots (png) and printouts (txt) of every macro, by topic
docs/report, docs/slides LaTeX sources and PDFs
notes/                   session logs, commands, the author's notes
```

## Reproducing

On lxplus, with this repository in `~/private/eic/eic-B0-detector` and `eic-shell` in `~/private/eic`
(paths in `scripts/env.sh`):

```bash
# software (once)
scripts/build/build_epic.sh                 # epic main -> ~/private/eic/epic/install
scripts/build/build_eicrecon.sh             # EICrecon main -> ~/private/eic/EICrecon/install
# samples (stay on the same lxplus node: jobs write to its /tmp, then copy to EOS)
nohup setsid scripts/production/lambda_sample.sh local lambda_new_s101 101 500 > /tmp/$USER/new_s101.log 2>&1 < /dev/null &
scripts/production/geantino_sample.sh local geantino_new_5000 5000
# GEO=container is the old geometry only while the eic-shell nightly still has epic 43f427e
# analysis
scripts/make_results.sh                     # all of results/ (or e.g. scripts/make_results.sh 04 05)
# documents
cd docs/report && latexmk -pdf report.tex; cd ../slides && latexmk -pdf slides.tex
```

Data (ROOT files, not in git) are in `/eos/user/g/gninio/eic/`:
`lambda_100ev/` (old/new, seed 42; reconstructed with the container EICrecon `containerreco` and
with EICrecon main `eicreconmain`), `lambda_1000ev/` (old/new, seeds 101, 102; `eicreconmain`,
`eicreconmain_5keV`; `tests/` has the run without seed momentum smearing), `geantino/`, each with `logs/`.

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
- Two TEfficiency graphs on one pad: the x axis follows the first one; set it to the histogram range
  (`DrawEff` in `LambdaB0.h`).
