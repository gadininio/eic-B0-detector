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
# ROOT outside eic-shell (for the analysis macros)
export EIC_ROOT_SETUP=${EIC_ROOT_SETUP:-/cvmfs/sft.cern.ch/lcg/views/LCG_107/x86_64-el9-gcc13-opt/setup.sh}
