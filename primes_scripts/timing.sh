#!/bin/bash

# The programs to test, on cartesius: prefix "mpirun -np $p"
single=./build-Release/primes/sieve
bsp=./build-Release/primes/bsp_sieve

# The interesting fields/output, the n's to test, number of processors to test
fields="real"
# ns="100000 1000000 5000000 10000000 15000000 20000000 25000000 30000000 35000000 40000000 45000000 50000000 55000000 60000000 65000000 70000000 75000000 80000000 85000000 90000000 95000000 100000000"
ns="100000 200000 300000 400000 500000 600000 700000 800000 900000 1000000 1100000 1200000 1300000 1400000 1500000"
# procs="1 2 3 4 5 10 20 30 40 50 100"
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

# for i in 1 2 3 4 5 6 7 8 9 10 20 30 40 50 60 70 80 90 100; do time mpirun -np $i ./build-Release/primes/bsp_sieve 1000000 $i > /dev/null ; done
