/* 
 * Raw (unoptimized) project running-through.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <omp.h>
#include <sys/time.h>

#pragma GCC diagnostic ignored "-Wunused-result"  // Shutdown unused warnings for `fscanf'.

/* Predefined constants. */
#define NAME_LENGTH_MAX 35    // Max length of command component.
#define LINE_LENGTH_MAX 60    // Max length of an entry line.
#define R_IDX 0               // File index of R.csv.
#define W_IDX 1               // File index of W.csv.
#define FILE_NUM 32           // Number of source files.
#define INST_LINE_LENGTH 42   // Length of first line (Timestamp,...).
#define SIZE_MAX 600000       // Upper bound of size.

/* Scanned statistics. */
static long unsigned int NUM_ARR[2][FILE_NUM] = {0};  // Number of entries in each source file.
static long unsigned int NUM[2] = {0};                // Number of entries of read / write.
static unsigned int CNT[2] = {0};                     // Number of different sizes of read / write.
static unsigned int NUM_THREADS;                      // Parallel degree of OpenMP.

/* Type definitions. */
typedef void (*PROCESS) (void);   // Type of process handler function.
typedef struct                    // Type of an entry node.
  {
    unsigned int size;
    double time_stamp;
    int src_file_idx;
    long unsigned int offset;
    long unsigned int write_offset;
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

/* Main function for optimized project. */
int
main (void)
{
  char file_name[NAME_LENGTH_MAX];
  int file_idx = 0;

  /* Set OpenMP parallel degree. */
  NUM_THREADS = omp_get_num_procs () > (FILE_NUM / 2) ? (FILE_NUM / 2) : omp_get_num_procs ();

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
  int file_cnt = FILE_NUM;                                    // `.csv.gz' files.

  /* Untar the source file. */
  if (fork () == 0)
    execl ("/bin/tar", "tar", "-xf", tar_file, "-C", "input", NULL);
  else
    wait (NULL);

  /* Use OpenMP for paralleled unzipping. */
  #pragma omp parallel for num_threads(NUM_THREADS)
  for (int id = 0; id < 6; id++)
    {
      char gz_file[NAME_LENGTH_MAX];

      /* Use separate processes for unzipping different files. */
      for (int date = 7; date <= (id < 2 ? 12 : 11); date++)
        {
          sprintf (gz_file, "input/20160222%02d-LUN%d.csv.gz", date, LUN_idx_arr[id]);
          if (fork () == 0)
            execl ("/bin/gunzip", "gunzip", "-qf", gz_file, NULL);
        }
    }

  /* Wait for all unzipping processes to finish. */
  while (file_cnt-- > 0)
    wait (NULL);
}

/* Statistics collecting process handler. */
void
scanStatistics (void)
{
  int size_mark[2][SIZE_MAX] = {0};
  long unsigned int R_num_tmp = 0, W_num_tmp = 0;

  /* Use OpenMP for paralleled statistics scanning. */
  #pragma omp parallel for num_threads(NUM_THREADS) reduction(+:R_num_tmp, W_num_tmp)
  for (int i = 0; i < FILE_NUM; i++)        // Each thread has several independent
    {                                       // source files to work with.
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
          if (mode_idx == R_IDX)
            R_num_tmp++;
          else
            W_num_tmp++;
          size_mark[mode_idx][size] = 1;
          NUM_ARR[mode_idx][i]++;
        }
      fseek (src_file, 0, SEEK_SET);  // Reset source file position.
    }
  NUM[R_IDX] = R_num_tmp;
  NUM[W_IDX] = W_num_tmp;

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
  long unsigned int slot_idx_arr[2][FILE_NUM] = {0};

  /* Accumulate the slot indexes that each file starts. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    for (int i = 1; i < FILE_NUM; i++)
      slot_idx_arr[mode_idx][i] = slot_idx_arr[mode_idx][i - 1] + NUM_ARR[mode_idx][i - 1];

  /* Use OpenMP for paralleled reading. */
  #pragma omp parallel num_threads(NUM_THREADS)
    {
      int thread_id = omp_get_thread_num (), num_threads = omp_get_num_threads ();
      int workload = FILE_NUM / num_threads;
      int start = 0 + thread_id * workload;
      int end = thread_id == num_threads - 1 ? FILE_NUM : start + workload;
      long unsigned int slot_idx[2] = {slot_idx_arr[R_IDX][start], slot_idx_arr[W_IDX][start]};

      /* Read and create nodes. */
      for (int i = start; i < end; i++)   // Each thread has several independent
        {                                 // source files to work with.
          FILE *src_file = global_src_file[i];
          long unsigned int offset = INST_LINE_LENGTH;
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

              /* Fill in an empty slot in corresponding node array. */
              slot_idx[mode_idx]++;
              node_arr[mode_idx][slot_idx[mode_idx]].size = size;
              node_arr[mode_idx][slot_idx[mode_idx]].time_stamp = time_stamp;
              node_arr[mode_idx][slot_idx[mode_idx]].src_file_idx = i;
              node_arr[mode_idx][slot_idx[mode_idx]].offset = offset;
              node_arr[mode_idx][slot_idx[mode_idx]].write_offset = strlen (line) + 1;

              /* Update offset. */
              offset += strlen (line) + 1;
            }
        }
    }
}

/* Entries sorting process handler. */
void
sortEntries (void)
{
  /* For two node arrays, use heap-sort in ascending order. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    {
      long unsigned int heap_size = NUM[mode_idx];

      /* Setup dummy head. */
      node_arr[mode_idx][0].size = 0;
      node_arr[mode_idx][0].time_stamp = 0;
      node_arr[mode_idx][0].src_file_idx = 0;
      node_arr[mode_idx][0].offset = 0;
      node_arr[mode_idx][0].write_offset = INST_LINE_LENGTH;

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
  /* Calculate the offset in destination files to write at. */
  for (int mode_idx = 0; mode_idx < 2; mode_idx++)
    for (long unsigned int i = 1; i <= NUM[mode_idx]; i++)
      node_arr[mode_idx][i].write_offset += node_arr[mode_idx][i - 1].write_offset;

  /* Use OpenMP for paralleled writing. */
  #pragma omp parallel num_threads(NUM_THREADS)
    {
      FILE *dst_file[2], *local_src_file[FILE_NUM];
      char file_name[NAME_LENGTH_MAX], line[LINE_LENGTH_MAX];
      int file_idx = 0;

      /* Open source files and destination files locally. */
      for (int date = 7; date <= 12; date++)
        for (int id = 0; id < (date == 12 ? 2 : 6); id++)
          {
            sprintf (file_name, "input/20160222%02d-LUN%d.csv", date, LUN_idx_arr[id]);
            local_src_file[file_idx] = fopen (file_name, "r");
            file_idx++;
          }
      dst_file[R_IDX] = fopen ("output/R.csv", "w");
      dst_file[W_IDX] = fopen ("output/W.csv", "w");

      /* Write the lines into destination files. */
      for (int mode_idx = 0; mode_idx < 2; mode_idx++)
        {
          int thread_id = omp_get_thread_num (), num_threads = omp_get_num_threads ();
          long unsigned int workload = NUM[mode_idx] / num_threads;
          long unsigned int start = 1 + thread_id * workload;
          long unsigned int end = thread_id == num_threads - 1 ? NUM[mode_idx] + 1 : start + workload;

          /* First thread writes the instruction line, others start from section head. */
          if (thread_id == 0)
            {
              fseek (dst_file[mode_idx], 0, SEEK_SET);
              fprintf (dst_file[mode_idx], "Timestamp,Response,IOType,LUN,Offset,Size\n");
            }
          else
            fseek (dst_file[mode_idx], node_arr[mode_idx][start - 1].write_offset, SEEK_SET);

          /* All threads write entries concurrently. */
          for (long unsigned int i = start; i < end; i++)   // Each thread has own independent
            {                                               // lines section to work with.
              fseek (local_src_file[node_arr[mode_idx][i].src_file_idx],
                     node_arr[mode_idx][i].offset, SEEK_SET);
              fscanf (local_src_file[node_arr[mode_idx][i].src_file_idx], "%s\n", line);
              fprintf (dst_file[mode_idx], "%s\n", line);
            }

          /* Last thread writes the size counts data. */
          if (thread_id == num_threads - 1)
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
        }

      /* Close locally opened files. */
      for (int i = 0; i < FILE_NUM; i++)
        fclose (local_src_file[i]);
      fclose (dst_file[R_IDX]);
      fclose (dst_file[W_IDX]);
    }
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
