SDIR=libxml2test
CFLAGS=-std=c++14 -g -I/usr/include/libxml2
LIBS=-lxml2
CC=g++
TT=./data/small_timetable.xml

parse_timetable: $(SDIR)/libxml2test.o
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

$(SDIR)/libxml2test.o: $(SDIR)/libxml2test.cpp $(SDIR)/stdafx.h.gch Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

$(SDIR)/stdafx.h.gch: $(SDIR)/stdafx.h Makefile
	$(CC) $(CFLAGS) -o $@ $< 

.PHONY: clean

clean:
	rm parse_timetable $(SDIR)/*.o $(SDIR)/*.gch

test: parse_timetable
	time ./parse_timetable $(TT) > connections.txt
