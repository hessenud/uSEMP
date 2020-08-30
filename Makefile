# Implicit command of: "cc blah.o -o blah"
# Note: Do not put a comment inside of the blah.o rule; the implicit rule will not run!
uSEMP.a:

# Implicit command of: "cc -c blah.c -o blah.o"
uSEMP.o:  uSEMP.cpp
	gcc uSEMP.cpp -o uSEMP.o

objects := $(wildcard *.o) # Right

all: uSEMP.o $(objects)
	ar uSEMP.a $(objects)