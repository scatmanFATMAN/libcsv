app_s=libcsv.so
app_a=libcsv.a
obj=csv.o

cc=gcc
cflags=-fpic -Wall -g

all: $(app_s) $(app_a)

release: cflags:=$(filter-out -g, $(cflags))
release: $(app_s) $(app_a)

$(app_s): $(obj)
	$(cc) -o $@ -shared $^

$(app_a): $(obj)
	ar rcs $@ $^

%.o: %.c
	$(cc) -o $@ -c $< $(cflags)

install: release
	cp $(app_s) /usr/local/lib
	cp csv.h /usr/local/include

clean:
	rm -f $(app_s) $(app_a) $(obj)
