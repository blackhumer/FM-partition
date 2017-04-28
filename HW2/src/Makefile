CC = g++
src = FM_Partitioner.cpp
output = ../bin/FM_Partitioner
bin = ./../bin/FM_Partitioner
test = ../testcases/
solution = ../output/FM_Partitioner_solution
verifier = .././verifier/verify

all:src

src:
	$(CC) -g $(src) -o $(output)
case1:
	time $(bin) -n $(test)p2-1.nets -c $(test)p2-1.cells
case2:
	time $(bin) -n $(test)p2-2.nets -c $(test)p2-2.cells
case3:
	time $(bin) -n $(test)p2-3.nets -c $(test)p2-3.cells
verify1:
	$(verifier)  $(test)p2-1.nets $(test)p2-1.cells $(solution)1
verify2:
	$(verifier)  $(test)p2-2.nets $(test)p2-2.cells $(solution)2
verify3:
	$(verifier)  $(test)p2-3.nets $(test)p2-3.cells $(solution)3
clean:
	rm ../bin/FM_Partitioner
