#include "mypthread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <bits/types/timer_t.h>

// Variables globales del sistema
static mythread_t *current_thread = NULL;
static mythread_t *ready_queue = NULL;
static mythread_t main_thread;
static int next_id = 1;
static ucontext_t scheduler_context;
static mythread_mutex_t thread_table_lock;

// Función auxiliar para agregar un hilo a la cola de listos
static void _enqueue_ready(mythread_t *thread)
{
    mythread_mutex_lock(&thread_table_lock);

    thread->state = MYTHREAD_STATE_READY;
    if (ready_queue == NULL)
    {
        ready_queue = thread;
        thread->next = thread; // Lista circular
    }
    else
    {
        thread->next = ready_queue->next;
        ready_queue->next = thread;
        ready_queue = thread;
    }

    mythread_mutex_unlock(&thread_table_lock);
}

// Función auxiliar para obtener el siguiente hilo de la cola de listos
static mythread_t *_dequeue_ready(void)
{
    mythread_mutex_lock(&thread_table_lock);

    if (ready_queue == NULL)
    {
        mythread_mutex_unlock(&thread_table_lock);
        return NULL;
    }

    mythread_t *next = ready_queue->next;
    if (next == ready_queue)
    {
        ready_queue = NULL;
    }
    else
    {
        ready_queue->next = next->next;
    }

    mythread_mutex_unlock(&thread_table_lock);
    return next;
}

// Implementación de mythread_create
int mythread_create(mythread_t *thread, const mythread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg, int sched_policy)
{
    // Inicializar sistema si es la primera vez
    static int initialized = 0;
    if (!initialized)
    {
        _mythread_system_init();
        initialized = 1;
    }

    // Asignar stack
    thread->stack = malloc(THREAD_STACK_SIZE);
    if (thread->stack == NULL)
    {
        return -1;
    }

    // Inicializar contexto
    if (getcontext(&thread->context) == -1)
    {
        free(thread->stack);
        return -1;
    }

    thread->context.uc_stack.ss_sp = thread->stack;
    thread->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    thread->context.uc_link = &scheduler_context;

    // Configurar atributos
    if (attr != NULL)
    {
        thread->attr = *attr;
    }
    else
    {
        // Valores por defecto
        thread->attr.detachstate = MYTHREAD_CREATE_JOINABLE;
        thread->attr.sched_policy = sched_policy; // Usar el scheduler especificado
        thread->attr.sched_priority = 0;
        thread->attr.tickets = (sched_policy == MYTHREAD_SCHED_LOTTERY) ? 10 : 0; // Default para lottery
    }

    // Configurar función a ejecutar
    makecontext(&thread->context, (void (*)(void))start_routine, 1, arg);

    // Configurar estado y TID
    thread->state = MYTHREAD_STATE_NEW;
    thread->id = next_id++;
    thread->joined_by = -1;
    thread->retval = NULL;
    thread->next = NULL;

    // Agregar a la cola de listos
    _enqueue_ready(thread);

    return 0;
}

// Implementación de mythread_exit
void mythread_exit(void *retval)
{
    mythread_t *self = current_thread;

    // Guardar valor de retorno
    self->retval = retval;
    self->state = MYTHREAD_STATE_TERMINATED;

    // Notificar al hilo que está esperando (si hay)
    if (self->joined_by != -1)
    {
        mythread_t *joiner = _find_thread_by_tid(self->joined_by);
        if (joiner != NULL)
        {
            _enqueue_ready(joiner);
        }
    }

    // Si es detached, liberar recursos
    if (self->attr.detachstate == MYTHREAD_CREATE_DETACHED)
    {
        free(self->stack);
    }

    // Cambiar al siguiente hilo
    _mythread_schedule();
}

// Implementación de mythread_join
int mythread_join(mythread_t thread, void **retval)
{
    mythread_t *self = current_thread;
    mythread_t *target = _find_thread_by_tid(thread.id);

    if (target == NULL || target->state == MYTHREAD_STATE_TERMINATED)
    {
        return -1; // Hilo no existe o ya terminó
    }

    if (target->attr.detachstate == MYTHREAD_CREATE_DETACHED)
    {
        return -1; // No se puede hacer join a un hilo detached
    }

    // Configurar que estamos esperando este hilo
    target->joined_by = self->id;
    self->state = MYTHREAD_STATE_BLOCKED;

    // Cambiar al siguiente hilo
    _mythread_schedule();

    // Cuando volvamos aquí, el hilo objetivo ha terminado
    if (retval != NULL)
    {
        *retval = target->retval;
    }

    // Liberar recursos del hilo terminado
    free(target->stack);

    return 0;
}

// Implementación de mythread_detach
int mythread_detach(mythread_t thread)
{
    mythread_mutex_lock(&thread_table_lock);

    mythread_t *target = _find_thread_by_tid(thread.id);
    if (target == NULL || target->state == MYTHREAD_STATE_TERMINATED)
    {
        mythread_mutex_unlock(&thread_table_lock);
        return -1;
    }

    target->attr.detachstate = MYTHREAD_CREATE_DETACHED;

    // Si ya terminó, liberar recursos
    if (target->state == MYTHREAD_STATE_TERMINATED)
    {
        free(target->stack);
    }

    mythread_mutex_unlock(&thread_table_lock);
    return 0;
}

// Implementación de mythread_yield
void mythread_yield(void)
{
    mythread_t *self = current_thread;

    // Volver a la cola de listos
    _enqueue_ready(self);

    // Cambiar al siguiente hilo
    _mythread_schedule();
}

// Función auxiliar para encontrar un hilo por TID
static mythread_t *_find_thread_by_tid(int id)
{
    if (id == 0)
        return &main_thread;

    mythread_mutex_lock(&thread_table_lock);

    mythread_t *found = NULL;
    mythread_t *t = ready_queue;

    if (t != NULL)
    {
        do
        {
            if (t->id == id)
            {
                found = t;
                break;
            }
            t = t->next;
        } while (t != ready_queue);
    }

    // También verificar el hilo actual (puede no estar en ready_queue)
    if (current_thread != NULL && current_thread->id == id)
    {
        found = current_thread;
    }

    mythread_mutex_unlock(&thread_table_lock);
    return found;
}

// Función de scheduling (simplificada, se ampliará en el paso 3)
static void _mythread_schedule(void)
{
    mythread_t *next = _dequeue_ready();
    if (next == NULL)
    {
        // No hay más hilos, volver al main
        setcontext(&main_thread.context);
    }

    mythread_t *prev = current_thread;
    current_thread = next;
    next->state = MYTHREAD_STATE_RUNNING;

    // Cambiar contexto
    swapcontext(&prev->context, &next->context);
}

// Inicialización del sistema
static void _mythread_system_init(void)
{
    // Inicializar el hilo principal (main)
    getcontext(&main_thread.context);
    main_thread.id = 0;
    main_thread.state = MYTHREAD_STATE_RUNNING;
    main_thread.stack = NULL;
    main_thread.next = NULL;

    current_thread = &main_thread;

    // Inicializar mutex
    mythread_mutex_init(&thread_table_lock);

    // Configurar contexto del scheduler
    getcontext(&scheduler_context);
    scheduler_context.uc_stack.ss_sp = malloc(THREAD_STACK_SIZE);
    scheduler_context.uc_stack.ss_size = THREAD_STACK_SIZE;
    scheduler_context.uc_link = &main_thread.context;
    // makecontext(&scheduler_context, _mythread_scheduler, 0); (Falta)
}

// Implementación de cambio de política de scheduling
int mythread_chsched(mythread_t *thread, int new_sched_policy)
{
    mythread_mutex_lock(&thread_table_lock);

    mythread_t *target = _find_thread_by_tid(thread->id);
    if (target == NULL)
    {
        mythread_mutex_unlock(&thread_table_lock);
        return -1; // Hilo no encontrado
    }

    // Cambiar política de scheduling
    target->attr.sched_policy = new_sched_policy;

    // Configurar parámetros adicionales según el scheduler
    if (new_sched_policy == MYTHREAD_SCHED_LOTTERY)
    {
        target->attr.tickets = 10; // Valor por defecto
    }
    else if (new_sched_policy == MYTHREAD_SCHED_RT)
    {
        target->attr.sched_priority = 1; // Prioridad por defecto
    }

    mythread_mutex_unlock(&thread_table_lock);
    return 0;
}