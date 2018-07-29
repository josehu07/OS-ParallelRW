/* 
 * Raw (unoptimized) project running-through.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <sys/time.h>

#pragma GCC diagnostic ignored "-Wunused-result"

/* Predefined constants. */
#define NAME_LENGTH_MAX 35    // Max length of command component.
#define LINE_LENGTH_MAX 60    // Max length of an entry line.
#define R_IDX 0               // File index of R.csv.
#define W_IDX 1               // File index of W.csv.
#define FILE_NUM 32           // Number of source files.
#define INST_LINE_LENGTH 42   // Length of first line (Timestamp,...).
#define SIZE_MAX 600000       // Upper bound of size.

/* Scanned statistics. */
static long unsigned int NUM[2] = {0};                // Number of entries of read / write.
static unsigned int CNT[2] = {0};                     // Number of different sizes of read / write.

/* Type definitions. */
typedef void (*PROCESS) (void);   // Type of process handler function.
typedef struct                    // Type of an entry node.
  {
    long unsigned int line_idx;
    unsigned int size;
    double time_stamp;
  } node;
typedef struct                    // Type of size count slot.
  {
    unsigned int size;
    long unsigned int cnt; 
  } cnt_struct;

/* Subroutine definitions. */
void decompress (void);
void scanStatistics (void);
void abstractRead (void);
void sortEntries (void);
void writeResult (void);
static void runProcess (char *name, PROCESS func);
static void heapify (node *arr, long unsigned int len, long unsigned int pivot);
static inline void swap (node *a, node *b);
static inline int larger (node *a, node *b);

/* Global variables or containers. */
static unsigned int LUN_idx_arr[6] = {0, 1, 2, 3, 4, 6};  // LUN indexes.
static FILE *global_src_file[FILE_NUM];                   // Source file pointers.
static cnt_struct *size_cnt_arr[2];                       // Array of size count data.
static node *node_arr[2];                                 // Huge node arrays.

/* Main function for analysing. */
int
main (void)
{
  char file_name[NAME_LENGTH_MAX];
  int file_idx = 0;

  /* Unzip to get source files. */
  runProcess ("Unzipping source file", decompress);

  /* Globally open source files. */
  for (int date = 7; date <= 12; date++)
    for (int id = 0; id < (date == 12 ? 2 : 6); id++)
      {
        sprintf (file_name, "input/20160222%02d-LUN%d.csv", date, LUN_idx_arr[id]);
        global_src_file[file_idx] = fopen (file_name, "r");
        file_idx++;
      }

  /* Collect necessary statistics. */
  runProcess ("Collecting statistics", scanStatistics);

  /* Allocate memory space for huge node arrays. */
  node_arr[R_IDX] = malloc (sizeof (node) * (NUM[R_IDX] + 1));
  node_arr[W_IDX] = malloc (sizeof (node) * (NUM[W_IDX] + 1));
  size_cnt_arr[R_IDX] = malloc (sizeof (cnt_struct) * CNT[R_IDX]);
  size_cnt_arr[W_IDX] = malloc (sizeof (cnt_struct) * CNT[W_IDX]);

  /* Initialize size count array. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    for (int i = 0; i < CNT[mode_idx]; i++)
      {
        size_cnt_arr[mode_idx][i].size = 0;
        size_cnt_arr[mode_idx][i].cnt = 0;
      }

  /* Read -> Sort -> Write processes. */
  runProcess ("Abstractively reading", abstractRead);
  runProcess ("Sorting lines by heap", sortEntries);
  runProcess ("Writing and attaching", writeResult);

  /* Release memory spaces. */
  free (node_arr[R_IDX]);
  free (node_arr[W_IDX]);
  free (size_cnt_arr[R_IDX]);
  free (size_cnt_arr[W_IDX]);

  /* Close globally opened files. */
  for (int i = 0; i < FILE_NUM; i++)
    fclose (global_src_file[i]);

  return 0;
}

/* Decompression process handler. */
void
decompress (void)
{
  char tar_file[NAME_LENGTH_MAX] = "input/systor17-01.tar";   // `tar' file.
  char gz_file[NAME_LENGTH_MAX];                              // `.csv.gz' files.

  /* Untar the source file. */
  if (fork () == 0)
    execl ("/bin/tar", "tar", "-xf", tar_file, "-C", "input", NULL);
  else
    wait (NULL);

  /* Use separate processes for unzipping different files, and wait sequentially. */
  for (int date = 7; date <= 12; date++)
    for (int id = 0; id < (date == 12 ? 2 : 6); id++)
      {
        sprintf (gz_file, "input/20160222%02d-LUN%d.csv.gz", date, LUN_idx_arr[id]);
        if (fork () == 0)
          execl ("/bin/gunzip", "gunzip", "-qf", gz_file, NULL);
        else
          wait (NULL);
      }
}

/* Statistics collecting process handler. */
void
scanStatistics (void)
{
  int size_mark[2][SIZE_MAX] = {0};

  /* Scan file statistics. */
  for (int i = 0; i < FILE_NUM; i++)
    {
      FILE *src_file = global_src_file[i];
      char line[LINE_LENGTH_MAX], mode;
      unsigned int size, mode_idx;
      double time_stamp;

      /* Scan each trace entry. */
      fscanf (src_file, "%*s\n");     // Abandon the instruction line. 
      while (fscanf (src_file, "%s\n", line) == 1)
        {
          /* Extract informations. */
          if (line[21] == ',')
            sscanf (line, "%lf,,%c,%*d,%*d,%u", &time_stamp, &mode, &size);
          else
            sscanf (line, "%lf,%*f,%c,%*d,%*d,%u", &time_stamp, &mode, &size);
          mode_idx = mode == 'W' ? W_IDX : R_IDX;

          /* Update statistics. */
          size_mark[mode_idx][size] = 1;
          NUM[mode_idx]++;
        }
      fseek (src_file, 0, SEEK_SET);  // Reset source file position.
    }

  /* Acquire number of different sizes. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    for (long unsigned int i = 0; i < SIZE_MAX; i++)
      if (size_mark[mode_idx][i] == 1)
        CNT[mode_idx]++;
}

/* Abstraction read process handler. */
void
abstractRead (void)
{
  FILE *int_file[2], *src_file;
  char file_name[NAME_LENGTH_MAX], line[LINE_LENGTH_MAX], mode;
  unsigned int size, mode_idx;
  long unsigned int entry_cnt[2] = {0};
  double time_stamp;

  /* Create intermediate files. */
  int_file[R_IDX] = fopen ("output/R-int.csv", "w");
  int_file[W_IDX] = fopen ("output/W-int.csv", "w");

  /* Go through all source files. */
  for (int i = 0; i < FILE_NUM; i++)
    {
      FILE *src_file = global_src_file[i];

      /* Scan each trace entry. */
      fscanf (src_file, "%*s\n");     // Abandon the instruction line.
      while (fscanf (src_file, "%s\n", line) == 1)
        {
          /* Extract informations. */
          if (line[21] == ',')
            sscanf (line, "%lf,,%c,%*d,%*d,%u", &time_stamp, &mode, &size);
          else
            sscanf (line, "%lf,%*f,%c,%*d,%*d,%u", &time_stamp, &mode, &size);
          mode_idx = mode == 'W' ? W_IDX : R_IDX;

          /* Fill in an empty slot in corresponding node array. */
          entry_cnt[mode_idx]++;
          node_arr[mode_idx][entry_cnt[mode_idx]].line_idx = entry_cnt[mode_idx] - 1;
          node_arr[mode_idx][entry_cnt[mode_idx]].size = size;
          node_arr[mode_idx][entry_cnt[mode_idx]].time_stamp = time_stamp;

          /* Write into intermediate file. */
          while (strlen (line) < LINE_LENGTH_MAX - 1)
            strcat (line, " ");
          strcat (line, "\n");
          fputs (line, int_file[mode_idx]);
        }
    }

  /* Close opened files. */
  fclose (int_file[R_IDX]);
  fclose (int_file[W_IDX]);
}

/* Entries sorting process handler. */
void
sortEntries (void)
{
  /* For two node arrays, use heap-sort in ascending order. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    {
      long unsigned int heap_size = NUM[mode_idx];

      /* Build heap by Floyd. */
      for (long unsigned int i = NUM[mode_idx] / 2 + 1; i > 0; i--)
        heapify (node_arr[mode_idx], heap_size, i);

      /* Iteratively extract heap top and place at tail. */
      for (long unsigned int i = NUM[mode_idx]; i > 1; i--)
        {
          swap (&node_arr[mode_idx][1], &node_arr[mode_idx][i]);
          heap_size--;
          heapify (node_arr[mode_idx], heap_size, 1);
        }
    }

  /* Calculate size counts data in sorted order. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    {
      for (long unsigned int i = 1; i <= NUM[mode_idx]; i++)
        for (unsigned int j = 0; j < CNT[mode_idx]; j++)
          {
            if (size_cnt_arr[mode_idx][j].size == node_arr[mode_idx][i].size)   // Already met.
              {
                size_cnt_arr[mode_idx][j].cnt++;
                break;
              }
            else if (size_cnt_arr[mode_idx][j].size == 0)                       // First found.
              {
                size_cnt_arr[mode_idx][j].size = node_arr[mode_idx][i].size;
                size_cnt_arr[mode_idx][j].cnt = 1;
                break;
              }
          }
    }
}

/* Result writing process handler. */
void
writeResult (void)
{
  FILE *int_file[2], *dst_file[2];
  char line[LINE_LENGTH_MAX];

  /* Open intermediate files and create destination files. */
  int_file[R_IDX] = fopen ("output/R-int.csv", "r");
  int_file[W_IDX] = fopen ("output/W-int.csv", "r");
  dst_file[R_IDX] = fopen ("output/R.csv", "w");
  dst_file[W_IDX] = fopen ("output/W.csv", "w");

  /* Loop through all entries in sorted order, write to destination file. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    {
      fprintf (dst_file[mode_idx], "Timestamp,Response,IOType,LUN,Offset,Size\n");
      for (long unsigned int i = 1; i <= NUM[mode_idx]; i++)
        {
          fseek (int_file[mode_idx], node_arr[mode_idx][i].line_idx * LINE_LENGTH_MAX, SEEK_SET);
          fscanf (int_file[mode_idx], "%s\n", line);
          fprintf (dst_file[mode_idx], "%s\n", line);
        }
    }

  /* Write size count data. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    {
      fprintf (dst_file[mode_idx], "\nSIZE,COUNT\n");
      for (unsigned int j = 0; j < CNT[mode_idx]; j++)
        {
          if (size_cnt_arr[mode_idx][j].size == 0)
            break;
          fprintf (dst_file[mode_idx], "%u,%lu\n", size_cnt_arr[mode_idx][j].size,
                                                   size_cnt_arr[mode_idx][j].cnt);
        }
    } 

  /* Close opened files. */
  fclose (int_file[R_IDX]);
  fclose (int_file[W_IDX]);
  fclose (dst_file[R_IDX]);
  fclose (dst_file[W_IDX]);
}

/* Auxiliary function for running a process section. */
static void
runProcess (char *name, PROCESS func)
{
  struct timeval tv_start, tv_end;
  int sec, usec;

  printf (" %21s...", name);
  fflush (stdout);
  gettimeofday (&tv_start, NULL);
  func ();
  gettimeofday (&tv_end, NULL);

  sec = tv_end.tv_sec - tv_start.tv_sec;
  usec = tv_end.tv_usec - tv_start.tv_usec;
  printf ("finished. Takes %2d.%07d secs.\n", sec, usec > 0 ? usec : 1000000 - usec);
}

/* Auxiliary function for heapify. */
static void
heapify (node *arr, long unsigned int len, long unsigned int pivot)
{
  while (1)
    {
      long unsigned int left = 2 * pivot;
      long unsigned int right = 2 * pivot + 1;
      long unsigned int largest = (left <= len && larger (&arr[left], &arr[pivot])) ? left : pivot;

      if (right <= len && larger (&arr[right], &arr[largest]))
        largest = right;
      if (largest == pivot)
        break;
      swap (&arr[pivot], &arr[largest]);
      pivot = largest;
    }
}

/* Auxiliary function for swapping. */
static inline void
swap (node *a, node *b)
{
  node tmp = *a;
  *a = *b;
  *b = tmp;
}

/* Auxiliary function for comparing nodes. */
static inline int
larger (node *a, node *b)
{
  return (a->size > b->size) || (a->size == b->size && a->time_stamp > b->time_stamp);
}
