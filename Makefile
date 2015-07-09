CFLAGS = -g -Wall
CC = gcc

all: genfile dumpfile msort

genfile: genfile.c util.h errors.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o genfile genfile.c

dumpfile: dumpfile.c util.h errors.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o dumpfile dumpfile.c

msort: msort.c util.h errors.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o msort msort.c -lm

tests: all input1G.bin input5G.bin
	./run_test.sh 5G 1G 5
	./run_test.sh 5G 1G 10
	./run_test.sh 5G 1G 25
	./run_test.sh 5G 1G 50

	./run_test.sh 5G 2G 5
	./run_test.sh 5G 2G 10
	./run_test.sh 5G 2G 25
	./run_test.sh 5G 2G 50

input1G.bin: genfile
	./genfile -s 1G -o input1G.bin
input5G.bin: genfile
	./genfile -s 5G -o input5G.bin

clean:
	rm -f *.o msort genfile dumpfile *.bin *.txt
