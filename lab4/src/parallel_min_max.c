#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для обработчика
static pid_t *g_child_pids = NULL;
static int g_pnum = 0;

static void on_timeout(int sig) {
    (void)sig;
    for (int i = 0; i < g_pnum; i++) {
        if (g_child_pids[i] > 0) {
            kill(g_child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1, array_size = -1, pnum = -1, timeout = 0; //задали таймаут
  bool with_files = false;

  while (true) {
    static struct option options[] = {
        {"seed",       required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum",       required_argument, 0, 0},
        {"timeout",    required_argument, 0, 0},
        {"by_files",   no_argument,       0, 'f'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0: seed = atoi(optarg); break;
          case 1: array_size = atoi(optarg); break;
          case 2: pnum = atoi(optarg); break;
          case 3: timeout = atoi(optarg); break;
          case 4: with_files = true; break;
        }
        break;
      case 'f': with_files = true; break;
    }
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"sec\"] [--by_files]\n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int pipes[pnum][2];
  char filenames[pnum][64];
  int chunk_size = array_size / pnum;
  pid_t child_pids[pnum]; 
  for (int i = 0; i < pnum; i++) child_pids[i] = -1; //инициализация

  if (timeout > 0) {
    g_child_pids = child_pids;// Передаем массив в глобальную область
    g_pnum = pnum;
    signal(SIGALRM, on_timeout);// Устанавливаем обработчик
    alarm(timeout);// Запускаем таймер
  }

  for (int i = 0; i < pnum; i++) {
    if (!with_files) {
      if (pipe(pipes[i]) == -1) { perror("pipe"); return 1; }
    } else {
      snprintf(filenames[i], sizeof(filenames[i]), "temp_minmax_%d.txt", i);
    }

    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        unsigned int begin = i * chunk_size;
        unsigned int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
        struct MinMax local = GetMinMax(array, begin, end);

        if (with_files) {
          FILE *f = fopen(filenames[i], "w");
          if (f) { fprintf(f, "%d %d\n", local.min, local.max); fclose(f); }
        } else {
          close(pipes[i][0]);
          write(pipes[i][1], &local, sizeof(struct MinMax));
          close(pipes[i][1]);
        }
        return 0;
      } else {
        child_pids[i] = child_pid;
        if (!with_files) close(pipes[i][1]);
      }
    }
  }

  while (active_child_processes > 0) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0) active_child_processes -= 1;
    else if (pid == -1) break; // Детей больше нет
    else usleep(10000);
  }

  if (timeout > 0) alarm(0); // Отменяем таймер, если успели

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX, max = INT_MIN;
    bool success = false;

    if (with_files) {
      FILE *f = fopen(filenames[i], "r");
      if (f) {
        if (fscanf(f, "%d %d", &min, &max) == 2) success = true;
        fclose(f);
        remove(filenames[i]);
      }
    } else {
      struct MinMax local;
      ssize_t n = read(pipes[i][0], &local, sizeof(struct MinMax)); 
      close(pipes[i][0]);
      if (n == sizeof(struct MinMax)) { //Проверка
        min = local.min;
        max = local.max;
        success = true;
      }
    }

    if (success) {
        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}