#include "../include/memoria_swap.h"
#include "../include/manejo_memoria.h"

void inicializar_swap() {
    archivo_swap = fopen(PATH_SWAPFILE, "r+b");
    if (!archivo_swap) {
        // Si no existe, crearlo
        archivo_swap = fopen(PATH_SWAPFILE, "w+b");
        if (!archivo_swap) {
            log_error(memoria_logger, "No se pudo crear el archivo de swap");
            exit(EXIT_FAILURE);
        }
    }
    // Limpiar el archivo swap al iniciar memoria
    int fd = fileno(archivo_swap);
    ftruncate(fd, 0); // Trunca el archivo a tamaño 0 (lo borra)
    rewind(archivo_swap); // Vuelve al inicio del archivo

    procesos_en_swap = list_create();
}

void suspender_proceso_a_swap(entrada_swap_proceso_t* entrada, proceso_t* proceso) {
    log_trace(memoria_logger, "Memoria restante antes de suspender: %d bytes", MEMORIA_OCUPADA);
    pthread_mutex_lock(mutex_procesos_en_swap);
    list_add(procesos_en_swap, entrada);
    pthread_mutex_unlock(mutex_procesos_en_swap);
    log_trace(memoria_logger, "Proceso %d suspendido a swap: Pagina inicial %d,Cantidad de paginas %d", 
        entrada->pid, entrada->pagina_inicial, entrada->cantidad_paginas);

     // Escribir las páginas del proceso en el archivo swap

    t_list* lista_marcos = obtener_marcos_asignados_suspender(proceso); // esto tambien libera los marcos en el bitmap y en la tabla de páginas :)
    log_trace(memoria_logger, "cantidad_paginas: %d", proceso->cantidad_paginas);
    log_trace(memoria_logger, "lista_marcos size: %d", list_size(lista_marcos));

     for (int i = 0; i < list_size(lista_marcos); i++) {
        int* marco_ptr = list_get(lista_marcos, i);
        log_trace(memoria_logger, "marco_ptr: %p", marco_ptr);
        int marco = *marco_ptr;
        free(marco_ptr);

        // Offset: la posición en el swap donde está la página i del proceso
        off_t offset = (entrada->pagina_inicial + i) * TAM_PAGINA; // calcular desplazamiento del swap en bytes
        fseek(archivo_swap, offset, SEEK_SET);
        
        void* pagina_mem = espacio_usuario + (marco * TAM_PAGINA);
        fwrite(pagina_mem, TAM_PAGINA, 1, archivo_swap); 
        }
        
    list_destroy(lista_marcos);  
    fflush(archivo_swap);
    log_trace(memoria_logger, "Memoria restante después de suspender: %d bytes", MEMORIA_OCUPADA);
}

t_list* obtener_marcos_asignados_suspender(proceso_t* proceso) {
    log_trace(memoria_logger, "Obteniendo marcos asignados para el proceso %d", proceso->pid);
    t_list* lista_marcos = list_create();
    recolectar_marcos_suspender(proceso->tabla_raiz, 0, lista_marcos, proceso->cantidad_paginas);
    return lista_marcos;
}

void recolectar_marcos_suspender(tabla_pagina_t* tabla, int nivel_actual, t_list* lista_marcos, int cantidad_paginas) {
    if (!tabla) return;
    log_trace(memoria_logger, "Recolectando marcos en nivel %d", nivel_actual);
    if (tabla->es_entrada_final) {
        pthread_mutex_lock(mutex_bitmap);
        for (int i = 0; i < cantidad_paginas; i++) { // SOLO HASTA cantidad_paginas
            int marco = tabla->marcos[i];
            if (marco != -1) {
                int* marco_ptr = malloc(sizeof(int));
                *marco_ptr = marco;
                tabla->marcos[i] = -1;
                bitmap[marco] = false;
                pthread_mutex_lock(mutex_memoria_ocupada);
                MEMORIA_OCUPADA -= TAM_PAGINA;
                pthread_mutex_unlock(mutex_memoria_ocupada);
                list_add(lista_marcos, marco_ptr);
            }
        }
        pthread_mutex_unlock(mutex_bitmap);
    } else {
        int paginas_por_subtabla = pow(ENTRADAS_POR_TABLA, CANTIDAD_NIVELES - nivel_actual - 1);
        int paginas_restantes = cantidad_paginas;
        for (int i = 0; i < ENTRADAS_POR_TABLA && paginas_restantes > 0; i++) {
            if (tabla->subtablas[i] != NULL) {
                int paginas_a_recolectar = paginas_restantes < paginas_por_subtabla ? paginas_restantes : paginas_por_subtabla;
                recolectar_marcos_suspender(tabla->subtablas[i], nivel_actual + 1, lista_marcos, paginas_a_recolectar);
                paginas_restantes -= paginas_a_recolectar;
            }
        }
    }
}


// Uso:
t_list* obtener_marcos_asignados(proceso_t* proceso) {
    log_trace(memoria_logger, "Obteniendo marcos asignados para el proceso %d", proceso->pid);
    t_list* lista_marcos = list_create();
    recolectar_marcos(proceso->tabla_raiz, 0, lista_marcos, proceso->cantidad_paginas);
    return lista_marcos;
}

void recolectar_marcos(tabla_pagina_t* tabla, int nivel_actual, t_list* lista_marcos, int cantidad_paginas) {
    if (!tabla) return;
    log_trace(memoria_logger, "Recolectando marcos en nivel %d", nivel_actual);
    if (tabla->es_entrada_final) {
        // Último nivel: recorrer marcos
        pthread_mutex_lock(mutex_bitmap);
        for (int i = 0; i < ENTRADAS_POR_TABLA; i++) {
            int marco = tabla->marcos[i];
            if (marco != -1) {
                int* marco_ptr = malloc(sizeof(int));
                *marco_ptr = marco;
                tabla->marcos[i] = -1; // Liberar el marco en la tabla
                bitmap[marco] = false; // Liberar el marco en el bitmap
                pthread_mutex_lock(mutex_memoria_ocupada);
                MEMORIA_OCUPADA -= TAM_PAGINA; // Actualizar memoria ocupada
                pthread_mutex_unlock(mutex_memoria_ocupada);
                list_add(lista_marcos, marco_ptr);
            }
        }
        pthread_mutex_unlock(mutex_bitmap);
    } else {
        // Niveles intermedios: recorrer subtablas
        for (int i = 0; i < ENTRADAS_POR_TABLA; i++) {
            if (tabla->subtablas[i] != NULL) {
                recolectar_marcos(tabla->subtablas[i], nivel_actual + 1, lista_marcos, ENTRADAS_POR_TABLA);
            }
        }
    }
}

void desuspender_proceso_de_swap(proceso_t* proceso) {
    int pid = proceso->pid;
    log_trace(memoria_logger, "Desuspendiendo proceso %d de swap", pid);
    
    // Buscar la entrada en swap
    entrada_swap_proceso_t* entrada = buscar_entrada_swap_por_pid(pid);
    if (!entrada) {
        log_debug(memoria_logger, "No se encontró el proceso %d en swap", pid);
        enviar_mensaje_error_a_kernel(fd_kernel, RESPUESTA_DESUSPENDER_PROCESO_KM);
        return;
    }

    // 1. Asignar marcos a todas las páginas del proceso
    //asignar_marcos_a_tabla(proceso->tabla_raiz, 0, entrada->cantidad_paginas); // que pasa si no hay marcos disponibles? faltaria un if aca?
    log_debug(memoria_logger, "Asignación de marcos completada para el proceso %d.", pid);

    if(!asignar_marcos_a_tabla(proceso->tabla_raiz, 0, entrada->cantidad_paginas)) {
        log_debug(memoria_logger, "No se pudieron asignar marcos para el proceso %d. No hay marcos libres.", pid);
        enviar_mensaje_error_a_kernel(fd_kernel, RESPUESTA_DESUSPENDER_PROCESO_KM);
        return;
    }
    else{
        // 2. Leer cada página del swap y copiarla al marco asignado
        for (int i = 0; i < entrada->cantidad_paginas; i++) {
            // Leer página de swap
            off_t offset = (entrada->pagina_inicial + i) * TAM_PAGINA;
            fseek(archivo_swap, offset, SEEK_SET);
            void* buffer_pagina = malloc(TAM_PAGINA);
            fread(buffer_pagina, TAM_PAGINA, 1, archivo_swap);

            // Obtener el marco físico asignado a la página lógica i
            int marco = obtener_marco_de_pagina(proceso->tabla_raiz, i);

            // Copiar el contenido leído a la memoria principal
            memcpy(espacio_usuario + (marco * TAM_PAGINA), buffer_pagina, TAM_PAGINA);

            free(buffer_pagina);
        }
        log_debug(memoria_logger, "Pase el for de desuspender proceso de swap para el proceso %d.", pid);
        // 3. Limpiar el espacio en swap
        borrar_espacio_swap(entrada);
        log_debug(memoria_logger, "Espacio en swap liberado para el proceso %d.", pid);
        // 4. Remover la entrada de la lista de procesos en swap  
        pthread_mutex_lock(mutex_procesos_en_swap);
        bool unlocked = false;
        for(int i = 0; i < list_size(procesos_en_swap); i++) {
            entrada_swap_proceso_t* entrada_lista = list_get(procesos_en_swap, i);
            if(entrada_lista->pid == pid) {
                list_remove(procesos_en_swap, i);
                free(entrada_lista);
                pthread_mutex_unlock(mutex_procesos_en_swap);
                unlocked = true;
                break;
            }
        }
        if (!unlocked) pthread_mutex_unlock(mutex_procesos_en_swap);
        log_trace(memoria_logger, "Proceso %d desuspendido de swap", pid);
        fflush(archivo_swap);
    }

}

int obtener_marco_de_pagina(tabla_pagina_t* tabla, int nro_pagina_logica) {
    int indices[CANTIDAD_NIVELES];
    int pagina = nro_pagina_logica;
    for (int nivel = CANTIDAD_NIVELES - 1; nivel >= 0; nivel--) {
        indices[nivel] = pagina % ENTRADAS_POR_TABLA;
        pagina /= ENTRADAS_POR_TABLA;
    }
    for (int nivel = 0; nivel < CANTIDAD_NIVELES - 1; nivel++) {
        tabla = tabla->subtablas[indices[nivel]];
    }
    return tabla->marcos[indices[CANTIDAD_NIVELES - 1]];
}

void borrar_espacio_swap(entrada_swap_proceso_t* entrada) { // Aclaro que no borra una chota, solo pone espacios en blanco en donde estaba el proceso
    char* buffer_espacios = malloc(TAM_PAGINA);
    memset(buffer_espacios, ' ', TAM_PAGINA); // llenar el buffer con espacios
    for (int i = 0; i < entrada->cantidad_paginas; i++) {
        off_t offset = (entrada->pagina_inicial + i) * TAM_PAGINA;
        fseek(archivo_swap, offset, SEEK_SET);
        fwrite(buffer_espacios, TAM_PAGINA, 1, archivo_swap);
    }
    free(buffer_espacios);
    fflush(archivo_swap);
}

bool es_entrada_buscada(void* elem, void* pid_ptr) {
    int pid = *(int*)pid_ptr;
    return ((entrada_swap_proceso_t*)elem)->pid == pid;
}

entrada_swap_proceso_t* buscar_entrada_swap_por_pid(int pid) {
    pthread_mutex_lock(mutex_procesos_en_swap);
    for (int i = 0; i < list_size(procesos_en_swap); i++) {
        entrada_swap_proceso_t* entrada = list_get(procesos_en_swap, i);
        if (entrada->pid == pid){
        pthread_mutex_unlock(mutex_procesos_en_swap);
        return entrada;
        }
    }
    pthread_mutex_unlock(mutex_procesos_en_swap);
    return NULL;
}

int tamanio_actual_swap_en_paginas(){
    fseek(archivo_swap, 0, SEEK_END);
    long bytes = ftell(archivo_swap);
    return bytes / TAM_PAGINA; // Convertir a páginas
}
