CC=cc
COMPILE_FLAGS=
LD_FLAGS=

mp4recover: sps.o mp4recover.o
	$(CC) $(LD_FLAGS) sps.o mp4recover.o -o mp4recover
	
sps.o: sps.c
	$(CC) $(COMPILE_FLAGS) -c sps.c

mp4recover.o: mp4recover.c
	$(CC) $(COMPILE_FLAGS) -c mp4recover.c

install:
	cp mp4recover /usr/local/sbin/mp4recover

clean:
	rm -f *.o
