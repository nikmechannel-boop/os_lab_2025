#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "calc_lib.h" // ПОДКЛЮЧАЕМ БИБЛИОТЕКУ

// Потоковая функция (структура и функции теперь в библиотеке)
void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  return (void *)(uint64_t *)Factorial(fargs);
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  while (true) {
    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 0:
        if (option_index == 0) port = atoi(optarg);
        else if (option_index == 1) tnum = atoi(optarg);
        break;
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) { perror("socket"); return 1; }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("bind"); return 1;
  }
  if (listen(server_fd, 128) < 0) {
    perror("listen"); return 1;
  }
  printf("Server listening at %d\n", port);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
    if (client_fd < 0) continue;

    while (true) {
      unsigned int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      int read_bytes = recv(client_fd, from_client, buffer_size, 0);
      if (read_bytes <= 0) break;
      if (read_bytes < buffer_size) break;

      uint64_t begin = 0, end = 0, mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      fprintf(stdout, "Receive: %lu %lu %lu\n", begin, end, mod);

      pthread_t threads[tnum];
      struct FactorialArgs args[tnum];
      uint64_t chunk_size = (end - begin + 1) / tnum;
      if (chunk_size == 0) chunk_size = 1;

      for (uint32_t i = 0; i < tnum; i++) {
        args[i].begin = begin + i * chunk_size;
        args[i].end = (i == tnum - 1) ? end : (begin + (i + 1) * chunk_size - 1);
        args[i].mod = mod;
        if (args[i].begin > args[i].end) args[i].begin = args[i].end;

        if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
          printf("Error: pthread_create failed!\n");
          return 1;
        }
      }

      uint64_t total = 1;
      for (uint32_t i = 0; i < tnum; i++) {
        uint64_t result = 0;
        pthread_join(threads[i], (void **)&result);
        total = MultModulo(total, result, mod);
      }

      printf("Total: %lu\n", total);

      char buffer[sizeof(total)];
      memcpy(buffer, &total, sizeof(total));
      send(client_fd, buffer, sizeof(total), 0);
    }
    close(client_fd);
  }
  return 0;
}