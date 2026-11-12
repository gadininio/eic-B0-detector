#!/bin/bash
# dis_lambda_job.sh: HTCondor wrapper of scripts/production/dis_lambda_sample.sh (see dis_lambda.sub).
#   dis_lambda_job.sh <BEAM> <SEED> <N> [DEST|default]
# Works in the job's scratch folder (bound into the container as the current directory).
set -e
BEAM=$1; SEED=$2; N=$3; DEST=$4
[ "$DEST" = default ] && DEST=
export EIC_WORK=$PWD/eic_work
REPO=/afs/cern.ch/user/g/gninio/private/eic/eic-B0-detector
echo "host $(hostname), scratch $PWD, beam $BEAM seed $SEED N $N"
BEAM=$BEAM "$REPO/scripts/production/dis_lambda_sample.sh" "$SEED" "$N" $DEST
