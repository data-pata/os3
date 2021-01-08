#include <stdio.h>
#include "green.h"
#include <unistd.h>

long count = 0;
green_mutex_t lock;
int shared_resource = 0;

void *test(void *arg) {
    int thread_id = *(int *)arg;
    int loop = 1000000;
    while (loop > 0)
    {
        // printf("thread %d: %d\n",i, loop );
        green_mutex_lock(&lock);
        if (thread_id == 1)
            shared_resource--;
        else if (thread_id == 0)
            shared_resource++;
        green_mutex_unlock(&lock);
        // sleep(10);
        loop--;
        //  green_yield();
    }
}

int main()
{
    green_t g0, g1;
    int a0 = 0;
    int a1 = 1;
    green_create(&g0, test, &a0);
    green_create(&g1, test, &a1);

    green_mutex_init(&lock);

    green_join(&g0, NULL);
    green_join(&g1, NULL);
    printf("all done and the final count is %ld\n", count);
    return 0;
}