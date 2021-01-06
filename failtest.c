#include <stdio.h>
#include "green.h"

int shared_resource = 0;
/* shows that the parallell execution is incorrect since it isn't consistent
with it's sequential counter part. In a sequenatial execution we can predict the
outcome of the program. However when the two time slice scheduled threads
repeatedly write to the shared object in memory the result may vary. This is due
to the non-atomic way the object is updated when "shared_resource--;" or
"shared_resource++;" is executed. Our thread library falls short and the
solution to this shortcoming, synchronization with mutex locks, follows in the
next chapter. */

void *test(void *arg)
{
    int thread_id = *(int *)arg;
    int loop = 100000;
    while (loop > 0)
    {
        if( thread_id == 1)
            shared_resource--;
        else if(thread_id == 0)
            shared_resource++;
        // printf("thread %d: %d\n", thread_id, loop);
        loop--;
        // shared_resource++;
        // green_yield();
    }
}

int main()
{
    green_t g0, g1;
    int a0 = 0;
    int a1 = 1;
    printf("begin with shared_resource %d\n", shared_resource);
    green_create(&g0, test, &a0);
    green_create(&g1, test, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);
    printf("done, shared_resource is %d\n", shared_resource);
    return 0;
}