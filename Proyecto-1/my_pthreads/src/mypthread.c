#include "mypthread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <linux/time.h>

// Variables globales
static mythread_t *current_thread = NULL;
static mythread_t *ready_queue = NULL;
static int next_id = 1;
static int quantum = 1000000; // Tiempo en microsegundos para RoundRobin

// Contador global de hilos activos
static int active_threads = 0;

// Hilo principal
static mythread_t main_thread;

// Variables para el scheduler de Sorteo
static int total_tickets = 0;
static int random_seed_initialized = 0;

// Variables para el scheduler de Tiempo Real
static struct timespec start_time;
static int rt_priority = 1;

// Función auxiliar para agregar un hilo a la cola de listos
static void enqueue_ready(mythread_t *thread) {
    thread->state = MYTHREAD_STATE_READY;
    if (!ready_queue) {
        ready_queue = thread;
        thread->next = thread;
    } else {
        thread->next = ready_queue->next;
        ready_queue->next = thread;
        ready_queue = thread;
    }
    
    // Actualizar total de tickets para scheduler de Sorteo
    if (thread->attr.sched_policy == MYTHREAD_SCHED_LOTTERY) {
        total_tickets += thread->attr.tickets;
    }
}

// Función auxiliar para obtener el siguiente hilo de la cola de listos
static mythread_t *dequeue_ready(void) {
    if (!ready_queue) return NULL;
    
    // Seleccionar el siguiente hilo según el scheduler
    mythread_t *next = NULL;
    switch (current_thread->attr.sched_policy) {
        case MYTHREAD_SCHED_RR:
            next = ready_queue->next;
            break;
            
        case MYTHREAD_SCHED_LOTTERY:
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
    
    if (next) {
        if (next == ready_queue) {
            ready_queue = NULL;
        } else {
            ready_queue->next = next->next;
        }
        
        // Actualizar total de tickets si es scheduler de Sorteo
        if (next->attr.sched_policy == MYTHREAD_SCHED_LOTTERY) {
            total_tickets -= next->attr.tickets;
        }
    }
    
    return next;
}

// Implementación de mythread_create
int mythread_create(mythread_t *thread, const mythread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
    thread->stack = malloc(THREAD_STACK_SIZE);
    if (!thread->stack) return -1;

    getcontext(&thread->context);
    thread->context.uc_stack.ss_sp = thread->stack;
    thread->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    thread->context.uc_link = NULL;
    makecontext(&thread->context, (void (*)(void))start_routine, 1, arg);

    thread->id = next_id++;
    thread->state = MYTHREAD_STATE_READY;
    thread->attr = attr ? *attr : (mythread_attr_t){MYTHREAD_SCHED_RR, 0, 0};

    active_threads++; // Incrementar el contador de hilos activos
    enqueue_ready(thread);
    return 0;
}

// Implementación de mythread_exit
void mythread_exit(void) {
    if (current_thread) {
        current_thread->state = MYTHREAD_STATE_TERMINATED;
        active_threads--; // Reducir el contador de hilos activos
        mythread_yield();
    }
}

// Implementación de mythread_yield
void mythread_yield(void) {
    if (current_thread && current_thread->state != MYTHREAD_STATE_TERMINATED) {
        // Para RoundRobin, usar quantum
        if (current_thread->attr.sched_policy == MYTHREAD_SCHED_RR) {
            usleep(quantum); // Esperar el quantum antes de ceder
        }
        enqueue_ready(current_thread); // Reinsertar el hilo actual si no ha terminado
    }
    
    // Obtener el siguiente hilo según el scheduler
    mythread_t *next = dequeue_ready();
    
    if (next) {
        // Si estamos cambiando de scheduler, inicializar el nuevo scheduler
        if (next->attr.sched_policy != current_thread->attr.sched_policy) {
            switch (next->attr.sched_policy) {
                case MYTHREAD_SCHED_RR:
                    // No hay inicialización necesaria
                    break;
                    
                case MYTHREAD_SCHED_LOTTERY:
                    if (!random_seed_initialized) {
                        srand(time(NULL));
                        random_seed_initialized = 1;
                    }
                    break;
                    
                case MYTHREAD_SCHED_RT:
                    clock_gettime(CLOCK_MONOTONIC, &start_time);
                    break;
            }
        }
        
        mythread_t *prev = current_thread;
        current_thread = next;
        current_thread->state = MYTHREAD_STATE_RUNNING;
        
        if (prev && prev->state != MYTHREAD_STATE_TERMINATED) {
            swapcontext(&prev->context, &current_thread->context);
        } else {
            setcontext(&current_thread->context);
        }
    } else if (active_threads > 0) {
        // Si no hay hilos listos pero aún hay hilos activos, esperar
        while (active_threads > 0) {
            usleep(1000); // Esperar brevemente para evitar un bucle ocupado
        }
    }
}

// Implementación de mythread_join
int mythread_join(mythread_t *thread) {
    if (!thread) return -1;
    
    // Esperar a que el hilo termine
    while (thread->state != MYTHREAD_STATE_TERMINATED) {
        mythread_yield();
        // Agregar un pequeño delay para evitar un bucle ocupado
        usleep(1000);
    }
    
    // Limpiar recursos solo si el hilo no es el hilo principal
    if (thread != &main_thread) {
        if (thread->stack) {
            free(thread->stack);
            thread->stack = NULL;
        }
    }
    
    return 0;
}

// Implementación de mythread_detach
int mythread_detach(mythread_t *thread) {
    if (!thread) return -1;
    
    // Marcar el hilo como detached
    thread->attr.sched_policy = MYTHREAD_SCHED_RR; // Forzar RoundRobin para detached
    
    // No necesitamos hacer nada más, el hilo ya está en la cola de listos
    return 0;
}

// Implementación de mythread_mutex_init
int mythread_mutex_init(mythread_mutex_t *mutex) {
    mutex->locked = 0;
    mutex->owner = NULL;
    return 0;
}

// Implementación de mythread_mutex_destroy
int mythread_mutex_destroy(mythread_mutex_t *mutex) {
    mutex->locked = 0;
    mutex->owner = NULL;
    return 0;
}

// Implementación de mythread_mutex_lock
int mythread_mutex_lock(mythread_mutex_t *mutex) {
    while (__sync_lock_test_and_set(&mutex->locked, 1)) {
        mythread_yield();
    }
    mutex->owner = current_thread;
    return 0;
}

// Implementación de mythread_mutex_unlock
int mythread_mutex_unlock(mythread_mutex_t *mutex) {
    if (mutex->owner != current_thread) return -1;
    mutex->owner = NULL;
    __sync_lock_release(&mutex->locked);
    return 0;
}

// Implementación de mythread_mutex_trylock
int mythread_mutex_trylock(mythread_mutex_t *mutex) {
    if (__sync_lock_test_and_set(&mutex->locked, 1)) return -1;
    mutex->owner = current_thread;
    return 0;
}

// Implementación de mythread_chsched
int mythread_chsched(mythread_t *thread, mythread_sched_t new_sched)
{
    if (!thread) return -1;
    
    // Si el hilo está en la cola de listos, necesitamos actualizar su posición
    if (thread->state == MYTHREAD_STATE_READY) {
        // Quitar el hilo de la cola
        if (thread == ready_queue) {
            ready_queue = NULL;
        } else {
            mythread_t *current = ready_queue->next;
            do {
                if (current->next == thread) {
                    current->next = thread->next;
                    if (thread == ready_queue) {
                        ready_queue = current;
                    }
                    break;
                }
                current = current->next;
            } while (current != ready_queue->next);
        }
        
        // Actualizar el scheduler y reinsertar el hilo
        thread->attr.sched_policy = new_sched;
        enqueue_ready(thread);
    } else {
        // Si el hilo está corriendo o terminado, solo cambiar el scheduler
        thread->attr.sched_policy = new_sched;
    }
    
    return 0;
}

// Implementación de mythread_get_current_sched
mythread_sched_t mythread_get_current_sched(void) {
    if (current_thread)
        return current_thread->attr.sched_policy;
    return MYTHREAD_SCHED_RR; // Valor por defecto si no hay hilo actual
}

// Inicialización del hilo principal
void mythread_init_main_thread(void) {
    getcontext(&main_thread.context);
    current_thread = &main_thread;
    main_thread.state = MYTHREAD_STATE_RUNNING;
    main_thread.id = 0;
    main_thread.attr = (mythread_attr_t){MYTHREAD_SCHED_RR, 0, 0};
    active_threads++; // Contar el hilo principal como activo
}