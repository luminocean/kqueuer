all: kqueue.so

kqueue.so: kqueue.o
	gcc kqueue.o -shared -o kqueue.so

kqueue.o: kqueue.c
	gcc -c -fPIC kqueue.c -o kqueue.o

.PHONY: clean

clean:
	rm *.o *.so