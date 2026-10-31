#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        //  Дочерний процесс 
        printf("Child (PID %d) is running. Exiting now...\n", getpid());
        exit(0); // Ребенок завершается, но родитель его НЕ ждет
    } else if (pid > 0) {
        //  Родительский процесс 
        printf("Parent (PID %d) created child (PID %d).\n", getpid(), pid);
        printf("Parent is sleeping for 20 seconds. Child is now a ZOMBIE!\n");
        printf("Run in another terminal: ps -el | grep Z\n\n");
        
        sleep(20); // Спим 20 секунд, давая  время увидеть зомби
        
        printf("Parent woke up. Calling wait() to clean up zombie...\n");
        wait(NULL); // Забираем код возврата зомби, и он исчезает
        printf("Zombie removed. Parent exiting.\n");
    } else {
        perror("fork");
        return 1;
    }
    return 0;
}