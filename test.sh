#!/bin/bash

diff <(./build-$1/primes/sieve $2) <(./build-$1/primes/bsp_sieve $2 $3)

