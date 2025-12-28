#include "../include/planificadores.h"
#include "../include/kernel_manejo_cpus.h"
#include "../include/kernel_pcb.h"

void atender_kernel_cpu_interrupt(int* fd_cpu_interrupt) {
    int fd_cpu_interrupt_local = *fd_cpu_interrupt;
    free(fd_cpu_interrupt);
    bool control_key = 1;
    while (control_key) {
        int cod_op = recibir_operacion(fd_cpu_interrupt_local);
        switch (cod_op) {
        case SYS_INTERRUPT:
            t_buffer* buffer = recibir_todo_el_buffer(fd_cpu_interrupt_local);
            uint32_t pid = extraer_uint32_del_buffer(buffer);
            uint32_t pc = extraer_uint32_del_buffer(buffer);
            t_pcb* proceso = buscar_proceso_por_pid(pid);
            log_debug(kernel_logger, "Recibi paquete de sys_interrupt, encontre proceso con pid: %d", proceso->pid);
            temporal_stop(proceso->cronometro); // Detiene el cronómetro del proceso
            proceso -> programCounter = pc; // Actualiza el PC del proceso
            proceso -> estimacion_proxima_rafaga -= (double) temporal_gettime(proceso -> cronometro); //verificar si funciona bien
            temporal_destroy(proceso->cronometro); // Libera el cronómetro del proceso
            log_debug(kernel_logger, "Liberando la cpu correspondiente");
            //ES LA FUNCION LIBERAR CPU SIN LOS MUTEX 
             for(int i = 0; i < list_size(lista_CPU); i++) {
                nodoCPU* actual = list_get(lista_CPU, i);
                if (actual->proceso_running != NULL && actual->proceso_running->pid == pid) {
                actual->estado = DISPONIBLE; // Marca la CPU como disponible
                actual->proceso_running = NULL; // Limpia el proceso en ejecución
                log_trace(kernel_logger, "CPU ID: %d liberada del proceso PID: %d", actual->id_cpu, pid);
                break;
                }
            }
            //ES LA FUNCION LIBERAR CPU SIN LOS MUTEX 
            log_info(kernel_logger, "## (%d) - Desalojado por algoritmo SJF/SRT", pid);
            pthread_mutex_lock(mutex_EXEC);
            for(int i = 0; i < list_size(cola_EXEC->lista_procesos); i++) {
                t_pcb* actual = list_get(cola_EXEC->lista_procesos, i);
                if(actual->pid == pid){
                    list_remove(cola_EXEC->lista_procesos, i);}
            }
            pthread_mutex_unlock(mutex_EXEC);
            agregar_a_READY(proceso); // Mueve el proceso a la cola READY
            sem_post(binario_desalojado);
            break;
        case PAQUETE:
            //
            break;
        case -1:
            log_error(kernel_logger, "El cpu interrupt se desconecto.");
            control_key = 0;
        default:
            log_warning(kernel_logger, "Operacion desconocida de CPU INTERRUPT.");
            break;
        }
    }
}