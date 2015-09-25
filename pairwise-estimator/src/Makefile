CC=g++ 
CFLAGS=-O3 -w -c -std=c++11 -fopenmp -Wall

LIBPATH=-L/usr/local/lib/ -L/usr/lib/ -L/home/rajvi/Bundler/lib/ann_1.1_char/lib/ -L../TheiaSfM-master/build/lib/
IFLAGS=-I/usr/local/include/ -I/usr/include/ -I/home/rajvi/PhD/Projects/openMVG-master/src/third_party/eigen/ -I/home/rajvi/Bundler/lib/ann_1.1_char/include/ -I../TheiaSfM-master/libraries/

#LIBPATH=-L/usr/local/lib/ -L/usr/lib/ -L/home/rajvi/Bundler/lib/ann_1.1_char/lib/ 
#IFLAGS=-I/usr/local/include/ -I/usr/include/eigen3/ -I/usr/local/include/theia/libraries/vlfeat -I/home/rajvi/Bundler/lib/ann_1.1_char/include/ 

LIBS=-ltheia -lANN_char -lz -lglog -leasyexif -lceres
#LIBS=-ltheia -lANN_char -lz

theia_matcher: theia_matcher.o Reader.o keys2a.o
	$(CC) $(IFLAGS) theia_matcher.o Reader.o keys2a.o $(LIBPATH) -Wall -o theia_matcher $(LIBS)

theia_matcher.o: theia_matcher.cpp
	$(CC) $(CFLAGS) $(IFLAGS) theia_matcher.cpp 

Reader.o: Reader.cpp Reader.h
	$(CC) $(CFLAGS) $(IFLAGS) Reader.cpp

keys2a.o: keys2a.cpp keys2a.h
	$(CC) $(CFLAGS) $(IFLAGS) keys2a.cpp
