#include "../include/cpu_kernel_interrupt.h"



void atender_cpu_kernel_interrupt(){
    bool control_key = 1;
    while (control_key) {
        int cod_op = recibir_operacion(fd_kernel_interrupt);
        switch (cod_op) {
        case INTERRUPT:
            log_info(cpu_logger, "## Llega interrupcion al puerto Interrupt");
            t_buffer* temp = recibir_todo_el_buffer(fd_kernel_interrupt);
            uint32_t pid_interrupt = extraer_uint32_del_buffer(temp);
            
            if (pid_interrupt == pid) {
                interrupcion_pendiente = true;
            }

            break;

        case -1:
            log_error(cpu_logger, "El KERNEL INTERRUPT se desconecto.");
            control_key = 0;
        default:
            log_trace(cpu_logger, "NO HAY INTERRUPCIONES");
            break;
        }
    }
}