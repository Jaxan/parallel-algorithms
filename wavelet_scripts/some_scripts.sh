(for i in $(seq 5 26); do echo -e "#define NEXP $i\n" > ../wavelet/defines.hpp && make -B -j4 mockup && echo $i && ./wavelet/mockup || break; done) | grep "parallel\|sequential"
(for i in $(seq 5 26); do echo -e "#define NEXP $i\n" > ../wavelet/defines.hpp && make -B -j4 mockup && echo $i && mpirun -np 4 ./wavelet/mockup || break; done) | grep "parallel\|sequential"

for i in $(seq 1 21); do echo 2 | ./bench | grep "r="; done
for i in $(seq 1 21); do echo 4 | mpirun -np 4 ./bench | grep "r="; done