#include <ucontext.h>

typedef struct green_t
{
    ucontext_t *context;
    void *(*fun)(void *);   
    void *arg;              
    struct green_t *next;   // next thread in suspended-linked-list
    struct green_t *join;   // thread waiting on this to terminate
    void *retval;           
    int zombie;            // indicates if terminated
} green_t;

void green_thread();
int green_create(green_t *thread, void *(*fun)(void *), void *arg);
int green_yield();
int green_join(green_t *thread, void **val);

green_t *dequeue(green_t **queue);
void enqueue(green_t **queue, green_t *thread);

typedef struct green_cond_t {
    struct green_t *susp_queue;
} green_cond_t;

typedef struct green_mutex_t
{
    volatile int taken;
    // handle the list
    struct green_t *susp_queue;
} green_mutex_t;

int green_mutex_init(green_mutex_t *mutex);
int green_mutex_lock(green_mutex_t *mutex);
int green_mutex_unlock(green_mutex_t *mutex);

void green_cond_init(green_cond_t *);
// void green_cond_wait(green_cond_t*);
void green_cond_wait(green_cond_t *cond, green_mutex_t *mutex);
void green_cond_signal(green_cond_t *);