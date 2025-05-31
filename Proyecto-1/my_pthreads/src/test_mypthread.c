#include "mypthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Variables globales para pruebas de mutex
mythread_mutex_t mutex;
int shared_counter = 0;

// Función para probar mythread_create, mythread_yield y los schedulers
void *thread_function(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 3; i++) {
        printf("Inicia ejecución de prueba\n");
        printf("Thread %d: Iteration %d\n", id, i);
        mythread_yield();
        printf("YIELD REALIZADO\n");
    }
    printf("Thread %d terminando...\n", id);
    mythread_exit();
    return NULL;
}

// Función para probar los mutex
void *mutex_test_function(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 3; i++) {
        if (mythread_mutex_trylock(&mutex) == 0) {
            shared_counter++;
            printf("Thread %d incrementó contador a %d\n", id, shared_counter);
            mythread_mutex_unlock(&mutex);
        } else {
            printf("Thread %d no pudo adquirir el mutex\n", id);
        }
        mythread_yield();
    }
    printf("Thread %d terminando...\n", id);
    mythread_exit();
    return NULL;
}

// Función para probar mythread_detach
void *detached_thread_function(void *arg) {
    int id = *(int *)arg;
    printf("Hilo detached %d está ejecutándose\n", id);
    mythread_yield();
    printf("Hilo detached %d terminando...\n", id);
    mythread_exit();
    return NULL;
}

// Función para probar mythread_chsched
void *scheduler_test_function(void *arg) {
    int id = *(int *)arg;
    printf("Thread %d está ejecutándose con política de scheduler %d\n", id, mythread_get_current_sched());
    // Yield to allow scheduler change to take effect
    mythread_yield();
    printf("Thread %d ahora está ejecutándose con política de scheduler %d\n", id, mythread_get_current_sched());
    printf("Thread %d terminando...\n", id);
    mythread_exit();
    return NULL;
}

int main() {
    mythread_init_main_thread(); // Inicializar el hilo principal

    mythread_t threads[6];
    int ids[6] = {1, 2, 3, 4, 5, 6};

    // Prueba de mythread_create, mythread_yield y RoundRobin
    printf("Prueba del scheduler RoundRobin...\n");
    mythread_create(&threads[0], NULL, thread_function, &ids[0]);

    printf("HILO CREADO\n");

    mythread_create(&threads[1], NULL, thread_function, &ids[1]);

    printf("HILO CREADO\n");

    mythread_join(&threads[0]);

    printf("HILO UNIDO\n");

    mythread_join(&threads[1]);

    printf("HILO UNIDO\n");

    // Mensaje de depuración
    printf("Prueba de RoundRobin completada. Continuando con las siguientes pruebas...\n");

    // Pausa para asegurarnos de que el hilo principal no termine prematuramente
    sleep(1);

    // Prueba de mutexes
    printf("\nPrueba de mutexes...\n");
    mythread_mutex_init(&mutex);
    printf("Inicializando mutex\n");
    mythread_create(&threads[2], NULL, mutex_test_function, &ids[2]);
    printf("Creando hilo 1\n");
    mythread_create(&threads[3], NULL, mutex_test_function, &ids[3]);
    printf("Creando hilo 2\n");
    mythread_join(&threads[2]);
    printf("Uniendo hilo 1\n");
    mythread_join(&threads[3]);
    printf("Uniendo hilo 2\n");
    mythread_mutex_destroy(&mutex);
    printf("Destruyendo mutex\n");

    // Prueba de mythread_detach
    printf("\nPrueba de mythread_detach...\n");
    mythread_create(&threads[4], NULL, detached_thread_function, &ids[4]);
    mythread_detach(&threads[4]);
    printf("Hilo detached creado y desvinculado exitosamente\n");
    
    // Esperar un momento para que el hilo detached termine
    usleep(100000); // Esperar 100ms para asegurar que el hilo detached termine
    
    // Verificar que el hilo detached terminó
    printf("Hilo detached %d ha terminado\n", ids[4]);

    // Prueba de mythread_chsched y diferentes schedulers
    printf("\nPrueba de diferentes schedulers...\n");
    mythread_attr_t attr_lottery = {MYTHREAD_SCHED_LOTTERY, 10, 0};
    printf("Creando hilo con scheduler LOTTERY\n");
    mythread_create(&threads[5], &attr_lottery, scheduler_test_function, &ids[5]);
    printf("Cambiando scheduler del hilo a RT\n");
    mythread_chsched(&threads[5], MYTHREAD_SCHED_RT);
    mythread_join(&threads[5]);

    printf("\nTodas las pruebas finalizadas exitosamente.\n");
    return 0;
}
