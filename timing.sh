#!/bin/bash

# The programs to test, on cartesius: prefix "mpirun -np 4"
single=./build-Release/primes/sieve
bsp=./build-Release/primes/bsp_sieve

# The interesting fields/output, the n's to test, number of processors to test
fields="sieving real"
ns="100000 1000000 10000000 20000000 30000000 40000000 50000000 75000000 100000000 125000000 150000000 175000000 200000000 225000000 250000000 275000000 300000000 325000000 350000000"
procs="1 2 3 4"

header=n
for p in single $procs; do
	for f in $fields; do
		header+="\t$p $f"
	done
done
echo -e $header

grepc=`echo $fields | sed 's/ /\\\|/g'`
sedc=`echo "s/$fields/	/g" | sed 's/ /\/	\/g\; s\//g'`

for i in $ns
do
	(echo $i
	(time -p $single $i) 2>&1 | grep $grepc
	(for p in $procs; do time -p $bsp $i $p; done) 2>&1 | grep $grepc
	) | sed "$sedc" | tr '\n' ' '
	echo ""
done

