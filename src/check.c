/* 
 * Result files (R.csv, W.csv) checking process.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wunused-result"  // Shutdown unused warnings for `fscanf'.

#define LINE_LENGTH_MAX 60  // Max length of an entry line.
#define R_NUM 59713948      // Number of entries of read.
#define W_NUM 17810657      // Number of entries of write.
#define R_SIZE_CNT 259      // Number of different sizes of read.
#define W_SIZE_CNT 367      // Number of different sizes of write.
#define R_IDX 0             // File index of R.csv.
#define W_IDX 1             // File index of W.csv.

static long unsigned int NUM[2] = {R_NUM, W_NUM};       // Number of entries of read / write.
static unsigned int CNT[2] = {R_SIZE_CNT, W_SIZE_CNT};  // Number of different sizes of read / write.

/* Checking the correctness of outputs. */
int
main (void)
{
  FILE *file[2];

  /* Open two result files. */
  file[R_IDX] = fopen ("output/R.csv", "r");
  file[W_IDX] = fopen ("output/W.csv", "r");

  /* Check each. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    {
      unsigned int size_prev = 0, size;
      long unsigned int size_num = 0, size_num_tmp;
      long unsigned int line_cnt = 1, error_cnt = 0;
      long unsigned int line_cnt_tmp, error_cnt_tmp;
      double time_stamp_prev = .0f, time_stamp;
      char line[LINE_LENGTH_MAX], mode;
      char true_mode = mode_idx == 0 ? 'R' : 'W';

      /* Sorted in ascending order of sizes and time stamps? */
      fscanf (file[mode_idx], "%*s\n");           // Abandon instructive line.
      while (fscanf (file[mode_idx], "%s\n", line) == 1)
        {
          if (strlen (line) < 10 || strcmp (line, "SIZE,COUNT") == 0)   // Met separation line.
            break;

          /* Extract informations. */
          if (line[21] == ',')
            sscanf (line, "%lf,,%c,%*d,%*d,%u", &time_stamp, &mode, &size);
          else
            sscanf (line, "%lf,%*f,%c,%*d,%*d,%u", &time_stamp, &mode, &size);
          line_cnt++;

          /* Mode correct? */
          if (mode != true_mode)
            {
              printf (" %c: Mode of entry %lu is %c instead of %c !\n",
                      true_mode, line_cnt, mode, true_mode);
              error_cnt++;
            }

          /* Check ascending ordering. */
          if (!(size > size_prev || (size == size_prev && time_stamp >= time_stamp_prev)))
            {
              printf (" %c: Entry order violation detected at line %lu !\n",
                      true_mode, line_cnt);
              error_cnt++;
            }

          /* Advance. */
          size_prev = size;
          time_stamp_prev = time_stamp;
        }
      if (error_cnt == 0)
        printf (" %c: Traces have correct mode and in order √\n", true_mode);

      /* Number of entries correct? */
      if (line_cnt - 1 != NUM[mode_idx])
        {
          printf (" %c: Number of entries is %lu instead of %lu !\n",
                  true_mode, line_cnt - 1, NUM[mode_idx]);
          error_cnt++;
        }
      else
        printf (" %c: Number of entries %lu / %lu √\n", true_mode, NUM[mode_idx],
                                                                   NUM[mode_idx]);

      /* Analysis data sorted in ascending order of sizes? */
      line_cnt_tmp = line_cnt;
      error_cnt_tmp = error_cnt;
      line_cnt += 2;
      size_prev = 0;
      while (fscanf (file[mode_idx], "%s\n", line) == 1)
        {
          /* Extract informations. */
          sscanf (line, "%u,%lu", &size, &size_num_tmp);
          size_num += size_num_tmp;
          line_cnt++;

          /* Check ascending ordering. */
          if (!(size > size_prev))
            {
              printf (" %c: Size order violation detected at line %lu !\n",
                      true_mode, line_cnt);
              error_cnt++;
            }

          /* Advance. */
          size_prev = size;
        }
      if (error_cnt == error_cnt_tmp)
        printf (" %c: Size-Count data is in ascending order √\n", true_mode);

      /* Number of different sizes correct? */
      if (line_cnt != 1 + NUM[mode_idx] + 2 + CNT[mode_idx])
        {
          printf (" %c: Number of different sizes is %lu instead of %u !\n",
                  true_mode, line_cnt - NUM[mode_idx] - 3, CNT[mode_idx]);
          error_cnt++;
        }
      else
        printf (" %c: Number of unique item sizes %u / %u √\n", true_mode, CNT[mode_idx],
                                                                           CNT[mode_idx]);

      /* Summed count of analysis data correct? */
      if (size_num != NUM[mode_idx])
        {
          printf (" %c: Sum of all analyzed counts is %lu instead of %lu !\n",
                  true_mode, size_num, NUM[mode_idx]);
          error_cnt++;
        }
      else
        printf (" %c: Sum of Size-Count %lu / %lu √\n", true_mode, NUM[mode_idx],
                                                                   NUM[mode_idx]);

      /* Congratulations! */
      if (error_cnt == 0)
        printf (" %c:        * All tests passed. *\n", mode_idx == 0 ? 'R' : 'W');
    }

  return 0;
}
