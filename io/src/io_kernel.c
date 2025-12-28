#include "../include/io_kernel.h"

void atender_io_kernel(){
    bool control_key = 1;
    while (control_key) {
        int cod_op = recibir_operacion(fd_kernel);
        switch (cod_op) {
        case MENSAJE:
            //
            break;
        case INICIO_IO:
            t_buffer* buffer_recv = recibir_todo_el_buffer(fd_kernel);
            uint32_t pid = extraer_uint32_del_buffer(buffer_recv);
            int tiempo_milisegundos = extraer_int_del_buffer(buffer_recv);
            log_info(io_logger, "## PID: <%d> - Inicio de IO - Tiempo: <%d>", pid, tiempo_milisegundos);
            
            usleep(tiempo_milisegundos * 1000);
            
            t_buffer* buffer_envio = crear_buffer();
            cargar_uint32_al_buffer(buffer_envio, pid);
            t_paquete* paquete_envio = crear_paquete(FIN_IO, buffer_envio);
            enviar_paquete(paquete_envio, fd_kernel); // Avisar al Kernel que terminó     
            log_info(io_logger, "## PID: <%d> - Fin de IO", pid);
            break;
        case -1:
            log_error(io_logger, "EL KERNEL se desconecto.");
            control_key = 0;
        default:
            log_warning(io_logger, "Operacion desconocida de KERNEL.");
            break;
        }
    }
}