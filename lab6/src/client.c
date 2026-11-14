#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "calc_lib.h" // ПОДКЛЮЧАЕМ БИБЛИОТЕКУ

struct Server {
  char ip[255];
  int port;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) return false;
  if (errno != 0) return false;
  *val = i;
  return true;
}

struct ServerTask {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};

void *ServerThread(void *arg) {
  struct ServerTask *task = (struct ServerTask *)arg;
  
  struct hostent *hostname = gethostbyname(task->server.ip);
  if (hostname == NULL) { task->result = 1; return NULL; }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(task->server.port);
  server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) { task->result = 1; return NULL; }

  if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    close(sck); task->result = 1; return NULL;
  }

  char task_buf[sizeof(uint64_t) * 3];
  memcpy(task_buf, &task->begin, sizeof(uint64_t));
  memcpy(task_buf + sizeof(uint64_t), &task->end, sizeof(uint64_t));
  memcpy(task_buf + 2 * sizeof(uint64_t), &task->mod, sizeof(uint64_t));

  send(sck, task_buf, sizeof(task_buf), 0);

  char response[sizeof(uint64_t)];
  recv(sck, response, sizeof(response), 0);
  memcpy(&task->result, response, sizeof(uint64_t));

  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = 0, mod = 0;
  char servers_file[255] = {'\0'};

  static struct option options[] = {
      {"k", required_argument, 0, 0},
      {"mod", required_argument, 0, 0},
      {"servers", required_argument, 0, 0},
      {0, 0, 0, 0}
  };

  int option_index = 0, c;
  while ((c = getopt_long(argc, argv, "", options, &option_index)) != -1) {
    if (c == 0) {
      if (option_index == 0) ConvertStringToUI64(optarg, &k);
      else if (option_index == 1) ConvertStringToUI64(optarg, &mod);
      else if (option_index == 2) strncpy(servers_file, optarg, 254);
    }
  }

  if (k == 0 || mod == 0 || strlen(servers_file) == 0) {
    fprintf(stderr, "Usage: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(servers_file, "r");
  if (!f) { perror("fopen"); return 1; }

  struct Server servers[10];
  int servers_num = 0;
  char line[128];
  while (fgets(line, sizeof(line), f) && servers_num < 10) {
    char *colon = strchr(line, ':');
    if (colon) {
      *colon = '\0';
      strcpy(servers[servers_num].ip, line);
      servers[servers_num].port = atoi(colon + 1);
      servers_num++;
    }
  }
  fclose(f);

  if (servers_num == 0) return 1;

  pthread_t threads[servers_num];
  struct ServerTask tasks[servers_num];
  uint64_t chunk = k / servers_num;

  for (int i = 0; i < servers_num; i++) {
    tasks[i].server = servers[i];
    tasks[i].begin = i * chunk + 1;
    tasks[i].end = (i == servers_num - 1) ? k : (i + 1) * chunk;
    tasks[i].mod = mod;
    tasks[i].result = 1;

    pthread_create(&threads[i], NULL, ServerThread, &tasks[i]);
  }

  uint64_t final_answer = 1;
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    final_answer = MultModulo(final_answer, tasks[i].result, mod);
  }

  printf("answer: %lu\n", final_answer);
  return 0;
}