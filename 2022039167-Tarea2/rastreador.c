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

#define MAX_SYSCALLS 500  // Cantidad máxima de syscalls a rastrear

// Tabla de nombres de system calls
const char *syscall_names[MAX_SYSCALLS] = {0};

void init_syscall_names() {
    syscall_names[SYS_read] = "read";
    syscall_names[SYS_write] = "write";
    syscall_names[SYS_open] = "open";
    syscall_names[SYS_close] = "close";
    syscall_names[SYS_stat] = "stat";
    syscall_names[SYS_fstat] = "fstat";
    syscall_names[SYS_lstat] = "lstat";
    syscall_names[SYS_poll] = "poll";
    syscall_names[SYS_lseek] = "lseek";
    syscall_names[SYS_mmap] = "mmap";
    syscall_names[SYS_mprotect] = "mprotect";
    syscall_names[SYS_munmap] = "munmap";
    syscall_names[SYS_brk] = "brk";
    syscall_names[SYS_rt_sigaction] = "rt_sigaction";
    syscall_names[SYS_rt_sigprocmask] = "rt_sigprocmask";
    syscall_names[SYS_rt_sigreturn] = "rt_sigreturn";
    syscall_names[SYS_ioctl] = "ioctl";
    syscall_names[SYS_pread64] = "pread64";
    syscall_names[SYS_pwrite64] = "pwrite64";
    syscall_names[SYS_readv] = "readv";
    syscall_names[SYS_writev] = "writev";
    syscall_names[SYS_access] = "access";
    syscall_names[SYS_pipe] = "pipe";
    syscall_names[SYS_select] = "select";
    syscall_names[SYS_sched_yield] = "sched_yield";
    syscall_names[SYS_mremap] = "mremap";
    syscall_names[SYS_msync] = "msync";
    syscall_names[SYS_mincore] = "mincore";
    syscall_names[SYS_madvise] = "madvise";
    syscall_names[SYS_shmget] = "shmget";
    syscall_names[SYS_shmat] = "shmat";
    syscall_names[SYS_shmctl] = "shmctl";
    syscall_names[SYS_dup] = "dup";
    syscall_names[SYS_dup2] = "dup2";
    syscall_names[SYS_pause] = "pause";
    syscall_names[SYS_nanosleep] = "nanosleep";
    syscall_names[SYS_getitimer] = "getitimer";
    syscall_names[SYS_alarm] = "alarm";
    syscall_names[SYS_setitimer] = "setitimer";
    syscall_names[SYS_getpid] = "getpid";
    syscall_names[SYS_sendfile] = "sendfile";
    syscall_names[SYS_socket] = "socket";
    syscall_names[SYS_connect] = "connect";
    syscall_names[SYS_accept] = "accept";
    syscall_names[SYS_sendto] = "sendto";
    syscall_names[SYS_recvfrom] = "recvfrom";
    syscall_names[SYS_sendmsg] = "sendmsg";
    syscall_names[SYS_recvmsg] = "recvmsg";
    syscall_names[SYS_shutdown] = "shutdown";
    syscall_names[SYS_bind] = "bind";
    syscall_names[SYS_listen] = "listen";
    syscall_names[SYS_getsockname] = "getsockname";
    syscall_names[SYS_getpeername] = "getpeername";
    syscall_names[SYS_socketpair] = "socketpair";
    syscall_names[SYS_setsockopt] = "setsockopt";
    syscall_names[SYS_getsockopt] = "getsockopt";
    syscall_names[SYS_clone] = "clone";
    syscall_names[SYS_fork] = "fork";
    syscall_names[SYS_vfork] = "vfork";
    syscall_names[SYS_execve] = "execve";
    syscall_names[SYS_exit] = "exit";
    syscall_names[SYS_wait4] = "wait4";
    syscall_names[SYS_kill] = "kill";
    syscall_names[SYS_uname] = "uname";
    syscall_names[SYS_semget] = "semget";
    syscall_names[SYS_semop] = "semop";
    syscall_names[SYS_semctl] = "semctl";
    syscall_names[SYS_shmdt] = "shmdt";
    syscall_names[SYS_msgget] = "msgget";
    syscall_names[SYS_msgsnd] = "msgsnd";
    syscall_names[SYS_msgrcv] = "msgrcv";
    syscall_names[SYS_msgctl] = "msgctl";
    syscall_names[SYS_fcntl] = "fcntl";
    syscall_names[SYS_flock] = "flock";
    syscall_names[SYS_fsync] = "fsync";
    syscall_names[SYS_fdatasync] = "fdatasync";
    syscall_names[SYS_truncate] = "truncate";
    syscall_names[SYS_ftruncate] = "ftruncate";
    syscall_names[SYS_getdents] = "getdents";
    syscall_names[SYS_getcwd] = "getcwd";
    syscall_names[SYS_chdir] = "chdir";
    syscall_names[SYS_fchdir] = "fchdir";
    syscall_names[SYS_rename] = "rename";
    syscall_names[SYS_mkdir] = "mkdir";
    syscall_names[SYS_rmdir] = "rmdir";
    syscall_names[SYS_creat] = "creat";
    syscall_names[SYS_link] = "link";
    syscall_names[SYS_unlink] = "unlink";
    syscall_names[SYS_symlink] = "symlink";
    syscall_names[SYS_readlink] = "readlink";
    syscall_names[SYS_chmod] = "chmod";
    syscall_names[SYS_fchmod] = "fchmod";
    syscall_names[SYS_chown] = "chown";
    syscall_names[SYS_fchown] = "fchown";
    syscall_names[SYS_lchown] = "lchown";
    syscall_names[SYS_umask] = "umask";
    syscall_names[SYS_gettimeofday] = "gettimeofday";
    syscall_names[SYS_getrlimit] = "getrlimit";
    syscall_names[SYS_getrusage] = "getrusage";
    syscall_names[SYS_sysinfo] = "sysinfo";
    syscall_names[SYS_times] = "times";
    syscall_names[SYS_ptrace] = "ptrace";
    syscall_names[SYS_getuid] = "getuid";
    syscall_names[SYS_syslog] = "syslog";
    syscall_names[SYS_getgid] = "getgid";
    syscall_names[SYS_setuid] = "setuid";
    syscall_names[SYS_setgid] = "setgid";
    syscall_names[SYS_geteuid] = "geteuid";
    syscall_names[SYS_getegid] = "getegid";
    syscall_names[SYS_setpgid] = "setpgid";
    syscall_names[SYS_getppid] = "getppid";
    syscall_names[SYS_getpgrp] = "getpgrp";
    syscall_names[SYS_setsid] = "setsid";
    syscall_names[SYS_setreuid] = "setreuid";
    syscall_names[SYS_setregid] = "setregid";
    syscall_names[SYS_getgroups] = "getgroups";
    syscall_names[SYS_setgroups] = "setgroups";
    syscall_names[SYS_setresuid] = "setresuid";
    syscall_names[SYS_getresuid] = "getresuid";
    syscall_names[SYS_setresgid] = "setresgid";
    syscall_names[SYS_getresgid] = "getresgid";
    syscall_names[SYS_getpgid] = "getpgid";
    syscall_names[SYS_setfsuid] = "setfsuid";
    syscall_names[SYS_setfsgid] = "setfsgid";
    syscall_names[SYS_getsid] = "getsid";
    syscall_names[SYS_capget] = "capget";
    syscall_names[SYS_capset] = "capset";
    syscall_names[SYS_rt_sigpending] = "rt_sigpending";
    syscall_names[SYS_rt_sigtimedwait] = "rt_sigtimedwait";
    syscall_names[SYS_rt_sigqueueinfo] = "rt_sigqueueinfo";
    syscall_names[SYS_rt_sigsuspend] = "rt_sigsuspend";
    syscall_names[SYS_sigaltstack] = "sigaltstack";
    syscall_names[SYS_utime] = "utime";
    syscall_names[SYS_mknod] = "mknod";
    syscall_names[SYS_personality] = "personality";
    syscall_names[SYS_ustat] = "ustat";
    syscall_names[SYS_statfs] = "statfs";
    syscall_names[SYS_fstatfs] = "fstatfs";
    syscall_names[SYS_sysfs] = "sysfs";
    syscall_names[SYS_getpriority] = "getpriority";
    syscall_names[SYS_setpriority] = "setpriority";
    syscall_names[SYS_sched_setparam] = "sched_setparam";
    syscall_names[SYS_sched_getparam] = "sched_getparam";
    syscall_names[SYS_sched_setscheduler] = "sched_setscheduler";
    syscall_names[SYS_sched_getscheduler] = "sched_getscheduler";
    syscall_names[SYS_sched_get_priority_max] = "sched_get_priority_max";
    syscall_names[SYS_sched_get_priority_min] = "sched_get_priority_min";
    syscall_names[SYS_sched_rr_get_interval] = "sched_rr_get_interval";
    syscall_names[SYS_mlock] = "mlock";
    syscall_names[SYS_munlock] = "munlock";
    syscall_names[SYS_mlockall] = "mlockall";
    syscall_names[SYS_munlockall] = "munlockall";
    syscall_names[SYS_vhangup] = "vhangup";
    syscall_names[SYS_modify_ldt] = "modify_ldt";
    syscall_names[SYS_pivot_root] = "pivot_root";
    syscall_names[SYS_prctl] = "prctl";
    syscall_names[SYS_arch_prctl] = "arch_prctl";
    syscall_names[SYS_adjtimex] = "adjtimex";
    syscall_names[SYS_setrlimit] = "setrlimit";
    syscall_names[SYS_chroot] = "chroot";
    syscall_names[SYS_sync] = "sync";
    syscall_names[SYS_acct] = "acct";
    syscall_names[SYS_settimeofday] = "settimeofday";
    syscall_names[SYS_mount] = "mount";
    syscall_names[SYS_swapon] = "swapon";
    syscall_names[SYS_swapoff] = "swapoff";
    syscall_names[SYS_reboot] = "reboot";
    syscall_names[SYS_sethostname] = "sethostname";
    syscall_names[SYS_setdomainname] = "setdomainname";
    syscall_names[SYS_iopl] = "iopl";
    syscall_names[SYS_ioperm] = "ioperm";
    syscall_names[SYS_init_module] = "init_module";
    syscall_names[SYS_delete_module] = "delete_module";
    syscall_names[SYS_quotactl] = "quotactl";
    syscall_names[SYS_gettid] = "gettid";
    syscall_names[SYS_readahead] = "readahead";
    syscall_names[SYS_setxattr] = "setxattr";
    syscall_names[SYS_lsetxattr] = "lsetxattr";
    syscall_names[SYS_fsetxattr] = "fsetxattr";
    syscall_names[SYS_getxattr] = "getxattr";
    syscall_names[SYS_lgetxattr] = "lgetxattr";
    syscall_names[SYS_fgetxattr] = "fgetxattr";
    syscall_names[SYS_listxattr] = "listxattr";
    syscall_names[SYS_llistxattr] = "llistxattr";
    syscall_names[SYS_flistxattr] = "flistxattr";
    syscall_names[SYS_removexattr] = "removexattr";
    syscall_names[SYS_lremovexattr] = "lremovexattr";
    syscall_names[SYS_fremovexattr] = "fremovexattr";
    syscall_names[SYS_tkill] = "tkill";
    syscall_names[SYS_time] = "time";
    syscall_names[SYS_futex] = "futex";
    syscall_names[SYS_sched_setaffinity] = "sched_setaffinity";
    syscall_names[SYS_sched_getaffinity] = "sched_getaffinity";
    syscall_names[SYS_io_setup] = "io_setup";
    syscall_names[SYS_io_destroy] = "io_destroy";
    syscall_names[SYS_io_getevents] = "io_getevents";
    syscall_names[SYS_io_submit] = "io_submit";
    syscall_names[SYS_io_cancel] = "io_cancel";
    syscall_names[SYS_epoll_create] = "epoll_create";
    syscall_names[SYS_remap_file_pages] = "remap_file_pages";
    syscall_names[SYS_getdents64] = "getdents64";
    syscall_names[SYS_set_tid_address] = "set_tid_address";
    syscall_names[SYS_restart_syscall] = "restart_syscall";
    syscall_names[SYS_semtimedop] = "semtimedop";
    syscall_names[SYS_fadvise64] = "fadvise64";
    syscall_names[SYS_timer_create] = "timer_create";
    syscall_names[SYS_timer_settime] = "timer_settime";
    syscall_names[SYS_timer_gettime] = "timer_gettime";
    syscall_names[SYS_timer_getoverrun] = "timer_getoverrun";
    syscall_names[SYS_timer_delete] = "timer_delete";
    syscall_names[SYS_clock_settime] = "clock_settime";
    syscall_names[SYS_clock_gettime] = "clock_gettime";
    syscall_names[SYS_clock_getres] = "clock_getres";
    syscall_names[SYS_clock_nanosleep] = "clock_nanosleep";
    syscall_names[SYS_exit_group] = "exit_group";
    syscall_names[SYS_epoll_wait] = "epoll_wait";
    syscall_names[SYS_epoll_ctl] = "epoll_ctl";
    syscall_names[SYS_tgkill] = "tgkill";
    syscall_names[SYS_utimes] = "utimes";
    syscall_names[SYS_mbind] = "mbind";
    syscall_names[SYS_set_mempolicy] = "set_mempolicy";
    syscall_names[SYS_get_mempolicy] = "get_mempolicy";
    syscall_names[SYS_mq_open] = "mq_open";
    syscall_names[SYS_mq_unlink] = "mq_unlink";
    syscall_names[SYS_mq_timedsend] = "mq_timedsend";
    syscall_names[SYS_mq_timedreceive] = "mq_timedreceive";
    syscall_names[SYS_mq_notify] = "mq_notify";
    syscall_names[SYS_mq_getsetattr] = "mq_getsetattr";
    syscall_names[SYS_kexec_load] = "kexec_load";
    syscall_names[SYS_waitid] = "waitid";
    syscall_names[SYS_add_key] = "add_key";
    syscall_names[SYS_request_key] = "request_key";
    syscall_names[SYS_keyctl] = "keyctl";
    syscall_names[SYS_ioprio_set] = "ioprio_set";
    syscall_names[SYS_ioprio_get] = "ioprio_get";
    syscall_names[SYS_inotify_init] = "inotify_init";
    syscall_names[SYS_inotify_add_watch] = "inotify_add_watch";
    syscall_names[SYS_inotify_rm_watch] = "inotify_rm_watch";
    syscall_names[SYS_migrate_pages] = "migrate_pages";
    syscall_names[SYS_openat] = "openat";
    syscall_names[SYS_mkdirat] = "mkdirat";
    syscall_names[SYS_mknodat] = "mknodat";
    syscall_names[SYS_fchownat] = "fchownat";
    syscall_names[SYS_futimesat] = "futimesat";
    syscall_names[SYS_newfstatat] = "newfstatat";
    syscall_names[SYS_unlinkat] = "unlinkat";
    syscall_names[SYS_renameat] = "renameat";
    syscall_names[SYS_linkat] = "linkat";
    syscall_names[SYS_symlinkat] = "symlinkat";
    syscall_names[SYS_readlinkat] = "readlinkat";
    syscall_names[SYS_fchmodat] = "fchmodat";
    syscall_names[SYS_faccessat] = "faccessat";
    syscall_names[SYS_pselect6] = "pselect6";
    syscall_names[SYS_ppoll] = "ppoll";
    syscall_names[SYS_unshare] = "unshare";
    syscall_names[SYS_set_robust_list] = "set_robust_list";
    syscall_names[SYS_get_robust_list] = "get_robust_list";
    syscall_names[SYS_splice] = "splice";
    syscall_names[SYS_tee] = "tee";
    syscall_names[SYS_sync_file_range] = "sync_file_range";
    syscall_names[SYS_vmsplice] = "vmsplice";
    syscall_names[SYS_move_pages] = "move_pages";
    syscall_names[SYS_utimensat] = "utimensat";
    syscall_names[SYS_epoll_pwait] = "epoll_pwait";
    syscall_names[SYS_signalfd] = "signalfd";
    syscall_names[SYS_timerfd_create] = "timerfd_create";
    syscall_names[SYS_eventfd] = "eventfd";
    syscall_names[SYS_fallocate] = "fallocate";
    syscall_names[SYS_timerfd_settime] = "timerfd_settime";
    syscall_names[SYS_timerfd_gettime] = "timerfd_gettime";
    syscall_names[SYS_accept4] = "accept4";
    syscall_names[SYS_signalfd4] = "signalfd4";
    syscall_names[SYS_eventfd2] = "eventfd2";
    syscall_names[SYS_epoll_create1] = "epoll_create1";
    syscall_names[SYS_dup3] = "dup3";
    syscall_names[SYS_pipe2] = "pipe2";
    syscall_names[SYS_inotify_init1] = "inotify_init1";
    syscall_names[SYS_preadv] = "preadv";
    syscall_names[SYS_pwritev] = "pwritev";
    syscall_names[SYS_rt_tgsigqueueinfo] = "rt_tgsigqueueinfo";
    syscall_names[SYS_perf_event_open] = "perf_event_open";
    syscall_names[SYS_recvmmsg] = "recvmmsg";
    syscall_names[SYS_fanotify_init] = "fanotify_init";
    syscall_names[SYS_fanotify_mark] = "fanotify_mark";
    syscall_names[SYS_prlimit64] = "prlimit64";
    syscall_names[SYS_name_to_handle_at] = "name_to_handle_at";
    syscall_names[SYS_open_by_handle_at] = "open_by_handle_at";
    syscall_names[SYS_clock_adjtime] = "clock_adjtime";
    syscall_names[SYS_syncfs] = "syncfs";
    syscall_names[SYS_sendmmsg] = "sendmmsg";
    syscall_names[SYS_setns] = "setns";
    syscall_names[SYS_getcpu] = "getcpu";
    syscall_names[SYS_process_vm_readv] = "process_vm_readv";
    syscall_names[SYS_process_vm_writev] = "process_vm_writev";
    syscall_names[SYS_kcmp] = "kcmp";
    syscall_names[SYS_finit_module] = "finit_module";
    syscall_names[SYS_sched_setattr] = "sched_setattr";
    syscall_names[SYS_sched_getattr] = "sched_getattr";
    syscall_names[SYS_renameat2] = "renameat2";
    syscall_names[SYS_seccomp] = "seccomp";
    syscall_names[SYS_getrandom] = "getrandom";
    syscall_names[SYS_memfd_create] = "memfd_create";
    syscall_names[SYS_kexec_file_load] = "kexec_file_load";
    syscall_names[SYS_bpf] = "bpf";
    syscall_names[SYS_execveat] = "execveat";
    syscall_names[SYS_userfaultfd] = "userfaultfd";
    syscall_names[SYS_membarrier] = "membarrier";
    syscall_names[SYS_mlock2] = "mlock2";
    syscall_names[SYS_copy_file_range] = "copy_file_range";
    syscall_names[SYS_preadv2] = "preadv2";
    syscall_names[SYS_pwritev2] = "pwritev2";
    syscall_names[SYS_pkey_mprotect] = "pkey_mprotect";
    syscall_names[SYS_pkey_alloc] = "pkey_alloc";
    syscall_names[SYS_pkey_free] = "pkey_free";
    syscall_names[SYS_statx] = "statx";
    syscall_names[SYS_io_pgetevents] = "io_pgetevents";
    syscall_names[SYS_rseq] = "rseq";
    syscall_names[SYS_pidfd_send_signal] = "pidfd_send_signal";
    syscall_names[SYS_io_uring_setup] = "io_uring_setup";
    syscall_names[SYS_io_uring_enter] = "io_uring_enter";
    syscall_names[SYS_io_uring_register] = "io_uring_register";
    syscall_names[SYS_open_tree] = "open_tree";
    syscall_names[SYS_move_mount] = "move_mount";
    syscall_names[SYS_fsopen] = "fsopen";
    syscall_names[SYS_fsconfig] = "fsconfig";
    syscall_names[SYS_fsmount] = "fsmount";
    syscall_names[SYS_fspick] = "fspick";
    syscall_names[SYS_pidfd_open] = "pidfd_open";
    syscall_names[SYS_clone3] = "clone3";
    syscall_names[SYS_close_range] = "close_range";
    syscall_names[SYS_openat2] = "openat2";
    syscall_names[SYS_pidfd_getfd] = "pidfd_getfd";
    syscall_names[SYS_faccessat2] = "faccessat2";
    syscall_names[SYS_process_madvise] = "process_madvise";
    syscall_names[SYS_epoll_pwait2] = "epoll_pwait2";
    syscall_names[SYS_mount_setattr] = "mount_setattr";
    syscall_names[SYS_quotactl_fd] = "quotactl_fd";
    syscall_names[SYS_landlock_create_ruleset] = "landlock_create_ruleset";
    syscall_names[SYS_landlock_add_rule] = "landlock_add_rule";
    syscall_names[SYS_landlock_restrict_self] = "landlock_restrict_self";
    syscall_names[SYS_memfd_secret] = "memfd_secret";
    syscall_names[SYS_process_mrelease] = "process_mrelease";
    syscall_names[SYS_futex_waitv] = "futex_waitv";
    syscall_names[SYS_set_mempolicy_home_node] = "set_mempolicy_home_node";
    syscall_names[SYS_cachestat] = "cachestat";
    syscall_names[SYS_fchmodat2] = "fchmodat2";
    syscall_names[SYS_map_shadow_stack] = "map_shadow_stack";
    syscall_names[SYS_futex_wake] = "futex_wake";
    syscall_names[SYS_futex_wait] = "futex_wait";
    syscall_names[SYS_futex_requeue] = "futex_requeue";
}


int syscall_count[MAX_SYSCALLS] = {0};  // Tabla acumulativa de syscalls
int verbose = 0;  // -v activado
int pause_mode = 0;  // -V activado
int syscall_counter=0;

void trace_syscalls(pid_t child) {
    
    int status;
    struct user_regs_struct regs;
<<<<<<< HEAD:2022039167-Tarea2/tracer.c
    int in_syscall = 0;//contador de system calls
=======
    int in_syscall =0; //distingue entre la entrada y la salida del proceso de ptrace
>>>>>>> refs/remotes/origin/main:2022039167-Tarea2/rastreador.c

    waitpid(child, &status, 0);
    ptrace(PTRACE_SYSCALL, child, NULL, NULL); 

    while (1) {
        waitpid(child, &status, 0);
        if (WIFEXITED(status)) break;

        // Capturar número de syscall y traduce
        ptrace(PTRACE_GETREGS, child, NULL, &regs);
        long syscall_num = regs.orig_rax;

        if (syscall_num >= 0 && syscall_num < MAX_SYSCALLS && !in_syscall) {
            syscall_count[syscall_num]++;
            syscall_counter++;

            in_syscall=1;//distingui proceso de entrada y de salida
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

        }else{
            in_syscall=0;
        }

        ptrace(PTRACE_SYSCALL, child, NULL, NULL); 
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
    printf("----------------------------------\n");
    printf("Total\t\t %d\n", syscall_counter);
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
        // el hijo permite ser rastreado 
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);

        //ejecutar el programa
        execvp(argv[prog_index], &argv[prog_index]);
        perror("execvp");//si encuentra un error
        return 1;
    } else {
        // Monitorea al hijo
        trace_syscalls(child);
        print_syscall_summary();
    }

    return 0;
}
