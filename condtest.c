#include <stdio.h>
#include "green.h"

green_cond_t cond;

volatile int flag = 0;
volatile int shared = 0;
void *test(void *arg) {
    int id = *(int *)arg;
    int loop = 1000000;
    //printf("now running: %d\n", id );
    while (loop > 0) {
        if (flag == id) {
            // printf("thread %d: %d\n", id, loop);
            loop--;
            flag = (id + 1) % 2; 
            shared++;
            green_cond_signal(&cond);
        } else
        {
            // printf("tread %d waiting now \n", id);
            green_cond_wait(&cond, NULL);
        }
    }
}

int main()
{
    green_t g0, g1;
    int a0 = 0;
    int a1 = 1;

    green_cond_init(&cond);

    green_create(&g0, test, &a0);
    green_create(&g1, test, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);

    printf("all done %d \n", shared);
    return 0;
}