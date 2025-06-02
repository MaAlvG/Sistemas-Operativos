#ifndef MYPTHREADS_H
#define MYPTHREADS_H

#include <ucontext.h>
#include <stdint.h>

// Tamaño del stack para cada hilo
#define THREAD_STACK_SIZE (64 * 1024)  // Aumentado para mayor estabilidad

// Tipos de scheduler
typedef enum {
    MYTHREAD_SCHED_RR,        // Round Robin (por defecto)
    MYTHREAD_SCHED_LOTTERY,   // Lottery Scheduling
    MYTHREAD_SCHED_RT         // Real Time Priority
} mythread_sched_t;

// Estados del hilo
typedef enum {
    MYTHREAD_STATE_READY,      // Listo para ejecutar
    MYTHREAD_STATE_RUNNING,    // Ejecutándose actualmente
    MYTHREAD_STATE_TERMINATED, // Terminado
    MYTHREAD_STATE_BLOCKED     // Bloqueado (para futuras extensiones)
} mythread_state_t;

// Atributos del hilo
typedef struct {
    mythread_sched_t sched_policy; // Política de scheduling
    int tickets;                   // Para Lottery Scheduling (1-100)
    int priority;                  // Para Real Time (1-10, mayor = más prioridad)
} mythread_attr_t;

// Estructura del hilo
typedef struct mythread {
    ucontext_t context;           // Contexto de ejecución
    void *stack;                  // Puntero al stack del hilo
    mythread_state_t state;       // Estado actual del hilo
    int id;                       // ID único del hilo
    mythread_attr_t attr;         // Atributos del hilo
    struct mythread *next;        // Puntero al siguiente hilo (lista enlazada)
} mythread_t;

// Mutex simple
typedef struct {
    volatile int locked;          // Estado del mutex (0 = libre, 1 = ocupado)
    mythread_t *owner;           // Hilo que posee el mutex
} mythread_mutex_t;

// Funciones principales de manejo de hilos
int mythread_create(mythread_t *thread, const mythread_attr_t *attr, 
                   void *(*start_routine)(void *), void *arg);
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

// Funciones de scheduling
int mythread_chsched(mythread_t *thread, mythread_sched_t new_sched);
mythread_sched_t mythread_get_current_sched(void);

// Función de inicialización (llamada automáticamente)
void mythread_init_main_thread(void);

// Macros de conveniencia para crear atributos
#define MYTHREAD_ATTR_INIT_RR() \
    ((mythread_attr_t){MYTHREAD_SCHED_RR, 1, 0})

#define MYTHREAD_ATTR_INIT_LOTTERY(tickets) \
    ((mythread_attr_t){MYTHREAD_SCHED_LOTTERY, (tickets), 0})

#define MYTHREAD_ATTR_INIT_RT(priority) \
    ((mythread_attr_t){MYTHREAD_SCHED_RT, 0, (priority)})

// Constantes para valores por defecto
#define MYTHREAD_DEFAULT_TICKETS    1
#define MYTHREAD_DEFAULT_PRIORITY   1
#define MYTHREAD_MAX_TICKETS       100
#define MYTHREAD_MAX_PRIORITY      10

// Códigos de error
#define MYTHREAD_SUCCESS            0
#define MYTHREAD_ERROR_INVALID     -1
#define MYTHREAD_ERROR_NOMEM       -2
#define MYTHREAD_ERROR_BUSY        -3

// Mutex inicializador estático
#define MYTHREAD_MUTEX_INITIALIZER  {0, NULL}

#endif // MYPTHREADS_H