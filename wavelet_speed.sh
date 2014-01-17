#!/bin/bash
#SBATCH -t 0:30:00
#SBATCH -n 4

p=2
start=6
end=27
iters=200

if [[ `whoami` == "bissstud" ]]; then
	cd $HOME/Students13/JoshuaMoerman/assignments
	echo "Running on Cartesius $@"
	RUNCOMMAND="srun"
else
	echo "Running locally $@"
	RUNCOMMAND=""
fi

for i in `seq $start $end`; do
	echo -e "\n\033[1;34mtime\t`date`\033[0;39m"
	let "n=2**$i"
	$RUNCOMMAND ./build-Release/wavelet/wavelet_parallel_mockup --m 1 --n $n --p $p --show-input --iterations $iters
done
echo -e "\n\033[1;31mtime\t`date`\033[0;39m"
