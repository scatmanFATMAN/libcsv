app=csv_test
obj=csv_test.o

cc=gcc

cflags=-I../../lib -Wall -g
libs=-L../../lib -lcsv

all: $(app)

$(app): $(obj)
	cd ../../lib && make
	$(cc) -o $@ $^ $(libs)

%.o: %.c
	$(cc) -o $@ -c $< $(cflags)

clean:
	rm -f $(app) $(obj)
