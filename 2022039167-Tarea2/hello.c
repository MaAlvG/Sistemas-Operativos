#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <errno.h>

#define MAX_SYSCALLS 512  // Cantidad máxima de syscalls a rastrear

int syscall_count[MAX_SYSCALLS] = {0};  // Tabla acumulativa de syscalls
int verbose = 0;  // -v activado
int pause_mode = 0;  // -V activado

void trace_syscalls(pid_t child) {
    int status;
    struct user_regs_struct regs;

    waitpid(child, &status, 0);
    ptrace(PTRACE_SYSCALL, child, NULL, NULL); // Permitir la primera syscall

    while (1) {
        waitpid(child, &status, 0);
        if (WIFEXITED(status)) break;

        // Capturar número de syscall
        ptrace(PTRACE_GETREGS, child, NULL, &regs);
        long syscall_num = regs.orig_rax;
        if (syscall_num >= 0 && syscall_num < MAX_SYSCALLS) {
            syscall_count[syscall_num]++;
            if (verbose) {
                printf("Syscall: %ld\n", syscall_num);
                if (pause_mode) {
                    printf("Presiona ENTER para continuar...");
                    getchar();
                }
            }
        }

        ptrace(PTRACE_SYSCALL, child, NULL, NULL); // Continuar ejecución
    }
}

// Imprimir la tabla final de syscalls
void print_syscall_summary() {
    printf("\nResumen de System Calls:\n");
    printf("-------------------------------\n");
    printf("Syscall\t|\tVeces usada\n");
    printf("-------------------------------\n");
    for (int i = 0; i < MAX_SYSCALLS; i++) {
        if (syscall_count[i] > 0) {
            printf("%d\t|\t%d\n", i, syscall_count[i]);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s [opciones] Prog [argumentos de Prog]\n", argv[0]);
        return 1;
    }

    // Procesar opciones
    int prog_index = 1;
    while (prog_index < argc && argv[prog_index][0] == '-') {
        if (strcmp(argv[prog_index], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[prog_index], "-V") == 0) {
            verbose = 1;
            pause_mode = 1;
        } else {
            fprintf(stderr, "Opción desconocida: %s\n", argv[prog_index]);
            return 1;
        }
        prog_index++;
    }

    if (prog_index >= argc) {
        fprintf(stderr, "Error: No se especificó un programa para rastrear.\n");
        return 1;
    }

    pid_t child = fork();
    if (child == 0) {
        // Hijo: Permitir ser rastreado y ejecutar el programa
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[prog_index], &argv[prog_index]);
        perror("execvp");
        return 1;
    } else {
        // Padre: Monitorea al hijo
        trace_syscalls(child);
        print_syscall_summary();
    }

    return 0;
}
