#include "../include/cpu_kernel_dispatch.h"


    pthread_mutex_t * mutex_pid;
    pthread_mutex_t * mutex_pc;
    sem_t* sem_pid_pc;
    sem_t* sem_contexto;
    bool esperar_contexto = true;

void inicializar_semaforos_pid_pc(){

    mutex_pid = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_pid, NULL);
    mutex_pc = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_pc, NULL);
    
    //A VER SI FUNCA
    sem_pid_pc = malloc(sizeof(sem_t));
    sem_init(sem_pid_pc, 0, 0);
    sem_contexto = malloc(sizeof(sem_t));
    sem_init(sem_contexto, 0, 0);
}

void atender_cpu_kernel_dispatch(){
    bool control_key = 1;
    while (control_key) {
        int cod_op = recibir_operacion(fd_kernel_dispatch);
        switch (cod_op) {
        case CONTEXTO:
            log_trace(cpu_logger, "Recibiendo contexto de KERNEL DISPATCH.");

            atender_contexto(cpu_logger);

            break;
        case CONTEXTO_INTERRUPCION:
            log_trace(cpu_logger, "Recibiendo contexto de interrupción de KERNEL DISPATCH.");
            t_buffer* temp = recibir_todo_el_buffer(fd_kernel_dispatch);
            pthread_mutex_lock(mutex_pid);
            pid = extraer_uint32_del_buffer(temp);
            pthread_mutex_unlock(mutex_pid);
            pthread_mutex_lock(mutex_pc);
            pc = extraer_uint32_del_buffer(temp);
            pthread_mutex_unlock(mutex_pc);
            //sem_post(sem_pid_pc);
            if(esperar_contexto){
                sem_post(sem_contexto);
            }
            sem_post(sem_interrupt);
            break;
        case -1:
            log_error(cpu_logger, "El KERNEL DISPATCH se desconecto.");
            control_key = 0;
        default:
            log_warning(cpu_logger, "Operacion desconocida de KERNEL DISPATCH.");
            break;
        }
    }
}

void atender_contexto(t_log* cpu_logger ){
    t_buffer* temp = recibir_todo_el_buffer(fd_kernel_dispatch);
    pthread_mutex_lock(mutex_pid);
    pid = extraer_uint32_del_buffer(temp);
    pthread_mutex_unlock(mutex_pid);
    pthread_mutex_lock(mutex_pc);
    pc = extraer_uint32_del_buffer(temp);
    pthread_mutex_unlock(mutex_pc);
    //sem_post(sem_pid_pc); 
    sem_post(sem_contexto);
}

