#ifndef MYPTHREAD_H
#define MYPTHREAD_H

#include <ucontext.h>
#include <stdint.h>

// Tamaño del stack para cada hilo (8MB)
#define THREAD_STACK_SIZE (8 * 1024 * 1024)

// Definimos los estados de un hilo
typedef enum {
    MYTHREAD_STATE_NEW,
    MYTHREAD_STATE_READY,
    MYTHREAD_STATE_RUNNING,
    MYTHREAD_STATE_BLOCKED,
    MYTHREAD_STATE_TERMINATED
} mythread_state_t;

// Definimos los tipos de scheduler soportados
typedef enum {
    MYTHREAD_SCHED_RR,
    MYTHREAD_SCHED_LOTTERY,
    MYTHREAD_SCHED_RT
} mythread_sched_t;

// Definimos la estructura para atributos del hilo
typedef struct {
    int detachstate;
    mythread_sched_t sched_policy;
    int sched_priority;
    int tickets;
} mythread_attr_t;

// Definimos la estructura principal del hilo
typedef struct mythread {
    ucontext_t context;
    void *stack;
    mythread_state_t state;
    int id;
    mythread_attr_t attr;
    
    void *retval;   // Valor de retorno
    int joined_by;  // ID del hilo que espera este (o -1)
    
    // Para manejo de scheduling
    struct mythread *next;  // Para listas enlazadas
} mythread_t;

// Definimos la estructura para el mutex
typedef struct {
    int locked;     // 0 = libre, 1 = bloqueado
    int owner_id;   // ID del dueño (si está bloqueado)
    mythread_t *wait_queue; // Cola de hilos esperando este mutex
} mythread_mutex_t;

// Constantes para atributos
#define MYTHREAD_CREATE_JOINABLE     0
#define MYTHREAD_CREATE_DETACHED     1

// Prototipos de funciones básicas
int mythread_create(mythread_t *thread, const mythread_attr_t *attr, void *(*start_routine)(void *), void *arg);
void mythread_exit(void *retval);
int mythread_join(mythread_t thread, void **retval);
int mythread_detach(mythread_t thread);
void mythread_yield(void);

// Funciones para cambio de scheduler
int mythread_cached(mythread_t *thread, mythread_sched_t new_sched);
int mythread_set_sched_param(mythread_t *thread, int priority, int tickets);

// Funciones de mutex
int mythread_mutex_init(mythread_mutex_t *mutex);
int mythread_mutex_destroy(mythread_mutex_t *mutex);
int mythread_mutex_lock(mythread_mutex_t *mutex);
int mythread_mutex_unlock(mythread_mutex_t *mutex);
int mythread_mutex_trylock(mythread_mutex_t *mutex);

// Funciones internas (no expuestas en la API pública)
void _mythread_scheduler_init(void);
void _mythread_schedule(void);
void _mythread_context_switch(mythread_t *new_thread);

#endif // MYPTHREAD_H