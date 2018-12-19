CC      = gcc
CFLAGS  = -g -Wall
RM      = rm -f


default: all

all: pipeline

signal_exe: pipeline.c
	$(CC) $(CFLAGS) -o pipeline pipeline.c

clean veryclean:
	$(RM) pipeline

