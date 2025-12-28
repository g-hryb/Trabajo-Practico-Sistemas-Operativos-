#include "../include/kernel_memoria.h"
#include "../include/planificadores.h"

sem_t* binario_respuesta_crear_proceso;
sem_t* binario_respuesta_eliminar_proceso;
sem_t* binario_respuesta_desuspender_proceso;
bool respuesta_crear_proceso;
bool respuesta_eliminar_proceso;
bool respuesta_desuspender_proceso;

void inicializar_binario_respuestas() {
    binario_respuesta_crear_proceso = malloc(sizeof(sem_t));
    sem_init(binario_respuesta_crear_proceso, 0, 0);
    binario_respuesta_eliminar_proceso= malloc(sizeof(sem_t));
    sem_init(binario_respuesta_eliminar_proceso, 0, 0);
    binario_respuesta_desuspender_proceso= malloc(sizeof(sem_t));
    sem_init(binario_respuesta_desuspender_proceso, 0, 0);
}


void atender_kernel_memoria(){
    bool control_key = 1;
    while (control_key) {
        int cod_op = recibir_operacion(fd_memoria);
        log_trace(kernel_logger, "Recibido código de operación: %d", cod_op);
        switch (cod_op) {
        case RESPUESTA_DESUSPENDER_PROCESO_KM:
            t_buffer* buffer_respuesta = recibir_todo_el_buffer(fd_memoria);
            int respuesta = extraer_int_del_buffer(buffer_respuesta);
            if (respuesta == 1) {
                respuesta_desuspender_proceso = true;
            } else {
                respuesta_desuspender_proceso = false;
            }
            sem_post(binario_respuesta_desuspender_proceso); // Señaliza que se recibió la respuesta
            break;
        case RESPUESTA_FINALIZAR_PROCESO_KM: {
            log_info(kernel_logger, "Recibiendo respuesta de finalización de proceso de memoria.");
            t_buffer* buffer_respuesta = recibir_todo_el_buffer(fd_memoria);
            int respuesta = extraer_int_del_buffer(buffer_respuesta);
            if (respuesta == 1) {
                respuesta_eliminar_proceso = true;
            } else {
                respuesta_eliminar_proceso = false;
            }
            sem_post(binario_respuesta_eliminar_proceso); // Señaliza que se recibió la respuesta
            break;
        }
        case RESPUESTA_CREAR_PROCESO_KM: {
            t_buffer* buffer_respuesta = recibir_todo_el_buffer(fd_memoria);
            int respuesta = extraer_int_del_buffer(buffer_respuesta);
            if (respuesta == 1) {
                respuesta_crear_proceso = true;
            } else {
                respuesta_crear_proceso = false;
            }
            sem_post(binario_respuesta_crear_proceso); // Señaliza que se recibió la respuesta
            break;
        }
        case RESPUESTA_MEMORY_DUMP: {
            t_buffer* buffer_respuesta = recibir_todo_el_buffer(fd_memoria);
            uint32_t pid = extraer_uint32_del_buffer(buffer_respuesta);
            int respuesta = extraer_int_del_buffer(buffer_respuesta);

            if (respuesta == 1) {
               blocked_a_ready(pid);
            } else {
                EXIT_PID(pid);
            }
            break;
            //Se debe bloquear al proceso hasta que memoria confirme
            //NO CON ESPERA ACTIVA, se manda a blocked y se maneja la situacion en kernel_memoria.c
            //Si hay error -> se manda a exit de una
            //Si esta todo joya -> vuelve a ready
        }
        case -1:
            log_error(kernel_logger, "La MEMORIA se desconecto.");
            control_key = 0;
        default:
            log_warning(kernel_logger, "Operacion desconocida de MEMORIA.");
            break;
        }
    }
}