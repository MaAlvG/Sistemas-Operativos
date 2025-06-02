#include "myPthreads.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

// Variables globales
static mythread_t *current_thread = NULL;
static mythread_t *ready_queue = NULL;
static int next_id = 1;
static int quantum = 10000; // Quantum en microsegundos
static int scheduler_initialized = 0;

// Contador global de hilos activos
static int active_threads = 0;

// Hilo principal
static mythread_t main_thread;

// Variables para el scheduler de Sorteo
static int total_tickets = 0;
static int random_seed_initialized = 0;

// Variables para el scheduler de Tiempo Real
static struct timespec start_time;

// Mutex simple para proteger estructuras críticas
static volatile int queue_lock = 0;

// Forward declarations
static void scheduler_timer_handler(int sig);
static void setup_scheduler_timer(void);

// Función auxiliar para adquirir lock simple
static void acquire_queue_lock(void) {
    while (__sync_lock_test_and_set(&queue_lock, 1)) {
        // Spin wait
    }
}

// Función auxiliar para liberar lock simple
static void release_queue_lock(void) {
    __sync_lock_release(&queue_lock);
}

// Función auxiliar para agregar un hilo a la cola de listos (thread-safe)
static void enqueue_ready(mythread_t *thread) {
    acquire_queue_lock();
    
    thread->state = MYTHREAD_STATE_READY;
    thread->next = NULL;
    
    if (!ready_queue) {
        ready_queue = thread;
        thread->next = thread; // Lista circular
    } else {
        thread->next = ready_queue->next;
        ready_queue->next = thread;
        ready_queue = thread; // El último insertado se convierte en tail
    }
    
    // Actualizar total de tickets para scheduler de Sorteo
    if (thread->attr.sched_policy == MYTHREAD_SCHED_LOTTERY) {
        total_tickets += thread->attr.tickets;
    }
    
    release_queue_lock();
}

// Función auxiliar para obtener el siguiente hilo según el scheduler
static mythread_t *select_next_thread(void) {
    if (!ready_queue) return NULL;
    
    mythread_t *next = NULL;
    
    // Usar scheduler por defecto Round Robin si no se especifica
    mythread_sched_t sched_policy = current_thread ? 
        current_thread->attr.sched_policy : MYTHREAD_SCHED_RR;
    
    switch (sched_policy) {
        case MYTHREAD_SCHED_RR:
        default:
            next = ready_queue->next;
            break;
            
        case MYTHREAD_SCHED_LOTTERY:
            if (total_tickets > 0) {
                if (!random_seed_initialized) {
                    srand(time(NULL));
                    random_seed_initialized = 1;
                }
                int random_ticket = rand() % total_tickets;
                int current_ticket_sum = 0;
                mythread_t *current = ready_queue->next;
                do {
                    if (current->attr.sched_policy == MYTHREAD_SCHED_LOTTERY) {
                        current_ticket_sum += current->attr.tickets;
                        if (current_ticket_sum > random_ticket) {
                            next = current;
                            break;
                        }
                    }
                    current = current->next;
                } while (current != ready_queue->next);
            }
            if (!next) next = ready_queue->next; // Fallback a RR
            break;
            
        case MYTHREAD_SCHED_RT:
            // Buscar el hilo con mayor prioridad
            mythread_t *highest_priority = ready_queue->next;
            mythread_t *current_rt = ready_queue->next;
            do {
                if (current_rt->attr.sched_policy == MYTHREAD_SCHED_RT && 
                    current_rt->attr.priority > highest_priority->attr.priority) {
                    highest_priority = current_rt;
                }
                current_rt = current_rt->next;
            } while (current_rt != ready_queue->next);
            next = highest_priority;
            break;
    }
    
    return next;
}

// Función auxiliar para remover un hilo específico de la cola (thread-safe)
static mythread_t *dequeue_ready(mythread_t *target) {
    acquire_queue_lock();
    
    if (!ready_queue || !target) {
        release_queue_lock();
        return NULL;
    }
    
    mythread_t *prev = ready_queue;
    mythread_t *current = ready_queue->next;
    
    // Buscar el hilo target
    do {
        if (current == target) {
            // Remover de la lista circular
            if (current == ready_queue && current->next == current) {
                // Único elemento
                ready_queue = NULL;
            } else {
                prev->next = current->next;
                if (current == ready_queue) {
                    ready_queue = prev;
                }
            }
            
            // Actualizar total de tickets si es scheduler de Sorteo
            if (current->attr.sched_policy == MYTHREAD_SCHED_LOTTERY) {
                total_tickets -= current->attr.tickets;
            }
            
            current->next = NULL;
            release_queue_lock();
            return current;
        }
        prev = current;
        current = current->next;
    } while (current != ready_queue->next);
    
    release_queue_lock();
    return NULL;
}

// Función para obtener el siguiente hilo de la cola
static mythread_t *get_next_ready(void) {
    mythread_t *next = select_next_thread();
    if (next) {
        return dequeue_ready(next);
    }
    return NULL;
}

// Implementación del scheduler con timer
static void scheduler_timer_handler(int sig) {
    if (current_thread && current_thread->attr.sched_policy == MYTHREAD_SCHED_RR) {
        mythread_yield();
    }
}

static void setup_scheduler_timer(void) {
    struct sigaction sa;
    struct itimerval timer;
    
    // Configurar el manejador de señal
    sa.sa_handler = scheduler_timer_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);
    
    // Configurar el timer
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = quantum;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = quantum;
    setitimer(ITIMER_REAL, &timer, NULL);
}

// Wrapper para la función del hilo
static void thread_wrapper(void *(*start_routine)(void *), void *arg) {
    // Ejecutar la función del hilo
    start_routine(arg);
    
    // Terminar automáticamente cuando la función retorna
    mythread_exit();
}

// Implementación de mythread_create
int mythread_create(mythread_t *thread, const mythread_attr_t *attr, 
                   void *(*start_routine)(void *), void *arg) {
    if (!thread || !start_routine) return -1;
    
    // Inicializar el scheduler si es la primera vez
    if (!scheduler_initialized) {
        mythread_init_main_thread();
        setup_scheduler_timer();
        scheduler_initialized = 1;
    }
    
    // Allocar stack
    thread->stack = malloc(THREAD_STACK_SIZE);
    if (!thread->stack) return -1;

    // Configurar contexto
    if (getcontext(&thread->context) == -1) {
        free(thread->stack);
        return -1;
    }
    
    thread->context.uc_stack.ss_sp = thread->stack;
    thread->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    thread->context.uc_link = &main_thread.context; // Retornar al hilo principal
    
    // Usar wrapper para manejo automático de terminación
    makecontext(&thread->context, (void (*)(void))thread_wrapper, 2, start_routine, arg);

    thread->id = next_id++;
    thread->state = MYTHREAD_STATE_READY;
    thread->next = NULL;
    
    // Configurar atributos
    if (attr) {
        thread->attr = *attr;
    } else {
        thread->attr = (mythread_attr_t){MYTHREAD_SCHED_RR, 1, 0}; // Default values
    }

    __sync_fetch_and_add(&active_threads, 1);
    enqueue_ready(thread);
    
    return 0;
}

// Implementación de mythread_exit
void mythread_exit(void) {
    if (!current_thread || current_thread == &main_thread) {
        return; // No permitir exit del hilo principal
    }
    
    current_thread->state = MYTHREAD_STATE_TERMINATED;
    __sync_fetch_and_sub(&active_threads, 1);
    
    // Cambiar a otro hilo
    mythread_t *next = get_next_ready();
    if (next) {
        mythread_t *prev = current_thread;
        current_thread = next;
        current_thread->state = MYTHREAD_STATE_RUNNING;
        setcontext(&current_thread->context);
    } else {
        // No hay más hilos, volver al principal
        current_thread = &main_thread;
        setcontext(&main_thread.context);
    }
}

// Implementación de mythread_yield
void mythread_yield(void) {
    if (!current_thread) return;
    
    mythread_t *next = get_next_ready();
    
    if (next && next != current_thread) {
        // Guardar el hilo actual si no ha terminado
        if (current_thread->state == MYTHREAD_STATE_RUNNING) {
            enqueue_ready(current_thread);
        }
        
        mythread_t *prev = current_thread;
        current_thread = next;
        current_thread->state = MYTHREAD_STATE_RUNNING;
        
        if (prev->state != MYTHREAD_STATE_TERMINATED) {
            swapcontext(&prev->context, &current_thread->context);
        } else {
            setcontext(&current_thread->context);
        }
    }
}

// Implementación de mythread_join
int mythread_join(mythread_t *thread) {
    if (!thread || thread == current_thread) return -1;
    
    // Esperar a que el hilo termine usando yield cooperativo
    while (thread->state != MYTHREAD_STATE_TERMINATED) {
        mythread_yield();
    }
    
    // Limpiar recursos
    if (thread->stack) {
        free(thread->stack);
        thread->stack = NULL;
    }
    
    return 0;
}

// Implementación de mythread_detach
int mythread_detach(mythread_t *thread) {
    if (!thread) return -1;
    
    // Marcar como detached (simplificado)
    // En una implementación completa, esto evitaría que join espere
    return 0;
}

// Implementación de mythread_mutex_init
int mythread_mutex_init(mythread_mutex_t *mutex) {
    if (!mutex) return -1;
    mutex->locked = 0;
    mutex->owner = NULL;
    return 0;
}

// Implementación de mythread_mutex_destroy
int mythread_mutex_destroy(mythread_mutex_t *mutex) {
    if (!mutex) return -1;
    mutex->locked = 0;
    mutex->owner = NULL;
    return 0;
}

// Implementación de mythread_mutex_lock
int mythread_mutex_lock(mythread_mutex_t *mutex) {
    if (!mutex) return -1;
    
    while (__sync_lock_test_and_set(&mutex->locked, 1)) {
        mythread_yield(); // Ceder CPU mientras espera
    }
    mutex->owner = current_thread;
    return 0;
}

// Implementación de mythread_mutex_unlock
int mythread_mutex_unlock(mythread_mutex_t *mutex) {
    if (!mutex || mutex->owner != current_thread) return -1;
    
    mutex->owner = NULL;
    __sync_lock_release(&mutex->locked);
    return 0;
}

// Implementación de mythread_mutex_trylock
int mythread_mutex_trylock(mythread_mutex_t *mutex) {
    if (!mutex) return -1;
    
    if (__sync_lock_test_and_set(&mutex->locked, 1)) {
        return -1; // Mutex ya está locked
    }
    mutex->owner = current_thread;
    return 0;
}

// Implementación de mythread_chsched
int mythread_chsched(mythread_t *thread, mythread_sched_t new_sched) {
    if (!thread) return -1;
    
    // Cambiar el scheduler del hilo
    thread->attr.sched_policy = new_sched;
    
    // Si el hilo está en ready queue, podría necesitar reordenamiento
    // Para simplificar, solo cambiamos el atributo
    
    return 0;
}

// Implementación de mythread_get_current_sched
mythread_sched_t mythread_get_current_sched(void) {
    if (current_thread)
        return current_thread->attr.sched_policy;
    return MYTHREAD_SCHED_RR;
}

// Inicialización del hilo principal
void mythread_init_main_thread(void) {
    if (getcontext(&main_thread.context) == -1) {
        perror("getcontext");
        exit(1);
    }
    
    main_thread.id = 0;
    main_thread.state = MYTHREAD_STATE_RUNNING;
    main_thread.attr = (mythread_attr_t){MYTHREAD_SCHED_RR, 1, 0};
    main_thread.stack = NULL; // El hilo principal usa el stack del proceso
    main_thread.next = NULL;
    
    current_thread = &main_thread;
    __sync_fetch_and_add(&active_threads, 1);
}