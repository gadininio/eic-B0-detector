# env.sh: locations used by the scripts. Source it, or override any variable before.
#
#   EIC_TOP   folder with the software builds and the eic-shell launcher:
#               $EIC_TOP/eic-shell, $EIC_TOP/epic/install (local epic build),
#               $EIC_TOP/EICrecon/install (local EICrecon build)
#   EIC_WORK  where jobs write while running. EOS is NOT visible inside eic-shell, so jobs
#             write to the local disk of the lxplus node (stay on the same node until the
#             files are copied!) and are copied to EIC_DATA afterwards.
#   EIC_DATA  final storage of the ROOT files (EOS).
#   EIC_REPO  this repository.
export EIC_TOP=${EIC_TOP:-$HOME/private/eic}
export EIC_WORK=${EIC_WORK:-/tmp/$USER/eic_work}
export EIC_DATA=${EIC_DATA:-/eos/user/${USER:0:1}/$USER/eic}
export EIC_REPO=${EIC_REPO:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
# Container image for eic-shell. The CVMFS "nightly" changes every day and broke the local builds
# (2026-11-25: EICrecon_pr3116 could not load libalgocore.so.1.0), so it is pinned to the nightly
# of 2026-11-24 (eic_xl:nightly at that date; EICrecon main ff59ecf inside). Builds made in it:
# EICrecon_main_1125, epic_43f427e_n1124, epic_ec774d8_n1124. Unset SIF to use the current nightly.
export SIF=${SIF:-/cvmfs/singularity.opensciencegrid.org/.images/c1/0bb172faa0530ae962270aaf50ccd2e8baa7e97b832e5785460a70dcd03e00}
# ROOT outside eic-shell (for the analysis macros)
export EIC_ROOT_SETUP=${EIC_ROOT_SETUP:-/cvmfs/sft.cern.ch/lcg/views/LCG_107/x86_64-el9-gcc13-opt/setup.sh}
