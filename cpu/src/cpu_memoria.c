#include "../include/cpu_memoria.h"

char *instruccion;

void atender_cpu_memoria()
{
    bool control_key = 1;
    while (control_key)
    {
        int cod_op = recibir_operacion(fd_memoria);
        switch (cod_op)
        {
        case INSTRUCTION_REQUESTED:
            t_buffer *buffer_intruccion = recibir_todo_el_buffer(fd_memoria);
            instruccion = extraer_string_del_buffer(buffer_intruccion);
            entradasPorTabla = extraer_int_del_buffer(buffer_intruccion);
            cantidadDeNiveles = extraer_int_del_buffer(buffer_intruccion);
            tamanioPagina = extraer_int_del_buffer(buffer_intruccion);
            /*  if(datos_memoria==NULL){

                    datos_memoria = malloc(tamanioPagina);
                     memset(datos_memoria, -1, tamanioPagina);


             } */
            log_trace(cpu_logger, "Instruccion recibida de MEMORIA: %s", instruccion);
            sem_post(sem_instruccion);
            log_trace(cpu_logger, "Semáforo de instrucción liberado");
            break;
        case RESPUESTA_REQUEST_READ_FULL_PAGE:
            log_trace(cpu_logger, "RECIBI PAGINA ENTERA");
            t_buffer *buffer_respuesta = recibir_todo_el_buffer(fd_memoria);

            
            free(datos_memoria);
            datos_memoria = calloc(1, tamanioPagina);
            tamanio_contenido = 0;
            void* contenido=limpiar_header_si_existe(buffer_respuesta->stream);

            
            memcpy(datos_memoria, contenido, tamanioPagina);

            tamanio_contenido = buffer_respuesta->size;

            destruir_buffer(buffer_respuesta);
            sem_post(sem_pagina);

            break;
        case RESPUESTA_REQUEST_WRITE_FULL_PAGE:

            log_trace(cpu_logger, "RECIBI PAGINA ENTERA");
            t_buffer *buffer_write_full = recibir_todo_el_buffer(fd_memoria);
            char *rta = extraer_string_del_buffer(buffer_write_full);
            // sem_post(sem_pagina);

            // log_trace(cpu_logger, "ANTES DE ESPERAR sem_pagina (WRITE FULL PAGE)");
            // sem_wait(sem_pagina);
            // log_trace(cpu_logger, "DESPUES DE ESPERAR sem_pagina (WRITE FULL PAGE)");

            break;
        case RESPUESTA_REQUEST_FRAME:
            t_buffer *buffer_frame = recibir_todo_el_buffer(fd_memoria);
            marco_proceso = extraer_int_del_buffer(buffer_frame);

            sem_post(sem_marco);
            log_trace(cpu_logger, "Marco recibido de MEMORIA");

            break;
        case ESPACIO_VACIO:
            log_trace(cpu_logger, "ESPACIO VACIO");
            datos_memoria = NULL;

            sem_post(sem_pagina);
            break;

        case RESPUESTA_REQUEST_WRITE:
            t_buffer *buffer_respuesta_write = recibir_todo_el_buffer(fd_memoria);
            int respuesta;
            respuesta = extraer_int_del_buffer(buffer_respuesta_write);
            if (respuesta == 1)
            {
                log_trace(cpu_logger, "Escritura exitosa");
            }
            if (respuesta == -1)
            {
                log_trace(cpu_logger, "Escritura fallida");
            }

            break;
        case RESPUESTA_REQUEST_READ:
            log_info(cpu_logger, "RECIBI PAGINA");
            t_buffer *buffer_respuesta2 = recibir_todo_el_buffer(fd_memoria);

            if (datos_memoria != NULL)
            {
                free(datos_memoria);
            }
            if(tamanio_contenido != 0) {
                tamanio_contenido = 0;
            }

            tamanio_contenido= buffer_respuesta2->size;
            datos_memoria = malloc(tamanio_contenido + 1); // +1 para \0 si querés usarlo como string
            memcpy(datos_memoria, buffer_respuesta2->stream, tamanio_contenido); // SOLO el tamaño real
            ((char*)datos_memoria)[tamanio_contenido] = '\0';
            destruir_buffer(buffer_respuesta2);
            

            sem_post(sem_pagina);
            break;
            /* log_info(cpu_logger, "RECIBI PAGINA");
            t_buffer* buffer_respuesta2 = recibir_todo_el_buffer(fd_memoria);
            char* string_a_leer = extraer_string_del_buffer(buffer_respuesta2);


            if(datos_memoria != NULL) {
                    free(datos_memoria);
                    datos_memoria = NULL;
                    datos_memoria = malloc(tamanioPagina);
                    datos_memoria = calloc(1, tamanioPagina);
                }
            else{
                datos_memoria = malloc(tamanioPagina);
                datos_memoria = calloc(1, tamanioPagina);
            }

            size_t tamanio_leido = strlen(string_a_leer);
            memcpy(datos_memoria, string_a_leer, tamanio_leido);
            free(string_a_leer);
            free(tamanio_leido);
            sem_post(sem_pagina); */

            break;
        case -1:
            log_error(cpu_logger, "La MEMORIA se desconecto.");
            control_key = 0;
        default:
            log_warning(cpu_logger, "Operacion desconocida de MEMORIA.");
            break;
        }
    }
}


void* limpiar_header_si_existe(void* contenido) {
    unsigned char* bytes = (unsigned char*)contenido;
    // Si empieza con 0x20 0x00 0x00 0x00, quita el header y rellena
    if (bytes[0] == 0x20 && bytes[1] == 0x00 && bytes[2] == 0x00 && bytes[3] == 0x00) {
        memmove(bytes, bytes + 4, tamanioPagina - 4);
        memset(bytes + tamanioPagina - 4, 0, 4);
    }
    return contenido;
}