CC=gcc
CFLAGS=-Wall -DDSCP -Wextra -O3 -lm
LDFLAGS=-Wall -DDSCP -Wextra -O3 -lm
EXEC=pss

pss: pscheduler.o queue.o bls.o pss.o
	$(CC) -o bin/$@ $(patsubst %,./bin/%,$(^)) $(LDFLAGS)

bls.o: include/queue.h

pscheduler.o: include/queue.h include/bls.h

pss.o: include/pscheduler.h include/queue.h include/bls.h

%.o: src/%.c include/%.h
	$(CC) -o bin/$@ -I include -c $< $(CFLAGS)

%.o: src/%.c
	$(CC) -o bin/$@ -I include -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf bin/*.o

mrproper: clean
	rm -rf bin/$(EXEC)
