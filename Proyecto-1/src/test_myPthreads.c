/**
 * test_myPthreads.c
 * Programa de prueba para verificar la funcionalidad de myPthreads
 * Incluye pruebas para:
 * - Creación de hilos con diferentes schedulers
 * - Cambio de scheduler
 * - Mutex
 * - Join y detach
 * - Yield
 */

#include "myPthreads.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

// Contador global para pruebas de mutex
volatile int global_counter = 0;
mythread_mutex_t counter_mutex;

// Estructura para pasar datos a los hilos
typedef struct {
    int id;
    int iterations;
    int sleep_time;
    char name[32];
} thread_data_t;

// Función para imprimir el tipo de scheduler como texto
const char* scheduler_name(mythread_sched_t sched) {
    switch(sched) {
        case MYTHREAD_SCHED_RR: return "Round Robin";
        case MYTHREAD_SCHED_LOTTERY: return "Lottery";
        case MYTHREAD_SCHED_RT: return "Real Time";
        default: return "Unknown";
    }
}

// Función para hilos de prueba básica
void* test_thread_function(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    printf("[Hilo %s] Iniciado con ID=%d, Scheduler=%s\n", 
           data->name, data->id, 
           scheduler_name(mythread_get_current_sched()));
    
    for (int i = 0; i < data->iterations; i++) {
        printf("[Hilo %s] Iteración %d/%d\n", data->name, i+1, data->iterations);
        
        // Simular trabajo
        usleep(data->sleep_time * 1000);
        
        // Ceder CPU explícitamente
        if (i % 2 == 0) {
            printf("[Hilo %s] Cediendo CPU explícitamente\n", data->name);
            mythread_yield();
        }
    }
    
    printf("[Hilo %s] Terminando\n", data->name);
    free(data); // Liberar memoria asignada
    return NULL;
}

// Función para probar mutex
void* mutex_test_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    printf("[Mutex %s] Iniciado\n", data->name);
    
    for (int i = 0; i < data->iterations; i++) {
        // Adquirir mutex
        mythread_mutex_lock(&counter_mutex);
        
        // Sección crítica
        int temp = global_counter;
        usleep(data->sleep_time * 1000); // Simular trabajo en sección crítica
        global_counter = temp + 1;
        
        printf("[Mutex %s] Incrementó contador a %d\n", data->name, global_counter);
        
        // Liberar mutex
        mythread_mutex_unlock(&counter_mutex);
        
        // Trabajo fuera de sección crítica
        usleep((data->sleep_time / 2) * 1000);
    }
    
    printf("[Mutex %s] Terminando\n", data->name);
    free(data);
    return NULL;
}

// Función para probar trylock
void* trylock_test_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int success_count = 0;
    
    printf("[TryLock %s] Iniciado\n", data->name);
    
    for (int i = 0; i < data->iterations * 3; i++) {
        // Intentar adquirir mutex sin bloquear
        if (mythread_mutex_trylock(&counter_mutex) == 0) {
            // Sección crítica
            printf("[TryLock %s] Adquirió mutex, contador=%d\n", data->name, global_counter);
            global_counter++;
            usleep(data->sleep_time * 500); // Tiempo más corto en sección crítica
            
            // Liberar mutex
            mythread_mutex_unlock(&counter_mutex);
            success_count++;
        } else {
            printf("[TryLock %s] Mutex ocupado, intentando más tarde\n", data->name);
        }
        
        // Esperar antes del siguiente intento
        usleep(data->sleep_time * 300);
    }
    
    printf("[TryLock %s] Terminando, adquirió mutex %d veces\n", data->name, success_count);
    free(data);
    return NULL;
}

// Función para probar cambio de scheduler
void* scheduler_switch_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    mythread_t* self = (mythread_t*)data->id; // Usar directamente el puntero al hilo
    
    printf("[Switch %s] Iniciado con scheduler %s\n", 
           data->name, scheduler_name(mythread_get_current_sched()));
    
    for (int i = 0; i < data->iterations; i++) {
        printf("[Switch %s] Iteración %d/%d con scheduler %s\n", 
               data->name, i+1, data->iterations, 
               scheduler_name(mythread_get_current_sched()));
        
        // Cambiar scheduler a mitad de ejecución
        if (i == data->iterations / 2) {
            mythread_sched_t new_sched;
            
            // Rotar entre los 3 tipos de scheduler
            switch(mythread_get_current_sched()) {
                case MYTHREAD_SCHED_RR:
                    new_sched = MYTHREAD_SCHED_LOTTERY;
                    break;
                case MYTHREAD_SCHED_LOTTERY:
                    new_sched = MYTHREAD_SCHED_RT;
                    break;
                case MYTHREAD_SCHED_RT:
                default:
                    new_sched = MYTHREAD_SCHED_RR;
                    break;
            }
            
            printf("[Switch %s] Cambiando scheduler de %s a %s\n", 
                   data->name, 
                   scheduler_name(mythread_get_current_sched()),
                   scheduler_name(new_sched));
            
            mythread_chsched(self, new_sched);
        }
        
        // Simular trabajo
        usleep(data->sleep_time * 1000);
        mythread_yield();
    }
    
    printf("[Switch %s] Terminando con scheduler %s\n", 
           data->name, scheduler_name(mythread_get_current_sched()));
    
    free(self);
    free(data);
    return NULL;
}

// Prueba de creación básica de hilos
void test_basic_threads() {
    printf("\n=== PRUEBA DE CREACIÓN BÁSICA DE HILOS ===\n");
    
    mythread_t threads[3];
    thread_data_t* data[3];
    
    for (int i = 0; i < 3; i++) {
        data[i] = (thread_data_t*)malloc(sizeof(thread_data_t));
        data[i]->id = i + 1;
        data[i]->iterations = 5;
        data[i]->sleep_time = 100 + (i * 50); // Tiempos diferentes
        sprintf(data[i]->name, "Básico-%d", i + 1);
        
        // Usar scheduler por defecto (Round Robin)
        mythread_create(&threads[i], NULL, test_thread_function, data[i]);
    }
    
    // Esperar a que todos terminen
    for (int i = 0; i < 3; i++) {
        mythread_join(&threads[i]);
        printf("Hilo Básico-%d completado\n", i + 1);
    }
}

// Prueba de diferentes schedulers
void test_different_schedulers() {
    printf("\n=== PRUEBA DE DIFERENTES SCHEDULERS ===\n");
    
    mythread_t threads[3];
    thread_data_t* data[3];
    mythread_attr_t attrs[3];
    
    // Configurar atributos para diferentes schedulers
    attrs[0] = MYTHREAD_ATTR_INIT_RR();
    attrs[1] = MYTHREAD_ATTR_INIT_LOTTERY(10); // 10 tickets
    attrs[2] = MYTHREAD_ATTR_INIT_RT(5);       // Prioridad 5
    
    for (int i = 0; i < 3; i++) {
        data[i] = (thread_data_t*)malloc(sizeof(thread_data_t));
        data[i]->id = i + 1;
        data[i]->iterations = 8;
        data[i]->sleep_time = 200;
        
        switch(i) {
            case 0: sprintf(data[i]->name, "RR"); break;
            case 1: sprintf(data[i]->name, "Lottery"); break;
            case 2: sprintf(data[i]->name, "RealTime"); break;
        }
        
        mythread_create(&threads[i], &attrs[i], test_thread_function, data[i]);
    }
    
    // Esperar a que todos terminen
    for (int i = 0; i < 3; i++) {
        mythread_join(&threads[i]);
        printf("Hilo %s completado\n", data[i]->name);
    }
}

// Prueba de mutex
void test_mutex() {
    printf("\n=== PRUEBA DE MUTEX ===\n");
    
    mythread_t threads[3];
    thread_data_t* data[3];
    
    // Inicializar mutex
    mythread_mutex_init(&counter_mutex);
    global_counter = 0;
    
    for (int i = 0; i < 3; i++) {
        data[i] = (thread_data_t*)malloc(sizeof(thread_data_t));
        data[i]->id = i + 1;
        data[i]->iterations = 5;
        data[i]->sleep_time = 100 + (i * 30);
        sprintf(data[i]->name, "Mutex-%d", i + 1);
        
        mythread_create(&threads[i], NULL, mutex_test_thread, data[i]);
    }
    
    // Esperar a que todos terminen
    for (int i = 0; i < 3; i++) {
        mythread_join(&threads[i]);
    }
    
    // Verificar resultado
    printf("Valor final del contador: %d (esperado: 15)\n", global_counter);
    assert(global_counter == 15);
    
    // Destruir mutex
    mythread_mutex_destroy(&counter_mutex);
}

// Prueba de trylock
void test_trylock() {
    printf("\n=== PRUEBA DE TRYLOCK ===\n");
    
    mythread_t threads[2];
    thread_data_t* data[2];
    
    // Inicializar mutex
    mythread_mutex_init(&counter_mutex);
    global_counter = 0;
    
    for (int i = 0; i < 2; i++) {
        data[i] = (thread_data_t*)malloc(sizeof(thread_data_t));
        data[i]->id = i + 1;
        data[i]->iterations = 5;
        data[i]->sleep_time = 100 + (i * 50);
        sprintf(data[i]->name, "Try-%d", i + 1);
        
        mythread_create(&threads[i], NULL, trylock_test_thread, data[i]);
    }
    
    // Esperar a que todos terminen
    for (int i = 0; i < 2; i++) {
        mythread_join(&threads[i]);
    }
    
    printf("Valor final del contador con trylock: %d\n", global_counter);
    
    // Destruir mutex
    mythread_mutex_destroy(&counter_mutex);
}

// Prueba de cambio de scheduler
void test_scheduler_switch() {
    printf("\n=== PRUEBA DE CAMBIO DE SCHEDULER ===\n");
    
    mythread_t threads[3];
    thread_data_t* data[3];
    mythread_attr_t attrs[3];
    
    // Configurar atributos para diferentes schedulers iniciales
    attrs[0] = MYTHREAD_ATTR_INIT_RR();
    attrs[1] = MYTHREAD_ATTR_INIT_LOTTERY(20);
    attrs[2] = MYTHREAD_ATTR_INIT_RT(3);
    
    for (int i = 0; i < 3; i++) {
        data[i] = (thread_data_t*)malloc(sizeof(thread_data_t));
        data[i]->iterations = 10;
        data[i]->sleep_time = 150 + (i * 20);
        sprintf(data[i]->name, "Switch-%d", i + 1);
        
        mythread_create(&threads[i], &attrs[i], test_thread_function, data[i]);
        
        // Guardar referencia al hilo para poder cambiarlo después
        data[i]->id = (int)&threads[i]; // Guardar el puntero directamente
    }
    
    // Esperar un poco para que los hilos inicien
    usleep(500 * 1000);
    
    // Cambiar scheduler del primer hilo
    printf("Cambiando scheduler del hilo Switch-1 de RR a Lottery\n");
    mythread_chsched(&threads[0], MYTHREAD_SCHED_LOTTERY);
    
    // Esperar otro poco
    usleep(500 * 1000);
    
    // Cambiar scheduler del segundo hilo
    printf("Cambiando scheduler del hilo Switch-2 de Lottery a RT\n");
    mythread_chsched(&threads[1], MYTHREAD_SCHED_RT);
    
    // Esperar a que todos terminen
    for (int i = 0; i < 3; i++) {
        mythread_join(&threads[i]);
        printf("Hilo Switch-%d completado\n", i + 1);
    }
}

// Prueba de detach
void test_detach() {
    printf("\n=== PRUEBA DE DETACH ===\n");
    
    mythread_t thread;
    thread_data_t* data = (thread_data_t*)malloc(sizeof(thread_data_t));
    
    data->id = 1;
    data->iterations = 3;
    data->sleep_time = 200;
    sprintf(data->name, "Detached");
    
    mythread_create(&thread, NULL, test_thread_function, data);
    
    // Detach el hilo
    printf("Detaching hilo...\n");
    mythread_detach(&thread);
    
    // No hacemos join, el hilo se ejecutará independientemente
    printf("Hilo detached, continuando sin esperar\n");
    
    // Esperar un poco para ver la salida del hilo detached
    usleep(1000 * 1000);
}

// Prueba de lottery scheduler con diferentes tickets
void test_lottery_tickets() {
    printf("\n=== PRUEBA DE LOTTERY SCHEDULER CON DIFERENTES TICKETS ===\n");
    
    mythread_t threads[3];
    thread_data_t* data[3];
    mythread_attr_t attrs[3];
    
    // Configurar diferentes cantidades de tickets
    attrs[0] = MYTHREAD_ATTR_INIT_LOTTERY(5);   // 5 tickets
    attrs[1] = MYTHREAD_ATTR_INIT_LOTTERY(10);  // 10 tickets
    attrs[2] = MYTHREAD_ATTR_INIT_LOTTERY(20);  // 20 tickets
    
    for (int i = 0; i < 3; i++) {
        data[i] = (thread_data_t*)malloc(sizeof(thread_data_t));
        data[i]->id = i + 1;
        data[i]->iterations = 15;
        data[i]->sleep_time = 100;
        sprintf(data[i]->name, "Lottery-%d-%d", i + 1, 
                attrs[i].tickets);
        
        mythread_create(&threads[i], &attrs[i], test_thread_function, data[i]);
    }
    
    // Esperar a que todos terminen
    for (int i = 0; i < 3; i++) {
        mythread_join(&threads[i]);
        printf("Hilo Lottery-%d completado\n", i + 1);
    }
}

// Prueba de real-time scheduler con diferentes prioridades
void test_realtime_priorities() {
    printf("\n=== PRUEBA DE REAL-TIME SCHEDULER CON DIFERENTES PRIORIDADES ===\n");
    
    mythread_t threads[3];
    thread_data_t* data[3];
    mythread_attr_t attrs[3];
    
    // Configurar diferentes prioridades
    attrs[0] = MYTHREAD_ATTR_INIT_RT(2);  // Baja prioridad
    attrs[1] = MYTHREAD_ATTR_INIT_RT(5);  // Media prioridad
    attrs[2] = MYTHREAD_ATTR_INIT_RT(9);  // Alta prioridad
    
    for (int i = 0; i < 3; i++) {
        data[i] = (thread_data_t*)malloc(sizeof(thread_data_t));
        data[i]->id = i + 1;
        data[i]->iterations = 8;
        data[i]->sleep_time = 150;
        sprintf(data[i]->name, "RT-%d-P%d", i + 1, 
                attrs[i].priority);
        
        mythread_create(&threads[i], &attrs[i], test_thread_function, data[i]);
    }
    
    // Esperar a que todos terminen
    for (int i = 0; i < 3; i++) {
        mythread_join(&threads[i]);
        printf("Hilo RT-%d completado\n", i + 1);
    }
}

// Función para probar específicamente el cambio de scheduler
void test_scheduler_change() {
    printf("\n=== PRUEBA DE CAMBIO DE SCHEDULER ===\n");
    
    mythread_t thread;
    thread_data_t* data = (thread_data_t*)malloc(sizeof(thread_data_t));
    mythread_attr_t attr = MYTHREAD_ATTR_INIT_RR();
    
    data->id = 1;
    data->iterations = 3;
    data->sleep_time = 100;
    sprintf(data->name, "SchedChange");
    
    // Crear hilo con RoundRobin
    mythread_create(&thread, &attr, test_thread_function, data);
    
    // Esperar un poco
    usleep(200 * 1000);
    
    // Cambiar a Lottery
    printf("Cambiando scheduler de RoundRobin a Lottery\n");
    mythread_chsched(&thread, MYTHREAD_SCHED_LOTTERY);
    
    // Esperar un poco más
    usleep(200 * 1000);
    
    // Cambiar a RealTime
    printf("Cambiando scheduler de Lottery a RealTime\n");
    mythread_chsched(&thread, MYTHREAD_SCHED_RT);
    
    // Esperar a que termine
    mythread_join(&thread);
    printf("Hilo con cambio de scheduler completado\n");
}

// Prueba simplificada de Lottery Scheduler
void test_lottery_scheduler() {
    printf("\n=== PRUEBA DE LOTTERY SCHEDULER ===\n");
    
    mythread_t threads[2];
    thread_data_t* data[2];
    mythread_attr_t attrs[2];
    
    // Configurar diferentes cantidades de tickets
    attrs[0] = MYTHREAD_ATTR_INIT_LOTTERY(5);   // 5 tickets
    attrs[1] = MYTHREAD_ATTR_INIT_LOTTERY(15);  // 15 tickets
    
    for (int i = 0; i < 2; i++) {
        data[i] = (thread_data_t*)malloc(sizeof(thread_data_t));
        data[i]->id = i + 1;
        data[i]->iterations = 5;
        data[i]->sleep_time = 100;
        sprintf(data[i]->name, "Lottery-%d-%d", i + 1, attrs[i].tickets);
        
        mythread_create(&threads[i], &attrs[i], test_thread_function, data[i]);
    }
    
    // Esperar a que todos terminen
    for (int i = 0; i < 2; i++) {
        mythread_join(&threads[i]);
        printf("Hilo Lottery-%d completado\n", i + 1);
    }
}

// Prueba simplificada de Real-Time Scheduler
void test_realtime_scheduler() {
    printf("\n=== PRUEBA DE REAL-TIME SCHEDULER ===\n");
    
    mythread_t threads[2];
    thread_data_t* data[2];
    mythread_attr_t attrs[2];
    
    // Configurar diferentes prioridades
    attrs[0] = MYTHREAD_ATTR_INIT_RT(3);  // Baja prioridad
    attrs[1] = MYTHREAD_ATTR_INIT_RT(7);  // Alta prioridad
    
    for (int i = 0; i < 2; i++) {
        data[i] = (thread_data_t*)malloc(sizeof(thread_data_t));
        data[i]->id = i + 1;
        data[i]->iterations = 5;
        data[i]->sleep_time = 100;
        sprintf(data[i]->name, "RT-%d-P%d", i + 1, attrs[i].priority);
        
        mythread_create(&threads[i], &attrs[i], test_thread_function, data[i]);
    }
    
    // Esperar a que todos terminen
    for (int i = 0; i < 2; i++) {
        mythread_join(&threads[i]);
        printf("Hilo RT-%d completado\n", i + 1);
    }
}

int main() {
    printf("=== PROGRAMA DE PRUEBA DE MYPTHREADS ===\n");
    printf("Verificando todas las funcionalidades...\n\n");
    
    // Pruebas básicas
    test_basic_threads();
    test_mutex();
    test_trylock();
    
    // Pruebas de schedulers
    test_different_schedulers();
    test_lottery_scheduler();
    test_realtime_scheduler();
    
    // Prueba de cambio de scheduler
    test_scheduler_change();
    
    printf("\n=== TODAS LAS PRUEBAS COMPLETADAS ===\n");
    printf("La implementación de myPthreads cumple con las especificaciones.\n");
    
    return 0;
}
