#include "../include/memoria_cpu.h"
#include "../include/memoria_proceso.h"
#include "../include/manejo_memoria.h"


void atender_memoria_cpu(int* fd_cpu){
    int fd_cpu_local = *fd_cpu;
    free(fd_cpu);
    bool control_key = 1;
    log_trace(memoria_logger, "FD de CPU recibido: %d", fd_cpu_local);
    while (control_key) {
        int cod_op = recibir_operacion(fd_cpu_local);
        switch (cod_op) {
        case REQUEST_INSTRUCTION: {
            log_trace(memoria_logger, "Recibiendo pedido de instruccion de CPU.");
            t_buffer* temp = recibir_todo_el_buffer(fd_cpu_local);
            uint32_t pc_request= extraer_uint32_del_buffer(temp);
            uint32_t pid_request = extraer_uint32_del_buffer(temp);
            proceso_t* proceso = buscar_proceso_por_pid(pid_request);
            
            usleep(RETARDO_MEMORIA * 1000); 
            
            if (!proceso) {
                log_error(memoria_logger, "Proceso no encontrado para PID=%d", pid_request);
                break;
            }
            if (!proceso->pseudocodigo) {
                log_error(memoria_logger, "Pseudocódigo no cargado para PID=%d", pid_request);
                break;
            }

            log_trace(memoria_logger, "CPU pidió instrucción: PC=%d, PID=%d", pc_request, pid_request);
            log_trace(memoria_logger, "DEBUG: puntero instrucciones: %p, cantidad: %d", 
                    (void*)proceso->pseudocodigo->instrucciones, proceso->pseudocodigo->cantidad);
            log_trace(memoria_logger, "Cantidad de instrucciones: %d", proceso->pseudocodigo->cantidad);

            if (pc_request >= proceso->pseudocodigo->cantidad) {
                log_error(memoria_logger, "PC fuera de rango: PC=%d, cantidad=%d", pc_request, proceso->pseudocodigo->cantidad);
                break;
            }


            char* instruccion = proceso->pseudocodigo->instrucciones[pc_request];
            
            t_buffer* buffer_instruccion = crear_buffer();
            cargar_string_al_buffer(buffer_instruccion, instruccion);


            //cosas que necesita dani
            cargar_int_al_buffer(buffer_instruccion, ENTRADAS_POR_TABLA);
            cargar_int_al_buffer(buffer_instruccion, CANTIDAD_NIVELES);
            cargar_int_al_buffer(buffer_instruccion, TAM_PAGINA);

            t_paquete* paquete_instruccion = crear_paquete(INSTRUCTION_REQUESTED, buffer_instruccion);
            enviar_paquete(paquete_instruccion, fd_cpu_local);
            //## PID: <PID> - Obtener instrucción: <PC> - Instrucción: <INSTRUCCIÓN> <...ARGS>
            //sacarle el \n al final de la instruccion

            if (instruccion[strlen(instruccion) - 1] == '\n') {
                instruccion[strlen(instruccion) - 1] = '\0';
            }

            //actualizar las metricas 
            proceso->instrucciones_solicitadas++;
            log_info(memoria_logger, "## PID: %d - Obtener instrucción: %d - Instrucción: %s", pid_request, pc_request, instruccion);
            
            break;
        }
        case REQUEST_FRAME: {
            log_trace(memoria_logger, "Recibiendo pedido de marco de CPU.");
            t_buffer* buffer = recibir_todo_el_buffer(fd_cpu_local);
            uint32_t pid_frame = extraer_uint32_del_buffer(buffer);
            log_trace(memoria_logger, "PID recibido: %d", pid_frame);

            uint32_t indices[CANTIDAD_NIVELES];
            for (int i = 0; i < CANTIDAD_NIVELES; i++) {
                indices[i] = extraer_uint32_del_buffer(buffer);
                log_trace(memoria_logger, "Índice nivel %d: %u", i + 1, indices[i]);
            }

            usleep(RETARDO_MEMORIA * CANTIDAD_NIVELES * 1000);

            proceso_t* proceso = buscar_proceso_por_pid(pid_frame);
            if (!proceso || !proceso->tabla_raiz) {
                log_error(memoria_logger, "Proceso no encontrado o tabla de páginas no inicializada.");
                //destruir_buffer(buffer);
                break;
            }
            log_trace(memoria_logger, "Tabla de páginas encontrada para PID: %d", pid_frame);

            tabla_pagina_t* tabla = proceso->tabla_raiz;
            int paginas_restantes = proceso->cantidad_paginas;
            int resultado = -1;

            // Recorrido de niveles intermedios
            for (int nivel = 0; nivel < CANTIDAD_NIVELES - 1; nivel++) {
                int paginas_por_subtabla = pow(ENTRADAS_POR_TABLA, CANTIDAD_NIVELES - nivel - 1);
                int subtablas_esperadas = (paginas_restantes <= paginas_por_subtabla) ? 1 : ceil((double)paginas_restantes / paginas_por_subtabla);

                if (!tabla->subtablas || indices[nivel] >= subtablas_esperadas) {
                    log_error(memoria_logger, "Acceso fuera de rango en nivel %d (índice %d, subtablas esperadas %d)", nivel, indices[nivel], subtablas_esperadas);
                    tabla = NULL;
                    break;
                }
                tabla = tabla->subtablas[indices[nivel]];
                proceso->accesos_tablas_paginas++;
                usleep(RETARDO_MEMORIA * 1000);
                paginas_restantes -= paginas_por_subtabla * indices[nivel];
            }

            log_trace(memoria_logger, "Tabla de páginas en nivel %d: %p", CANTIDAD_NIVELES - 1, tabla);

            if (!tabla) {
                log_error(memoria_logger, "No se pudo acceder a la tabla de páginas (acceso fuera de rango).");
                t_buffer* respuesta = crear_buffer();
                cargar_int_al_buffer(respuesta, -1);
                t_paquete* paquete = crear_paquete(RESPUESTA_REQUEST_FRAME, respuesta);
                enviar_paquete(paquete, fd_cpu_local);
                destruir_buffer(buffer);
                log_error(memoria_logger, "Marco enviado a CPU: -1");
                break;
            }

            // Chequeo de rango en el último nivel (tabla final)
            if (tabla->es_entrada_final) {
                int entradas_final = paginas_restantes < ENTRADAS_POR_TABLA ? paginas_restantes : ENTRADAS_POR_TABLA;
                if (indices[CANTIDAD_NIVELES - 1] >= entradas_final) {
                    log_error(memoria_logger, "Índice fuera de rango en último nivel (índice %d, entradas %d)", indices[CANTIDAD_NIVELES - 1], entradas_final);
                    resultado = -1;
                } else {
                    int* marco_ptr = &tabla->marcos[indices[CANTIDAD_NIVELES - 1]];
                    if (*marco_ptr == -1) {
                        int nuevo_marco = asignar_marco_libre();
                        if (nuevo_marco == -1) {
                            log_error(memoria_logger, "No hay marcos libres para asignar.");
                            resultado = -1;
                        } else {
                            *marco_ptr = nuevo_marco;
                            resultado = nuevo_marco;
                            log_trace(memoria_logger, "Asignado marco %d a PID %d, entrada %d", nuevo_marco, pid_frame, indices[CANTIDAD_NIVELES - 1]);
                        }
                    } else {
                        resultado = *marco_ptr;
                    }
                }
            } else {
                log_error(memoria_logger, "No se llegó a una tabla final.");
                resultado = -1;
            }

            log_trace(memoria_logger, "Resultado del acceso a tabla de páginas: %d", resultado);

            // Armar buffer de respuesta y enviar
            t_buffer* respuesta = crear_buffer();
            cargar_int_al_buffer(respuesta, resultado);
            t_paquete* paquete = crear_paquete(RESPUESTA_REQUEST_FRAME, respuesta);
            enviar_paquete(paquete, fd_cpu_local);
            log_debug(memoria_logger, "Marco enviado a CPU: %d", resultado);

            proceso->accesos_tablas_paginas += CANTIDAD_NIVELES;
            //destruir_buffer(buffer);
            break;
        }
        
        case REQUEST_READ:{
            
            log_trace(memoria_logger, "Recibiendo pedido de lectura de CPU.");
            t_buffer* buffer_read = recibir_todo_el_buffer(fd_cpu_local);
            uint32_t pid_read = extraer_uint32_del_buffer(buffer_read);
            uint32_t direccion_fisica = extraer_uint32_del_buffer(buffer_read);
            int tamanio = extraer_int_del_buffer(buffer_read);
            //destruir_buffer(buffer_read);
            usleep(RETARDO_MEMORIA * 1000);//no entiendo si hay que poner un retardo aca pero bueno, lo dejamos

            proceso_t* proceso_read = buscar_proceso_por_pid(pid_read);
            if (!proceso_read || !proceso_read->tabla_raiz) {
                log_error(memoria_logger, "Proceso no encontrado o tabla de páginas no inicializada.");
                break;
            }

             // Validar rango
            if (direccion_fisica + tamanio > TAM_MEMORIA) {
                log_error(memoria_logger, "Lectura fuera de rango de memoria.");
                break;
            }
            
            // Leer del espacio de usuario
            void* buffer_envio = malloc(tamanio);
            memcpy(buffer_envio, ((char*)espacio_usuario) + direccion_fisica, tamanio);
            log_trace(memoria_logger, "Contenido copiado (string): '%.*s'", tamanio, (char*)buffer_envio);
            
            // Enviar respuesta a CPU
            
            t_buffer* buffer_respuesta = crear_buffer();
            cargar_contenido_al_buffer(buffer_respuesta, buffer_envio, tamanio);
            t_paquete* paquete_respuesta = crear_paquete(RESPUESTA_REQUEST_READ, buffer_respuesta);
            enviar_paquete(paquete_respuesta, fd_cpu_local);
            free(buffer_envio);

            // Actualizar métricas del proceso
            proceso_read->lecturas_memoria++;
            log_info(memoria_logger, "## PID: %d - Lectura - Dir. Física: %u - Tamaño: %d", pid_read, direccion_fisica, tamanio);
            
            
            break;
        }
        case REQUEST_WRITE:{ // [pid],[direccion_fisica], [tamanio], [datos]
            
            log_trace(memoria_logger, "Recibiendo pedido de escritura de CPU.");
            t_buffer* buffer_write = recibir_todo_el_buffer(fd_cpu_local);
            uint32_t pid = extraer_uint32_del_buffer(buffer_write);
            uint32_t direccion_fisica_write = extraer_uint32_del_buffer(buffer_write);
            int tamanio_write = extraer_int_del_buffer(buffer_write);
            char* datos = extraer_string_del_buffer(buffer_write);
            //destruir_buffer(buffer_write);
            //void* datos = buffer_write->stream + (sizeof(uint32_t) + sizeof(uint32_t) + sizeof(int));
            log_trace(memoria_logger, "PID: %d, Dirección Física: %d, Tamaño: %d", pid, direccion_fisica_write, tamanio_write);
            usleep(RETARDO_MEMORIA * 1000);//no entiendo si hay

            // Validar que no este fuera de rango
            if (direccion_fisica_write + tamanio_write > TAM_MEMORIA) {
                    log_error(memoria_logger, "Escritura fuera de rango de memoria.");
                    t_buffer* buffer_respuesta = crear_buffer();
                    cargar_int_al_buffer(buffer_respuesta, -1); // -1 indica error
                    t_paquete* paquete_respuesta = crear_paquete(RESPUESTA_REQUEST_WRITE, buffer_respuesta);
                    enviar_paquete(paquete_respuesta, fd_cpu_local);                
                    destruir_buffer(buffer_write);
                    free(datos);
                break;
            }
            
            proceso_t* proceso_write = buscar_proceso_por_pid(pid);
                if (!proceso_write || !proceso_write->tabla_raiz) {
                        log_error(memoria_logger, "Proceso no encontrado o tabla de páginas no inicializada.");
                        t_buffer* buffer_respuesta = crear_buffer();
                        cargar_int_al_buffer(buffer_respuesta, -1); // -1 indica error
                        t_paquete* paquete_respuesta = crear_paquete(RESPUESTA_REQUEST_WRITE, buffer_respuesta);
                        enviar_paquete(paquete_respuesta, fd_cpu_local);
                        destruir_buffer(buffer_write);
                        free(datos);
                    break;
                }

            if (tamanio_write > 0 && datos[tamanio_write - 1] == '\n') {
                tamanio_write--;
            }
            
            memcpy(((char*)espacio_usuario) + direccion_fisica_write, datos, tamanio_write);
            log_trace(memoria_logger, "Escribiendo en espacio de usuario: PID=%d, Dir. Física=%u, Tamaño=%d", pid, direccion_fisica_write, tamanio_write);
 
            //loggear espacio de usuario para ver si se escribio bien
            log_trace(memoria_logger, "Contenido escrito en espacio de usuario: '%.*s'", tamanio_write, (char*)espacio_usuario + direccion_fisica_write);
            
            // Enviar respuesta OK
             

            t_buffer* buffer_respuesta = crear_buffer();
            cargar_int_al_buffer(buffer_respuesta, 1); // 1 indica éxito
            t_paquete* paquete_respuesta = crear_paquete(RESPUESTA_REQUEST_WRITE, buffer_respuesta);
            enviar_paquete(paquete_respuesta, fd_cpu_local);

            // Actualizar métricas del proceso
            proceso_write->escrituras_memoria++;
            log_info(memoria_logger, "## PID: %d - Escritura - Dir. Física: %u - Tamaño: %d", pid, direccion_fisica_write, tamanio_write);
            log_trace(memoria_logger, "Datos escritos: %.*s", tamanio_write, (char*)datos); // Mostrar los datos escritos
            break;
        }
        case REQUEST_READ_FULL_PAGE: {
            log_trace(memoria_logger, "Recibiendo pedido de lectura de página completa.");
            t_buffer* buffer = recibir_todo_el_buffer(fd_cpu_local);
            uint32_t pid_read_full = extraer_uint32_del_buffer(buffer);
            uint32_t direccion_fisica = extraer_uint32_del_buffer(buffer);

            log_trace(memoria_logger, "PID: %d, Dirección Física: %u", pid_read_full, direccion_fisica);
            usleep(RETARDO_MEMORIA * 1000);

            if (direccion_fisica % TAM_PAGINA != 0) {
                log_error(memoria_logger, "Dirección física no alineada a página.");
                destruir_buffer(buffer);
                break;
            }
            if (direccion_fisica + TAM_PAGINA > TAM_MEMORIA) {
                log_error(memoria_logger, "Lectura fuera de rango de memoria.");
                destruir_buffer(buffer);
                break;
            }

            void* buffer_envio = malloc(TAM_PAGINA);
            memcpy(buffer_envio, ((char*)espacio_usuario) + direccion_fisica, TAM_PAGINA);
            log_trace(memoria_logger, "Contenido copiado (string): '%.*s'", TAM_PAGINA, (char*)buffer_envio);

            t_buffer* buffer_respuesta = crear_buffer();
            cargar_contenido_al_buffer(buffer_respuesta, buffer_envio, TAM_PAGINA);
            t_paquete* paquete_respuesta = crear_paquete(RESPUESTA_REQUEST_READ_FULL_PAGE, buffer_respuesta);
            enviar_paquete(paquete_respuesta, fd_cpu_local);

            //free(buffer_envio);
            //destruir_buffer(buffer);

            log_trace(memoria_logger, "## PID: %d - Lectura de página completa - Dir. Física: %u", pid_read_full, direccion_fisica);
            break;
        }

        case REQUEST_WRITE_FULL_PAGE: {
            t_buffer* buffer = recibir_todo_el_buffer(fd_cpu_local);
            uint32_t pid_write_full = extraer_uint32_del_buffer(buffer);
            uint32_t direccion_fisica = extraer_uint32_del_buffer(buffer);
            char* contenido = extraer_string_del_buffer(buffer);

            usleep(RETARDO_MEMORIA * 1000);
            
            if (direccion_fisica % TAM_PAGINA != 0) {
                log_error(memoria_logger, "Dirección física no alineada a página.");
                destruir_buffer(buffer);
                free(contenido);
                break;
            }
            if (direccion_fisica + TAM_PAGINA > TAM_MEMORIA) {
                log_error(memoria_logger, "Escritura fuera de rango de memoria.");
                destruir_buffer(buffer);
                free(contenido);
                break;
            }

            // Limpiar la página antes de escribir (opcional, pero seguro)c
            memset(((char*)espacio_usuario) + direccion_fisica, 0, TAM_PAGINA);

            // Copiar solo la cantidad de bytes del string, sin pasarse
            size_t len = strlen(contenido);
            if (len > TAM_PAGINA) len = TAM_PAGINA;
            memcpy(((char*)espacio_usuario) + direccion_fisica, contenido, len);

            // Enviar respuesta OK
            t_buffer* buffer_respuesta = crear_buffer();
            cargar_string_al_buffer(buffer_respuesta, "OK");
            t_paquete* paquete_respuesta = crear_paquete(RESPUESTA_REQUEST_WRITE_FULL_PAGE, buffer_respuesta);
            enviar_paquete(paquete_respuesta, fd_cpu_local);
            free(contenido);

            log_trace(memoria_logger, "## PID: %d - Escritura de página completa - Dir. Física: %u", pid_write_full, direccion_fisica);
            break;
        }

        case -1:
            log_error(memoria_logger, "El CPU se desconecto.");
            control_key = 0;
        default:
            log_warning(memoria_logger, "Operacion desconocida de CPU.");
            break;
        }
        
    }
}