#include <semaphore.h>

int main(int argc, char **argv) {
    sem_t sem;

    if (sem_init(&sem, 0, 0) != 0) {
        return 1;
    }

    return 0;
}
