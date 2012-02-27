#ifndef MEEG_HH
#define MEEG_HH
#include <TCanvas.h>

#define SAMPLE_INTERVAL 25.0
void doStats(int n, int nmin, int nmax, int *y, int &count, double &center, double &spread);
void doStats(int n, int nmin, int nmax, short int *y, int &count, double &center, double &spread);
void plotResults(char *name, char *filename, int n, double *x, double *y, TCanvas *canvas);
void plotResults2(char *name, char *name2, char *filename, int n, double *x, double *y, double *y2, TCanvas *canvas);
#endif
