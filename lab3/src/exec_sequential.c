#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
int main(int argc, char **argv) {
    // Ожидаем ровно 2 аргумента: seed и array_size
    if (argc != 3) {
        printf("Usage: %s seed array_size\n", argv[0]);
        return 1;
    }
    pid_t pid = fork();
    if (pid == -1) {
        // fork не удался
        perror("fork");
        return 1;
    } else if (pid == 0) {
        // ===== ребёнок =====
        // Заменяем себя на sequential_min_max
        execl("./sequential_min_max", "sequential_min_max", argv[1], argv[2], NULL);

        // Если мы здесь — exec не сработал
        perror("execl");
        exit(1);
    } else {
        // ===== родитель =====
        int status;
        waitpid(pid, &status, 0);
        printf("Child process finished with status %d\n", WEXITSTATUS(status));
    }
    return 0;
}