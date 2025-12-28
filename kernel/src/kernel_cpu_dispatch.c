#include "../include/kernel_cpu_dispatch.h"
#include "../include/planificadores.h"
#include "../include/kernel_manejo_ios.h"
#include "../include/kernel_manejo_cpus.h"



void atender_kernel_cpu_dispatch(int* fd_cpu_dispatch) {
    int fd_cpu_dispatch_local = *fd_cpu_dispatch;
    free(fd_cpu_dispatch);
    bool control_key = 1;
    while (control_key) {
        int cod_op = recibir_operacion(fd_cpu_dispatch_local);
        log_trace(kernel_logger, "Código de operación recibido: %d", cod_op);
        switch (cod_op) {
        case SYS_COMIENZO_IO:
            log_trace(kernel_logger, "Recibiendo syscall de COMIENZO_IO de CPU DISPATCH.");
            t_buffer* temp = recibir_todo_el_buffer(fd_cpu_dispatch_local);
            uint32_t pid = extraer_uint32_del_buffer(temp);
            uint32_t pc = extraer_uint32_del_buffer(temp);
            char* dispositivo_IO = extraer_string_del_buffer(temp); 
            int cantidad_milisegundos = extraer_int_del_buffer(temp);
            log_info(kernel_logger, "## (<%d>) - Solicitó syscall: <IO>", pid);
            if(!existe_io(dispositivo_IO)){
                EXIT_PID(pid);
                log_debug(kernel_logger, "El dispositivo IO %s no existe en el sistema. Proceso %d finalizado.", dispositivo_IO, pid);
            }
            else{
                liberar_cpu(pid);
                exec_a_blocked(pid, pc);
                log_info(kernel_logger, "## (<%d>) - Bloqueado por IO: %s", pid, dispositivo_IO);
                agregar_a_cola_por_IO(pid, dispositivo_IO, cantidad_milisegundos);
            }
           break;
        case SYS_INIT_PROC:
            log_trace(kernel_logger, "Recibiendo syscall de INIT_PROC de CPU DISPATCH.");
            t_buffer* buffer = recibir_todo_el_buffer(fd_cpu_dispatch_local);
            uint32_t pid_proc = extraer_uint32_del_buffer(buffer);
            char* archivo_pseudocodigo = extraer_string_del_buffer(buffer);
            int tamanio_proceso = extraer_int_del_buffer(buffer);
            log_info(kernel_logger, "## (<%d>) - Solicitó syscall: <INIT_PROC>", pid_proc);
            INIT_PROC(archivo_pseudocodigo, tamanio_proceso);
            break;
        case SYS_DUMP_MEMORY:
            log_trace(kernel_logger, "Recibiendo syscall de DUMP_MEMORY de CPU DISPATCH.");
            t_buffer* temp_dump = crear_buffer();
            temp_dump = recibir_todo_el_buffer(fd_cpu_dispatch_local); //Contiene el PID del proceso que solicita el DUMP
            uint32_t pid_dump = extraer_uint32_del_buffer(temp_dump);
            uint32_t pc_dump = extraer_uint32_del_buffer(temp_dump);
            log_info(kernel_logger, "## (<%d>) - Solicitó syscall: <DUMP_MEMORY>", pid_dump);
            log_trace(kernel_logger, "Proceso %d solicita DUMP_MEMORY", pid_dump);
            log_trace(kernel_logger, "Actualizando PC del proceso %d a %d", pid_dump, pc_dump);


            t_buffer* buffer_dump = crear_buffer();
            cargar_uint32_al_buffer(buffer_dump, pid_dump);
            t_paquete* paquete_dump = crear_paquete(MEMORY_DUMP, buffer_dump);
            enviar_paquete(paquete_dump, fd_memoria);
            liberar_cpu(pid_dump);
            exec_a_blocked(pid_dump, pc_dump);
           
            break;
        case SYS_EXIT:
            //Finalizacion de proceso, no tiene parametros
            log_trace(kernel_logger, "Recibiendo syscall de EXIT de CPU DISPATCH.");
            t_buffer* temp_exit = recibir_todo_el_buffer(fd_cpu_dispatch_local);
            uint32_t pid_exit = extraer_uint32_del_buffer(temp_exit);
            log_info(kernel_logger, "## (<%d>) - Solicitó syscall: <EXIT>", pid_exit);
            log_trace(kernel_logger, "Proceso %d solicita EXIT", pid_exit);

            EXIT_PID(pid_exit);
        break;

        case -1:
            log_error(kernel_logger, "El CPU DISPATCH se desconecto.");
            control_key = 0;
        default:
            log_warning(kernel_logger, "Operacion desconocida de CPU DISPATCH.");
            break;
        }
    }
}

