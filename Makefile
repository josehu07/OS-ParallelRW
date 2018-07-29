CC=gcc
INDIR=./src
OUTDIR=./bin
CFLAGS=-O3 -g

all: raw_project opt_project check analyze

raw_project: $(INDIR)/raw_project.c
	$(CC) $(INDIR)/raw_project.c -o $(OUTDIR)/raw_project $(CFLAGS)

opt_project: $(INDIR)/opt_project.c
	$(CC) $(INDIR)/opt_project.c -o $(OUTDIR)/opt_project -fopenmp $(CFLAGS)

check: $(INDIR)/check.c
	$(CC) $(INDIR)/check.c -o $(OUTDIR)/check $(CFLAGS)

analyze: $(INDIR)/analyze.c
	$(CC) $(INDIR)/analyze.c -o $(OUTDIR)/analyze $(CFLAGS)

clean:
	rm -f input/2016* input/*.txt
	rm -f output/* bin/* result/*

clear:
	rm -f input/2016* input/*.txt
	rm -f output/*

sweep:
	rm -f input/*.txt output/*-int.csv
