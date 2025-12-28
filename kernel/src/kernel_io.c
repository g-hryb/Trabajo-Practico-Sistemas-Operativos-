#include "../include/kernel_io.h"
#include "../include/kernel_manejo_ios.h"
#include "../include/planificadores.h"

void atender_kernel_io(int* fd_io) {
    int fd_io_local = *fd_io;
    free(fd_io);
    bool control_key = 1;
    while (control_key) {
        int cod_op = recibir_operacion(fd_io_local);
        switch (cod_op) {
        case FIN_IO:
            t_buffer* buffer = recibir_todo_el_buffer(fd_io_local);
            uint32_t pid = extraer_uint32_del_buffer(buffer);
            blocked_a_ready(pid);
            log_info(kernel_logger, "## (<%d>) finalizó IO y pasa a READY/SUSP_READY", pid);
            liberar_io(fd_io_local);
            break;
        case -1:
            log_error(kernel_logger, "El IO se desconecto.");
            eliminar_nodo_io(fd_io_local); // Elimina el IO de la lista
            control_key = 0;
            break;
        default:
            log_warning(kernel_logger, "Operacion desconocida de IO.");
            break;
        }
    }
}


