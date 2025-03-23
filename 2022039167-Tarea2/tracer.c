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

#define MAX_SYSCALLS 1000  // Cantidad máxima de syscalls a rastrear

#define SYS_eventfd __NR_eventfd
#define SYS_eventfd2 __NR_eventfd2
#define SYS_execve __NR_execve
#define SYS_exit __NR_exit
#define SYS_exit_group __NR_exit_group
#define SYS_faccessat __NR_faccessat
// Tabla de nombres de system calls (basada en syscall.h de Linux)
const char *syscall_names[MAX_SYSCALLS] = {0};

void init_syscall_names() {
    syscall_names[SYS_read] = "read";
    syscall_names[SYS_write] = "write";
    syscall_names[SYS_open] = "open";
    syscall_names[SYS_close] = "close";
    syscall_names[SYS_fork] = "fork";
    syscall_names[SYS_execve] = "execve";
    syscall_names[SYS_exit] = "exit";
    syscall_names[SYS_wait4] = "wait4";
    syscall_names[SYS_brk] = "brk";
    syscall_names[SYS_mmap] = "mmap";
    syscall_names[SYS_munmap] = "munmap";
    syscall_names[SYS_ioctl] = "ioctl";
    syscall_names[SYS_fstat] = "fstat";
    syscall_names[SYS_lseek] = "lseek";
    syscall_names[SYS_getpid] = "getpid";
    syscall_names[SYS_getuid] = "getuid";
    syscall_names[SYS_geteuid] = "geteuid";
    syscall_names[SYS_getgid] = "getgid";
    syscall_names[SYS_getegid] = "getegid";
    syscall_names[SYS_time] = "time";
    syscall_names[SYS_uname] = "uname";
    syscall_names[SYS_getppid] = "getppid";
    syscall_names[SYS_stat] = "stat";
    syscall_names[SYS_access] = "access";
    syscall_names[SYS_pipe] = "pipe";
    syscall_names[SYS_dup] = "dup";
    syscall_names[SYS_dup2] = "dup2";
    syscall_names[SYS_setsid] = "setsid";
    syscall_names[SYS_nanosleep] = "nanosleep";
    syscall_names[SYS_sched_yield] = "sched_yield";
    syscall_names[SYS_clone] = "clone";
    syscall_names[SYS_exit_group] = "exit_group";
    // Se pueden agregar más syscalls según sea necesario
}

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
                if (syscall_names[syscall_num]) {
                    printf("Syscall: %s (%ld)\n", syscall_names[syscall_num], syscall_num);
                } else {
                    printf("Syscall: %ld (desconocida)\n", syscall_num);
                }

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
    printf("----------------------------------\n");
    printf("Syscall\t\tVeces usada\n");
    printf("----------------------------------\n");
    for (int i = 0; i < MAX_SYSCALLS; i++) {
        if (syscall_count[i] > 0) {
            if (syscall_names[i]) {
                printf("%-15s | %d\n", syscall_names[i], syscall_count[i]);
            } else {
                printf("%-15d | %d (desconocida)\n", i, syscall_count[i]);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s [opciones] Prog [argumentos de Prog]\n", argv[0]);
        return 1;
    }

    init_syscall_names();  // Inicializar la tabla de nombres de syscalls

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
