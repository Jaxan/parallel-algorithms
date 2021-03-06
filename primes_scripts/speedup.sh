#!/bin/bash

# The programs to test, on cartesius: prefix "mpirun -np 4"
single=./build-Release/primes/sieve
bsp=./build-Release/primes/bsp_sieve

# The interesting fields/output, the n's to test, number of processors to test
field="sieving"
ns="100000000"
procs="1 2 3 4 5 6 7 8 9 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90 95 100"

for i in $ns; do
	echo "Speedup for $i"
	(time -p $single $i) 2>&1 | grep $field | sed "s/$field/baseline/"
	for p in $procs; do
		(time -p mpirun -np $p $bsp $i $p) 2>&1 | grep $field | sed "s/$field/$p/"
	done
done

