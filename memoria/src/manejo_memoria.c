#include "../include/manejo_memoria.h"

/* bool hay_espacio_libre(int tamanio) {
    pthread_mutex_lock(mutex_memoria_ocupada);
    int espacio_libre = TAM_MEMORIA - MEMORIA_OCUPADA;
    pthread_mutex_unlock(mutex_memoria_ocupada);
    return espacio_libre >= tamanio;
}
 */
void inicializar_espacio_de_usuario() { // funcan los logs
    espacio_usuario = malloc(TAM_MEMORIA);
    memset(espacio_usuario, 0, TAM_MEMORIA);
    if (!espacio_usuario) {
        log_error(memoria_logger, "No se pudo reservar el espacio de usuario");
        exit(EXIT_FAILURE);
    }
    log_trace(memoria_logger, "Espacio de usuario inicializado (%d bytes)", TAM_MEMORIA);
}

void inicializar_frames_maximos(){ //17/6
    // Calcular la cantidad de frames máximos
    framesMaximos = TAM_MEMORIA / TAM_PAGINA;
    log_trace(memoria_logger, "Cantidad de frames máximos: %d", framesMaximos);
    bitmap = malloc(framesMaximos * sizeof(bool));
}

tabla_pagina_t* crear_tablas_proceso(int nivel_actual, int paginas_restantes) {
    tabla_pagina_t* tabla = malloc(sizeof(tabla_pagina_t));
    tabla->es_entrada_final = (nivel_actual == CANTIDAD_NIVELES - 1);

    if (tabla->es_entrada_final) {
        int entradas = paginas_restantes < ENTRADAS_POR_TABLA ? paginas_restantes : ENTRADAS_POR_TABLA;
        tabla->cantidad_entradas = entradas; //AAAAAAAAA
        tabla->marcos = calloc(entradas, sizeof(int));
        for (int i = 0; i < entradas; i++)
            tabla->marcos[i] = -1;
        log_trace(memoria_logger, "[Nivel %d] Tabla final: %d entradas", nivel_actual, entradas);
    } else {
        int paginas_por_subtabla = pow(ENTRADAS_POR_TABLA, CANTIDAD_NIVELES - nivel_actual - 1);

        // Si todas las páginas entran en una sola rama, solo creá una subtabla
        int subtablas_necesarias = (paginas_restantes <= paginas_por_subtabla) ? 1 : ceil((float)paginas_restantes / paginas_por_subtabla);
        tabla->cantidad_entradas = subtablas_necesarias; //AAAAAAAAA
        tabla->subtablas = calloc(subtablas_necesarias, sizeof(tabla_pagina_t*));
        log_trace(memoria_logger, "[Nivel %d] Subtablas necesarias: %d", nivel_actual, subtablas_necesarias);

        for (int i = 0; i < subtablas_necesarias; i++) {
            int paginas_en_esta_subtabla = paginas_restantes < paginas_por_subtabla ? paginas_restantes : paginas_por_subtabla;
            tabla->subtablas[i] = crear_tablas_proceso(nivel_actual + 1, paginas_en_esta_subtabla);
            paginas_restantes -= paginas_en_esta_subtabla;
        }
    }
    return tabla;
}

void enviar_mensaje_ok_a_kernel(int fd_kernel, int cod_op){
    t_buffer* buffer_respuesta = crear_buffer();
    cargar_int_al_buffer(buffer_respuesta, 1); // 1 = funca
    t_paquete* paquete_respuesta = crear_paquete(cod_op, buffer_respuesta);
    log_trace(memoria_logger, "Enviando mensaje de OK al Kernel: %d",cod_op);
    enviar_paquete(paquete_respuesta, fd_kernel);
}

void enviar_mensaje_error_a_kernel(int fd_kernel, int cod_op){
    t_buffer* buffer_respuesta = crear_buffer();
    cargar_int_al_buffer(buffer_respuesta, -1); // 0 = error
    t_paquete* paquete_respuesta = crear_paquete(cod_op, buffer_respuesta);
    log_trace(memoria_logger, "Enviando mensaje de error al Kernel: %d",cod_op);
    enviar_paquete(paquete_respuesta, fd_kernel);
}

int asignar_marco_libre(){
    pthread_mutex_lock(mutex_bitmap);
    for (int i = 0; i < framesMaximos; i++) {
        if (!bitmap[i]) {
            bitmap[i] = true;
            pthread_mutex_unlock(mutex_bitmap);
            return i;
        }
    }
    pthread_mutex_unlock(mutex_bitmap);
    return -1; // No hay marcos libres
}

/* bool asignar_marcos_a_tabla(tabla_pagina_t* tabla, int nivel_actual, int cantidad_marcos) {
    if (!tabla) return false;

    if (tabla->es_entrada_final) {
    int entradas = cantidad_marcos < ENTRADAS_POR_TABLA ? cantidad_marcos : ENTRADAS_POR_TABLA;
    for (int i = 0; i < entradas; i++) {
        if (tabla->marcos[i] == -1) {
            int marco_libre = asignar_marco_libre();
            if (marco_libre != -1) {
                tabla->marcos[i] = marco_libre;
                pthread_mutex_lock(mutex_memoria_ocupada);
                MEMORIA_OCUPADA += TAM_PAGINA;
                log_trace(memoria_logger, "MEMORIA_OCUPADA incrementada: %d", MEMORIA_OCUPADA);
                pthread_mutex_unlock(mutex_memoria_ocupada);
                log_trace(memoria_logger, "Asignado marco %d a página %d del proceso", marco_libre, i);
            } else {
                log_error(memoria_logger, "No hay marcos libres para asignar a página %d", i);
                return false;
            }
        }
    }

    }else {
        // Niveles intermedios: recorrer subtablas
        int paginas_por_subtabla = pow(ENTRADAS_POR_TABLA, CANTIDAD_NIVELES - nivel_actual - 1);
        int paginas_restantes = cantidad_marcos;
        for (int i = 0; tabla->subtablas[i] != NULL && paginas_restantes > 0; i++) {
            int paginas_en_esta_subtabla = paginas_restantes < paginas_por_subtabla ? paginas_restantes : paginas_por_subtabla;
            asignar_marcos_a_tabla(tabla->subtablas[i], nivel_actual + 1, paginas_en_esta_subtabla);
            paginas_restantes -= paginas_en_esta_subtabla;
        }
    log_trace(memoria_logger, "Asignación de marcos en tabla de nivel %d completada", nivel_actual);
    return true; // Asignación exitosa
}
} */

bool asignar_marcos_a_tabla_rec(tabla_pagina_t* tabla, int nivel_actual, int cantidad_marcos, int* pagina_logica) {
    if (!tabla) return false;

    if (tabla->es_entrada_final) {
        int entradas = cantidad_marcos < ENTRADAS_POR_TABLA ? cantidad_marcos : ENTRADAS_POR_TABLA;
        for (int i = 0; i < entradas; i++) {
            if (tabla->marcos[i] == -1) {
                int marco_libre = asignar_marco_libre();
                if (marco_libre != -1) {
                    tabla->marcos[i] = marco_libre;
                    pthread_mutex_lock(mutex_memoria_ocupada);
                    MEMORIA_OCUPADA += TAM_PAGINA;
                    log_trace(memoria_logger, "MEMORIA_OCUPADA incrementada: %d", MEMORIA_OCUPADA);
                    pthread_mutex_unlock(mutex_memoria_ocupada);
                    log_trace(memoria_logger, "Asignado marco %d a página lógica %d (entrada %d de tabla final)", marco_libre, *pagina_logica, i);
                } else {
                    log_debug(memoria_logger, "No hay marcos libres para asignar a página lógica %d", *pagina_logica);
                    return false;
                }
            }
            (*pagina_logica)++; // Avanzar el número de página lógica global
        }
    } else {
        int paginas_por_subtabla = pow(ENTRADAS_POR_TABLA, CANTIDAD_NIVELES - nivel_actual - 1);
        int paginas_restantes = cantidad_marcos;
        for (int i = 0; tabla->subtablas[i] != NULL && paginas_restantes > 0; i++) {
            int paginas_en_esta_subtabla = paginas_restantes < paginas_por_subtabla ? paginas_restantes : paginas_por_subtabla;
            if (!asignar_marcos_a_tabla_rec(tabla->subtablas[i], nivel_actual + 1, paginas_en_esta_subtabla, pagina_logica))
                return false;
            paginas_restantes -= paginas_en_esta_subtabla;
        }
    }
    log_trace(memoria_logger, "Asignación de marcos en tabla de nivel %d completada", nivel_actual);
    return true;
}

// Wrapper para mantener la firma original
bool asignar_marcos_a_tabla(tabla_pagina_t* tabla, int nivel_actual, int cantidad_marcos) {
    int pagina_logica = 0;
    return asignar_marcos_a_tabla_rec(tabla, nivel_actual, cantidad_marcos, &pagina_logica);
}

int espacio_libre_en_memoria(){
    
    pthread_mutex_lock(mutex_memoria_ocupada);
    int espacio_libre = TAM_MEMORIA - MEMORIA_OCUPADA;
    pthread_mutex_unlock(mutex_memoria_ocupada);
    
    //log_trace(memoria_logger, "Espacio libre en memoria: %d bytes", espacio_libre);
    return espacio_libre;
}