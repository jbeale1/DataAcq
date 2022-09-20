// Read set of values on STDIN, calculate RMS value
// from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
// compile with
//    gcc -o rms findRMS.c -lm
//
// 20-Sep-2022 J.Beale

#include <stdio.h>
#include <stdlib.h>
#include <math.h>         // sqrt()

#define UI (unsigned int)

int main(void)
{
  char *line = NULL;
  size_t len = 0;
  ssize_t lineSize = 0;

  double sMax = -9E99;
  double sMin = 9E99;
  double datSum = 0;
  double sumsq = 0; // initialize running squared sum of differences
  long   n = 0;    // no data yet
  double mean = 0; // start off with running mean at zero
  double m2 = 0;
  double delta = 0;

  while ( 1 ) {
        lineSize = getline(&line, &len, stdin);  // read line, total chars = lineSize
        if (lineSize < 0) {  // in Linux pipe, end of STDIN returns -1
            break;
        }
        double x = atof(line);  // convert to floating-point value

        datSum += x;
        if (x > sMax) sMax = x;
        if (x < sMin) sMin = x;
        n = n + 1;
        delta = x - mean;
        mean += delta/n;
        m2 += (delta * (x - mean));
  }
  double variance = m2/(n-1);  // (n-1):Sample Variance  (n): Population Variance
  double stdev = sqrt(variance);  // Calculate standard deviation

  printf("Total: %d Mean: %9.3f Stdev: %9.3f ", n, mean, stdev);
  printf("Max: %9.3f  Min: %9.3f \n", n, sMax, sMin);

  free(line);
  return 0;
}
