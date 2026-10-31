#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <stdint.h>

uint64_t k = 0;       // Число, факториал которого ищем
uint64_t mod = 1;     // Модуль
uint32_t pnum = 1;    // Кол-во потоков
uint64_t result = 1;  // Глобальный результат
pthread_mutex_t mutex;

// Структура для передачи границ в поток
struct ThreadArgs {
    uint64_t start;
    uint64_t end;
};

// Функция, которая выполняется в потоке
void *ThreadFactorial(void *args) {
    struct ThreadArgs *range = (struct ThreadArgs *)args;
    uint64_t local_res = 1;

    // Считаем произведение на СВОЕМ куске
    for (uint64_t i = range->start; i <= range->end; i++) {
        local_res = (local_res * (i % mod)) % mod;
    }
    
    // Критическая секция: добавляем результат в общий
    pthread_mutex_lock(&mutex);
    result = (result * local_res) % mod;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

int main(int argc, char **argv) {
    // Разбор аргументов (k, pnum, mod)
    static struct option options[] = {
        {"k",     required_argument, 0, 'k'},
        {"pnum",  required_argument, 0, 'p'},
        {"mod",   required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    int option_index = 0, c;
    while ((c = getopt_long(argc, argv, "k:p:m:", options, &option_index)) != -1) {
        switch (c) {
            case 'k': k = atoi(optarg); break;
            case 'p': pnum = atoi(optarg); break;
            case 'm': mod = atoi(optarg); break;
        }
    }

    if (k == 0 || mod <= 1 || pnum == 0) {
        printf("Usage: %s -k 10 --pnum=4 --mod=10\n", argv[0]);
        return 1;
    }

    pthread_mutex_init(&mutex, NULL);
    pthread_t threads[pnum];
    struct ThreadArgs args[pnum];
    
    // Делим k на куски
    uint64_t chunk = k / pnum;
    if (chunk == 0) chunk = 1; // Если потоков больше, чем чисел

    for (uint32_t i = 0; i < pnum; i++) {
        args[i].start = i * chunk + 1; // Начинаем с 1, а не с 0
        args[i].end = (i == pnum - 1) ? k : (i + 1) * chunk;
        
        // Защита от пустого диапазона (если потоков слишком много)
        if (args[i].start > args[i].end) {
            args[i].start = args[i].end;
        }

        pthread_create(&threads[i], NULL, ThreadFactorial, &args[i]);
    }

    // Ждем завершения всех потоков
    for (uint32_t i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    printf("%lu! mod %lu = %lu\n", k, mod, result);
    return 0;
}