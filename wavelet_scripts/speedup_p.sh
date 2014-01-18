#!/bin/bash
#SBATCH -t 0:30:00
#SBATCH -n 64

m=5
n=1048576
pstart=1
pend=6
iters=100

if [[ `whoami` == "bissstud" ]]; then
	cd $HOME/Students13/JoshuaMoerman/assignments
	echo "Running on Cartesius $@"
	RUNCOMMAND="srun"
else
	echo "Running locally $@"
	RUNCOMMAND=""
fi

for i in `seq $pstart $pend`; do
	echo -e "\n\033[1;34mtime\t`date`\033[0;39m"
	let "p=2**$i"
	$RUNCOMMAND ./build-Release/wavelet/wavelet_parallel_mockup --m $m --n $n --p $p --show-input --iterations $iters
done
echo -e "\n\033[1;31mtime\t`date`\033[0;39m"
