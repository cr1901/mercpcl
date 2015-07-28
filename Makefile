# User variables- do not assume ld.so.conf exists


OBJECTS=mercpcl.o
CPPFLAGS=-I/usr/pkg/include
# CFLAGS=-O0 -g
CFLAGS=-O2
LDFLAGS=-Wl,-rpath=/usr/pkg/lib -L/usr/pkg/lib
LIBS=-lftdi -lusb-1.0

.c.o:
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

mercpcl: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

clean:
	rm -rf mercpcl *.o

install: mercpcl
	install -c mercpcl /usr/local/bin

test: mercpcl test.sh
	./test.sh
