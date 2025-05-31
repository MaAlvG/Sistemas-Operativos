#ifndef MYPTHREAD_H
#define MYPTHREAD_H

#include <ucontext.h>

// Tamaño del stack para cada hilo
#define THREAD_STACK_SIZE (8 * 1024)

// Tipos de scheduler
typedef enum {
    MYTHREAD_SCHED_RR,
    MYTHREAD_SCHED_LOTTERY,
    MYTHREAD_SCHED_RT
} mythread_sched_t;

// Estados del hilo
typedef enum {
    MYTHREAD_STATE_READY,
    MYTHREAD_STATE_RUNNING,
    MYTHREAD_STATE_TERMINATED
} mythread_state_t;

// Atributos del hilo
typedef struct {
    mythread_sched_t sched_policy;
    int tickets;       // Para Lottery
    int priority;      // Para Tiempo Real
} mythread_attr_t;

// Estructura del hilo
typedef struct mythread {
    ucontext_t context;
    void *stack;
    mythread_state_t state;
    int id;
    mythread_attr_t attr;
    struct mythread *next;
} mythread_t;

// Mutex
typedef struct {
    int locked;
    mythread_t *owner;
} mythread_mutex_t;

// Funciones principales
int mythread_create(mythread_t *thread, const mythread_attr_t *attr, void *(*start_routine)(void *), void *arg);
void mythread_exit(void);
void mythread_yield(void);
int mythread_join(mythread_t *thread);
int mythread_detach(mythread_t *thread);

// Funciones de mutex
int mythread_mutex_init(mythread_mutex_t *mutex);
int mythread_mutex_destroy(mythread_mutex_t *mutex);
int mythread_mutex_lock(mythread_mutex_t *mutex);
int mythread_mutex_unlock(mythread_mutex_t *mutex);
int mythread_mutex_trylock(mythread_mutex_t *mutex);

// Cambiar scheduler
int mythread_chsched(mythread_t *thread, mythread_sched_t new_sched);

// Función para obtener el scheduler del hilo actual
mythread_sched_t mythread_get_current_sched(void);

// Inicializar el hilo principal
void mythread_init_main_thread(void);

#endif // MYPTHREAD_H