/* 
 * Performance analysis process.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_LENGTH_MAX 200
#define SAMPLE_RATE 10
#define HEAD_WIDTH 16
#define PRINT_WIDTH 12
#define RAW_IDX 0             // File index of raw.
#define OPT_IDX 1             // File index of opt.

#pragma GCC diagnostic ignored "-Wunused-result"  // Shutdown unused warnings for `fscanf'.

/* Analyze the performance of results. */
int
main (void)
{
  FILE *time_file[2], *perf_file[2];
  double timespan[2] = {0};
  double cpu_util[2] = {0}, io_time_perc[2] = {0}, io_bandwidth[2] = {0};
  char line[LINE_LENGTH_MAX];

  /* Open analysis files. */
  time_file[RAW_IDX] = fopen ("result/raw-time", "r");
  time_file[OPT_IDX] = fopen ("result/opt-time", "r");
  perf_file[RAW_IDX] = fopen ("result/raw-perf", "r");
  perf_file[OPT_IDX] = fopen ("result/opt-perf", "r");

  /* Print head. */
  printf (" %*s %*s %*s\n", HEAD_WIDTH, " ", PRINT_WIDTH, "Unoptimized", PRINT_WIDTH, "Optimized");

  /* Calculate and show total timespans. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    {
      double time[5];

      /* Read in time for five sections. */
      fscanf (time_file[mode_idx], " Unzipping source file...finished. Takes %lf secs.\n", &time[0]);
      fscanf (time_file[mode_idx], " Collecting statistics...finished. Takes %lf secs.\n", &time[1]);
      fscanf (time_file[mode_idx], " Abstractively reading...finished. Takes %lf secs.\n", &time[2]);
      fscanf (time_file[mode_idx], " Sorting lines by heap...finished. Takes %lf secs.\n", &time[3]);
      fscanf (time_file[mode_idx], " Writing and attaching...finished. Takes %lf secs.\n", &time[4]);

      /* Calculate total time span. */
      for (int i = 0; i < 5; i++)
        timespan[mode_idx] += time[i];
    }
  printf (" %-*s %*.2lf %*.2lf\n", HEAD_WIDTH, "Timespan (secs)", PRINT_WIDTH, timespan[0], 
                                                                  PRINT_WIDTH, timespan[1]);

  /* Calculate and show cpu-utilization and io-bandwidth. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    {
      double cpu_util_tmp, io_time_perc_tmp, r_bandwidth_tmp, w_bandwidth_tmp;
      int sections = timespan[mode_idx] / SAMPLE_RATE;

      /* Skip first output (from machine start). */
      for (int i = 0; i < 5; i++)
        fgets (line, LINE_LENGTH_MAX, perf_file[mode_idx]);
      do
        fgets (line, LINE_LENGTH_MAX, perf_file[mode_idx]);
      while (strcmp (line, "") != 0 && strcmp (line, "\n") != 0);

      /* Read in informations for necessary sections. */
      for (int i = 0; i < sections; i++)
        {
          /* Skip lines. */
          fgets (line, LINE_LENGTH_MAX, perf_file[mode_idx]);

          /* CPU utilization rate. */
          fscanf (perf_file[mode_idx], "%lf", &cpu_util_tmp);
          cpu_util[mode_idx] += cpu_util_tmp;

          /* Skip lines. */
          for (int j = 0; j < 3; j++)
            fgets (line, LINE_LENGTH_MAX, perf_file[mode_idx]);

          /* IO informations. */
          fscanf (perf_file[mode_idx], "%*s %*f %*f %*f %*f %lf %lf %*f %*f %*f %*f %*f %*f %lf",
                  &r_bandwidth_tmp, &w_bandwidth_tmp, &io_time_perc_tmp);
          io_time_perc[mode_idx] += io_time_perc_tmp;
          io_bandwidth[mode_idx] += r_bandwidth_tmp + w_bandwidth_tmp;

          /* Skip lines. */
          do
            fgets (line, LINE_LENGTH_MAX, perf_file[mode_idx]);
          while (strcmp (line, "") != 0 && strcmp (line, "\n") != 0);
        }

      /* Calculate statistics. */
      cpu_util[mode_idx] /= sections;
      io_time_perc[mode_idx] /= sections;
      io_bandwidth[mode_idx] /= sections;
    }
  printf (" %-*s %*.3lf %*.3lf\n", HEAD_WIDTH, "% CPU Util Rate", PRINT_WIDTH, cpu_util[0],
                                                                  PRINT_WIDTH, cpu_util[1]);
  printf (" %-*s %*.3lf %*.3lf\n", HEAD_WIDTH, "% I/O Time Used", PRINT_WIDTH, io_time_perc[0],
                                                                  PRINT_WIDTH, io_time_perc[1]);
  printf (" %-*s %*.1lf %*.1lf\n", HEAD_WIDTH, "Bandwidth (kB/s)", PRINT_WIDTH, io_bandwidth[0],
                                                                   PRINT_WIDTH, io_bandwidth[1]);

  return 0; 
}
