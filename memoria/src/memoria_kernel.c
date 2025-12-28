#include "../include/memoria_kernel.h"
#include "../include/inicializar_memoria.h"
#include "../include/gestor_memoria.h" //no me toma el gestor 
#include "../include/memoria_proceso.h"
#include "../include/manejo_memoria.h"
#include "../include/memoria_swap.h"
#include "../include/memoria_finalizar.h"
#include "../include/memory_dump.h"

void atender_memoria_kernel(){
    bool control_key = 1;
    while (control_key) {
        int cod_op = recibir_operacion(fd_kernel);
        log_trace(memoria_logger, "Recibido código de operación: %d", cod_op);
        switch (cod_op) {
        case MENSAJE:
            //
            break;
        case PAQUETE:
            //
            break;        
        case CREAR_PROCESO_KM:{ //[int pid][char* path][int size]
        
            log_trace(memoria_logger, "Recibi el mensaje de crear proceso.");
            t_buffer* buffer = recibir_todo_el_buffer(fd_kernel);
            uint32_t pid = extraer_uint32_del_buffer(buffer);
            char* path = extraer_string_del_buffer(buffer);
            int tamanio = extraer_int_del_buffer(buffer);

            int espacio_restate = espacio_libre_en_memoria();

            if (tamanio == 0)
            {
                // si el tamanio del proceso es cero entonces creamos las metricas del proceso pero no creamos la tabla de paginas
                proceso_t* proceso = crear_estructura_proceso(pid, path, 0);
                pthread_mutex_lock(mutex_lista_procesos);
                list_add(lista_procesos, proceso);
                pthread_mutex_unlock(mutex_lista_procesos);
                enviar_mensaje_ok_a_kernel(fd_kernel, RESPUESTA_CREAR_PROCESO_KM);     
                free(path);        
                break;

            }else if(espacio_restate < tamanio){
                log_info(memoria_logger, "No hay suficiente espacio en memoria para crear el proceso con PID %d y tamaño %d bytes.", pid, tamanio);
                enviar_mensaje_error_a_kernel(fd_kernel, RESPUESTA_CREAR_PROCESO_KM);
                free(path);
                break;
            }
            else {
                log_trace(memoria_logger, "Queda espacio libre en memoria: %d bytes", espacio_restate);
                int cantidad_paginas = ceil((float)tamanio / (float)TAM_PAGINA); // Calcular la cantidad de páginas necesarias
                log_trace(memoria_logger, "Cantidad de páginas necesarias: %d", cantidad_paginas);
                
                proceso_t* proceso = crear_estructura_proceso(pid, path, cantidad_paginas);
                pthread_mutex_lock(mutex_lista_procesos);
                list_add(lista_procesos, proceso);
                pthread_mutex_unlock(mutex_lista_procesos);
                
                // Creamos las tablas de páginas del proceso
                proceso->tabla_raiz = crear_tablas_proceso(0, cantidad_paginas); //HAY QUE PONER 0
                
                // Asignamos marcos de memoria al proceso
                if(!asignar_marcos_a_tabla(proceso->tabla_raiz, 0, cantidad_paginas)){
                    log_debug(memoria_logger, "No se pudo crear el proceso: %d", proceso->pid);
                    enviar_mensaje_error_a_kernel(fd_kernel, RESPUESTA_CREAR_PROCESO_KM);
                    destruir_proceso(proceso);
                    free(path);
                    break;
                  }
                  else{
                    pthread_mutex_lock(mutex_lista_procesos);
                    list_add(lista_procesos, proceso);
                    pthread_mutex_unlock(mutex_lista_procesos);
                    enviar_mensaje_ok_a_kernel(fd_kernel, RESPUESTA_CREAR_PROCESO_KM);
                  }
            
            }
            log_info(memoria_logger, "## PID: %d - Proceso Creado - Tamaño: %d", pid, tamanio);
            log_trace(memoria_logger, "Creando proceso con PID %d y tamaño %d bytes.", pid, tamanio);// LOG OBLIGATORIO
            log_trace(memoria_logger, "llegue a crear todo el proceso"); // log de prueba
            free(path);
            break;
        }
        case SUSPENDER_PROCESO_KM: {// [int pid]

            log_trace(memoria_logger, "Recibi el mensaje de suspender proceso.");
            t_buffer* buffer_suspension = recibir_todo_el_buffer(fd_kernel);
            int pid = extraer_int_del_buffer(buffer_suspension);
            proceso_t* proceso = buscar_proceso_por_pid(pid);

            if (proceso == NULL) {
                log_error(memoria_logger, "No se encontró el proceso con PID %d en memoria.", pid);
                enviar_mensaje_error_a_kernel(fd_kernel, RESPUESTA_SUSPENDER_PROCESO_KM);
                break;
            }

            entrada_swap_proceso_t* entrada_swap = malloc(sizeof(entrada_swap_proceso_t));
            entrada_swap->pid = pid;
            entrada_swap->pagina_inicial = tamanio_actual_swap_en_paginas(); // Asignar la pagina inicial
            entrada_swap->cantidad_paginas = proceso->cantidad_paginas; 
            
            usleep(RETARDO_SWAP * 1000);
            suspender_proceso_a_swap(entrada_swap, proceso);
            proceso->bajadas_a_swap ++;

       break;
        }
        case DESUSPENDER_PROCESO_KM:{ // [int pid]
            
            log_trace(memoria_logger, "Recibi el mensaje de desuspender proceso.");
            t_buffer* buffer_desuspension = recibir_todo_el_buffer(fd_kernel);
            int pid_desuspender = extraer_int_del_buffer(buffer_desuspension);
            proceso_t* proceso_desuspender = buscar_proceso_por_pid(pid_desuspender);
            
            usleep(RETARDO_SWAP * 1000);

            
            if (proceso_desuspender == NULL ) {
                log_error(memoria_logger, "No se encontró el proceso con PID %d en memoria.", pid_desuspender);
                enviar_mensaje_error_a_kernel(fd_kernel, RESPUESTA_DESUSPENDER_PROCESO_KM);
                break;
            }

            int tamanio_restante = espacio_libre_en_memoria();
            if (proceso_desuspender->cantidad_paginas  * TAM_PAGINA > tamanio_restante) {
                log_error(memoria_logger, "No hay espacio suficiente en memoria para desuspender el proceso con PID %d.", pid_desuspender);
                enviar_mensaje_error_a_kernel(fd_kernel, RESPUESTA_DESUSPENDER_PROCESO_KM);
                break;
            
            }
            desuspender_proceso_de_swap(proceso_desuspender);
            
            proceso_desuspender->subidas_a_memoria ++;
            
            // Enviar mensaje de OK al Kernel
            enviar_mensaje_ok_a_kernel(fd_kernel, RESPUESTA_DESUSPENDER_PROCESO_KM);
        break;
        }
        case FINALIZAR_PROCESO_KM: // [int pid]
            log_trace(memoria_logger, "Recibi el mensaje de finalizar proceso.");
            t_buffer* buffer_finalizar = recibir_todo_el_buffer(fd_kernel);
            uint32_t pid_finalizar = extraer_uint32_del_buffer(buffer_finalizar);
            proceso_t* proceso_finalizar = buscar_proceso_por_pid(pid_finalizar);

            if (proceso_finalizar == NULL) {
                log_error(memoria_logger, "No se encontró el proceso con PID %d en memoria.", pid_finalizar);
                enviar_mensaje_error_a_kernel(fd_kernel, RESPUESTA_FINALIZAR_PROCESO_KM);
                break;
            }
            // Imprimir métricas del proceso
            mostrar_metricas_proceso(proceso_finalizar);
            

            //si la cantidad de paginas de un proceso es 0, entonces no se tiene que liberar la tabla de paginas
            if (proceso_finalizar->cantidad_paginas == 0){
                log_trace(memoria_logger, "El proceso con PID %d no tiene páginas asignadas, se borra solo de la lista ", pid_finalizar);
                eliminar_proceso_en_memoria(proceso_finalizar->pid);    
                log_trace(memoria_logger, "El proceso con PID %d ha sido finalizado sin páginas asignadas.", pid_finalizar);
                
            }else if(proceso_esta_en_swap(proceso_finalizar)){
                // El proceso está en swap, liberar espacio en swap
                borrar_espacio_swap(buscar_entrada_swap_por_pid(proceso_finalizar->pid)); 
                log_trace(memoria_logger, "Liberando espacio en swap para el proceso con PID %d.", pid_finalizar);
                // Actualizar lista de procesos suspendidos
                eliminar_proceso_en_swap(proceso_finalizar->pid); //sacarlo de la lista de procesos suspendidos
            } else {
                // El proceso está en memoria, liberar espacio en memoria
                liberar_espacio_en_memoria(proceso_finalizar);
                log_trace(memoria_logger, "Liberando espacio en memoria para el proceso con PID %d.", pid_finalizar);
                // Actualizar lista de procesos activos
        
                eliminar_proceso_en_memoria(proceso_finalizar->pid);// sacarlo de la lista de procesos activos
                
            }
            
            log_trace(memoria_logger, "El proceso con PID %d ha sido finalizado.", pid_finalizar);
            // notificar al kernel que el proceso se finalizó correctamente
            enviar_mensaje_ok_a_kernel(fd_kernel, RESPUESTA_FINALIZAR_PROCESO_KM);
            break; 

        case MEMORY_DUMP: {// [int pid]

        t_buffer* buffer_dump = recibir_todo_el_buffer(fd_kernel);
        int pid_dump = extraer_int_del_buffer(buffer_dump);
        proceso_t* proceso_dump = buscar_proceso_por_pid(pid_dump);

        if (proceso_dump == NULL) {
            log_error(memoria_logger, "No se encontró el proceso con PID %d en memoria.", pid_dump);
            t_buffer* buffer_error = crear_buffer();
            cargar_uint32_al_buffer(buffer_error, proceso_dump->pid);
            cargar_int_al_buffer(buffer_error, -1); // -1 indica error
            t_paquete* paquete_error = crear_paquete(RESPUESTA_MEMORY_DUMP, buffer_error);
            enviar_paquete(paquete_error, fd_kernel);
            break;
        }

        // Crear archivo dump
        
        if(crear_archivo_dump(proceso_dump)){
            //notificar al kernel que el dump se realizó correctamente
            t_buffer* buffer_ok = crear_buffer();
            cargar_uint32_al_buffer(buffer_ok, proceso_dump->pid);
            cargar_int_al_buffer(buffer_ok, 1); // 1 = OK
            t_paquete* paquete_ok = crear_paquete(RESPUESTA_MEMORY_DUMP, buffer_ok);
            enviar_paquete(paquete_ok, fd_kernel); 
        }
        log_trace(memoria_logger, "Se realizó el memory dump del proceso con PID %d.", pid_dump);
        //## PID: <PID> - Memory Dump solicitado
        log_info(memoria_logger, "## PID: %d - Memory Dump solicitado ", pid_dump);
        break;
        }
        case -1:
            log_error(memoria_logger, "El KERNEL se desconecto.");
            control_key = 0;
        default:
            log_warning(memoria_logger, "Operacion desconocida de Kernel.");
            break;
        }
    } 
}