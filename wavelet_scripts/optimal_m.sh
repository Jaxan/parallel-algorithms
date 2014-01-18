#!/bin/bash
#SBATCH -t 0:30:00
#SBATCH -n 4

p=4
n=32768
mstart=1
mend=12
iters=100

if [[ `whoami` == "bissstud" ]]; then
	cd $HOME/Students13/JoshuaMoerman/assignments
	echo "Running on Cartesius $@"
	RUNCOMMAND="srun"
else
	echo "Running locally $@"
	RUNCOMMAND=""
fi

for m in `seq $mstart $mend`; do
	echo -e "\n\033[1;34mtime\t`date`\033[0;39m"
	$RUNCOMMAND ./build-Release/wavelet/wavelet_parallel_mockup --m $m --n $n --p $p --show-input --iterations $iters
done
echo -e "\n\033[1;31mtime\t`date`\033[0;39m"
