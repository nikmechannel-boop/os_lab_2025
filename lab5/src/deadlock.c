#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Два глобальных мьютекса
pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m2 = PTHREAD_MUTEX_INITIALIZER;

void *threadA(void *arg) {
    printf("Thread A: trying to lock m1...\n");
    pthread_mutex_lock(&m1);
    printf("Thread A: locked m1. Sleeping 1 sec...\n");
    sleep(1); // Даем потоку B время захватить m2

    printf("Thread A: trying to lock m2...\n");
    pthread_mutex_lock(&m2); //  ЗДЕСЬ ПОТОК A ЗАВИСНЕТ НАВСЕГДА
    
    printf("Thread A: locked m2!\n");
    pthread_mutex_unlock(&m2);
    pthread_mutex_unlock(&m1);
    return NULL;
}

void *threadB(void *arg) {
    printf("Thread B: trying to lock m2...\n");
    pthread_mutex_lock(&m2);
    printf("Thread B: locked m2. Sleeping 1 sec...\n");
    sleep(1); // Даем потоку A время захватить m1

    printf("Thread B: trying to lock m1...\n");
    pthread_mutex_lock(&m1); // ЗДЕСЬ ПОТОК B ЗАВИСНЕТ НАВСЕГДА
    
    printf("Thread B: locked m1!\n");
    pthread_mutex_unlock(&m1);
    pthread_mutex_unlock(&m2);
    return NULL;
}

int main() {
    pthread_t tA, tB;
    
    // Запускаем оба потока
    pthread_create(&tA, NULL, threadA, NULL);
    pthread_create(&tB, NULL, threadB, NULL);
    
    printf("Main: waiting for threads (they will hang)...\n");
    
    // Ждем потоки. Программа зависнет здесь.
    pthread_join(tA, NULL);
    pthread_join(tB, NULL);
    
    printf("This line will NEVER be printed.\n");
    return 0;
}