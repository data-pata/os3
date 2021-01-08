#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#include "green.h"

static sigset_t block;

void timer_handler(int);

#define PERIOD 100

#define FALSE 0  
#define TRUE 1

#define STACK_SIZE 4096

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE};

// currently running thread, 
// which has a pointer to next ready thread?! 
static green_t *running = &main_green;
struct green_t *ready_queue = NULL;

// called on program load
static void init() __attribute__((constructor));
void init()
{
    sigemptyset(&block);
    sigaddset(&block, SIGALRM);
    // sigaddset(&block, SIGVTALRM);

    struct sigaction act = {0};
    struct timeval interval;
    struct itimerval period;

    act.sa_handler = timer_handler;
    // assert(sigaction(SIGVTALRM, &act, NULL) == 0);
    assert(sigaction(SIGALRM, &act, NULL) == 0);
    interval.tv_sec = 0;
    interval.tv_usec = PERIOD;
    period.it_interval = interval;
    period.it_value = interval;
    setitimer(ITIMER_REAL, &period, NULL);
    // setitimer(ITIMER_VIRTUAL, &period, NULL);

    /*The  function getcontext() initializes the structure pointed at by ucp to
       the currently active context.*/
    getcontext(&main_cntx);
    // if(errno) printf("getcontext failed\n");

}
void timer_handler(int sig)
{   
    // printf("interrupt\n");
    // green_yield();
    green_t *susp = running;
    enqueue(&ready_queue, susp);
    green_t *next = dequeue(&ready_queue);
    running = next;
    swapcontext(susp->context, next->context);
}

int green_create(green_t *new, void *(*fun) (void *), void *arg)
{
    sigprocmask(SIG_BLOCK, &block, NULL);
    // create a new context
    ucontext_t *cntx = (ucontext_t *) malloc(sizeof(ucontext_t));
    getcontext(cntx);

    void *stack = malloc(STACK_SIZE);

    cntx->uc_stack.ss_sp = stack;
    cntx->uc_stack.ss_size = STACK_SIZE;
    makecontext(cntx, green_thread, 0); 

    // attach context to the the new thread
    new->context = cntx;
    new->fun = fun;
    new->arg = arg;
    new->next = NULL;
    new->join = NULL;
    new->retval = NULL;
    new->zombie = FALSE;

    enqueue(&ready_queue, new);
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

/*start the execution of the real function and, after returning from the
call, terminate the thread.*/
void green_thread()
{
    sigprocmask(SIG_BLOCK, &block, NULL);
    green_t *this = running;
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    // call function of "this", func, with pointer to args as parameter arg
    void *result = (*this->fun)(this->arg);

    sigprocmask(SIG_BLOCK, &block, NULL);
    // this place waiting (joining) thread in ready queue
    if (this->join != NULL)
        enqueue(&ready_queue, this->join);

    // save result of execution in this struct
    // problem that result is a local pointer??
    this->retval = result;

    //free allocated memory (stack and whole context) of this thread
    free(this->context->uc_stack.ss_sp);
    free((void *)this->context);

    // we're a zombie 
    this->zombie = TRUE;

    // find the next thread to run
    green_t *next = dequeue(&ready_queue);  

    running = next; // will set running to null if no waiting queue!!?
    setcontext(next->context); // switches context to next and terminates (destroys?) this context
    
    sigprocmask(SIG_UNBLOCK, &block, NULL);
}

int green_yield()
{
    sigprocmask(SIG_BLOCK, &block, NULL);
    green_t *susp = running;
    // add susp to ready queue
    enqueue(&ready_queue, susp);
    // select the next thread for execution
    green_t *next = dequeue(&ready_queue);

    running = next;
    swapcontext(susp->context, next->context);
    sigprocmask(SIG_UNBLOCK, &block, NULL);  // really here? wont this stop interrupts for all following threads until this one is running again??
    return 0;
}
// OBsERVERA RES GÖRS INGET MED ?
int green_join(green_t *thread, void **res)
{
    if(!thread->zombie) 
    {
        sigprocmask(SIG_BLOCK, &block, NULL);
        green_t *susp = running;
        //add susp to thread's join queue
        enqueue(&thread->join, susp);
        // select the next thread for execution from ready_queue
        green_t *next = dequeue(&ready_queue);
        running = next; // already done in in d
        swapcontext(susp->context, next->context);
      
        sigprocmask(SIG_UNBLOCK, &block, NULL); 
    }
    // retain return value from zombie thread
    void *result = thread->retval;
    // free(thread);
    // if(running->context != NULL)
    // {
    //     free(running->context->uc_stack.ss_sp);
    //     free((void *)running->context);
    // }
    //free allocated memory (stack and whole context) of this thread
    // green_t *this = running;

    /* collect result and free context  
    is done in green_thread which is called through swapcontext ?? */
    return 0;
}

green_t *dequeue(green_t **queue_head)
{
    if (*queue_head == NULL)
        return NULL;    // CHANGE TO RETURN &main_green ??
    green_t *dequeued = *queue_head; // yields a pointer to the head struct
    *queue_head = dequeued->next;
    dequeued->next = NULL;  // detach from queue
    return dequeued;
}

void enqueue(green_t **queue_head, green_t *thread)
{ // IF queue_head is not pointer to pointer IT MIGHT ONLY CHANGE POINTER LOCALLY !!
    // pointer is passed by value so copying to head is verbose but easier to understand
    // green_t *head = queue_head;
    // empty queue add first
    if(thread == NULL) return;
    if(*queue_head == NULL) 
    {
        *queue_head = thread;  
    }
    else
    {
        while( (*queue_head)->next != NULL)
        {
            (*queue_head) = (*queue_head)->next;
        }
        (*queue_head)->next = thread;
    }
}

void green_cond_init(green_cond_t *cond) 
{
    cond->susp_queue = NULL;
}

// void green_cond_wait(green_cond_t *cond)
// {
//     sigprocmask(SIG_BLOCK, &block, NULL);
//     green_t *susp = running;
//     enqueue(&cond->susp_queue, susp);
//     green_t *next = dequeue(&ready_queue);
//     running = next;
//     swapcontext(susp->context, next->context);
//     sigprocmask(SIG_UNBLOCK, &block, NULL);
// }
void green_cond_wait(green_cond_t *cond, green_mutex_t *mutex) 
{
    sigprocmask(SIG_BLOCK, &block, NULL);   // block timer interrupt
    green_t *susp = running;                // suspend the running thread on condition
    enqueue(&cond->susp_queue, susp);   

    if(mutex != NULL) 
    {
        // release the lock if we have a mutex
        if(mutex->susp_queue != NULL)
        {
            green_t *to_ready = dequeue(&mutex->susp_queue);
            enqueue(&ready_queue, to_ready);    // obs adds only one thread to ready_queue?
        }   // what if we dont have the lock, then it's already false and we put susp in ready
        else
            mutex->taken = FALSE;
        // move suspended thread (on mutex) to ready queue
    }
    // if(mutex != NULL)
    //     green_mutex_unlock(mutex);
    
    // run next thread and start waiting
    green_t *next = dequeue(&ready_queue);
    running = next;
    swapcontext(susp->context, next->context); 
    
    // woken up here on condition signal
    if (mutex != NULL)
    {
        // trying to take the lock, always returns with lock taken 
        // green_mutex_lock(mutex);
        if (mutex->taken)
        {
            // bad luck, suspend on the lock
            enqueue(&mutex->susp_queue, susp);
            green_t *next = dequeue(&ready_queue);
            running = next;
            swapcontext(susp->context, next->context);
        }
        else  // take the lock
        mutex->taken = TRUE;
    }
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    // return 0;
}

void green_cond_signal(green_cond_t *cond) 
{
    if (cond->susp_queue != NULL)
    {
        sigprocmask(SIG_BLOCK, &block, NULL);
        green_t *to_signal = dequeue(&cond->susp_queue);
        enqueue(&ready_queue, to_signal);
        sigprocmask(SIG_BLOCK, &block, NULL);
    }
}

int green_mutex_init(green_mutex_t *mutex)
{
    mutex->taken = FALSE;
    mutex->susp_queue = NULL;
}

int green_mutex_lock(green_mutex_t *mutex)
{   // block timer interrupt
    sigprocmask(SIG_BLOCK, &block, NULL);

    green_t *susp = running;
    if (mutex->taken)
    {
    // suspend the running thread
    enqueue(&mutex->susp_queue, susp);
    // find the next thread
    green_t *next = dequeue(&ready_queue);
    running = next;
    swapcontext(susp->context, next->context);
    }
    else
    {
        // take the lock
        mutex->taken = TRUE;
    }
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}
/* unlock lock by passing the batton to next suspended on lock or simply
releasing it for taking*/
int green_mutex_unlock(green_mutex_t *mutex)
{
    sigprocmask(SIG_BLOCK, &block, NULL);
    // if another thread is suspended, waiting on the lock: pass the baton. 
  if(mutex->susp_queue != NULL)
  {
      // move suspended thread to ready queue
      green_t *to_ready = dequeue(&mutex->susp_queue);
      enqueue(&ready_queue, to_ready);
  }
  else  
  {
      mutex->taken = FALSE;
  }
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}