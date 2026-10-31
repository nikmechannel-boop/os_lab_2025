#ifndef SUM_LIB_H
#define SUM_LIB_H

// Структура для передачи аргументов в поток
struct SumArgs {
  int *array; // Указатель на массив
  int begin;  // С какого индекса считать
  int end;    // По какой индекс (не включая)
};

// Функция подсчета суммы на заданном промежутке
int Sum(const struct SumArgs *args);

#endif