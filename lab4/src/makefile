CC=gcc
CFLAGS=-I. -Wall

# Собираем  1-е и 3-е задание
all: parallel_min_max process_memory

# Задание 1
parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

# Задание 3
process_memory : process_memory.c
	$(CC) -o process_memory process_memory.c -w -std=gnu89

# Компиляция объектных файлов
utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

# Очистка
clean :
	rm -f utils.o find_min_max.o parallel_min_max process_memory temp_minmax_*.txt

.PHONY: all clean