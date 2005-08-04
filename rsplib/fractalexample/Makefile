CXXFLAGS = -O0 -g -Wall -DHAVE_CONFIG_H
CPP=g++
MOC=/usr/lib/qt3/bin/moc
#SCTPLIBS=-lsctpsocket -lsctplib
SCTPLIBS=-lsctp

all:   	fractalpooluser


fractalpooluser:	fractalpooluser.o fractalpooluser.moc
	$(CPP) fractalpooluser.o -o fractalpooluser ../netdouble.o  -L/usr/lib/qt3/lib -L/usr/local/lib -L../.libs ../.libs/librsplib.a $(SCTPLIBS) -lglib -lqt-mt

fractalpooluser.o:	fractalpooluser.cc fractalpooluser.moc
	$(CPP) fractalpooluser.cc -c -o fractalpooluser.o $(CXXFLAGS) -I/usr/lib/qt3/include -I.. -I../..

fractalpooluser.moc:	fractalpooluser.cc
	$(MOC) fractalpooluser.cc -o fractalpooluser.moc


mutex.o:	mutex.cc mutex.h
	$(CPP) mutex.cc -c -o mutex.o $(CXXFLAGS)

thread.o:	thread.cc thread.h
	$(CPP) thread.cc -c -o thread.o $(CXXFLAGS)


clean:
	rm -f core.* *.moc *.o fractalpooluser fractalpooluser.moc
