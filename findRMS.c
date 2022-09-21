// Read a set of values (one per line) from STDIN, find min/max/avg/std
// from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
//
// compile with
//    gcc -o findRMS findRMS.c -lm
//
// usage example:
//    $ cat numbers.txt | ./findRMS
//    Count: 1000 Mean: 796113.554 Stdev:     4.674 Min: 796099.00  Max: 796131.00
//
// 21-Sep-2022 J.Beale

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

  printf("Count: %ld Mean: %9.3f Stdev: %9.3f ", n, mean, stdev);
  printf("Min: %9.2f  Max: %9.2f \n", sMin, sMax);

  free(line);
  return 0;
}
