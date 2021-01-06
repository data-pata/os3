#include "green.h"
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

int NUM_WORKERS = 5;
int LOOPS = 10;
double start_time = 0.0;

void task(void *arg)
{

    for (size_t i = 0; i < LOOPS; i++)
    {
        printf("Hello from thread %d, round: %d, time: %f\n", *(int*)arg, i, read_timer());
    }
}

int main(int argc, char const *argv[])
{
    start_time = read_timer();
    green_t *workerid[NUM_WORKERS];

    for (size_t i = 0; i < NUM_WORKERS ; i++)
    {
        int a = 0;
        green_t *worker;
        workerid[i] = worker;
        green_create(workerid, task, &i);
    }

    for (size_t i = 0; i < 4; i++)
    {

        /* code */
    }

    return 0;
}

double read_timer()
{
    // static bool initialized = false;
    struct timeval time;
    // struct timeval end;
    // if (!initialized)
    // {
        gettimeofday(&time, NULL);
        // initialized = true;
    // }
    // gettimeofday(&end, NULL);
    // return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
    return (time.tv_sec - start_time) + 1.0e-6 * (time.tv_usec - start_time)
}