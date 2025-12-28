#include "../include/memoria_proceso.h"
t_list* lista_procesos = NULL;

void inicializar_lista_procesos() {
        lista_procesos = list_create(); 
}

proceso_t* crear_proceso(uint32_t pid) {
    proceso_t* nuevo = malloc(sizeof(proceso_t));
    nuevo->pid = pid;
    nuevo->tabla_raiz = NULL;
    // Inicializá otros campos si es necesario
    return nuevo;
}

// Podés agregar más funciones, por ejemplo:
// - buscar_proceso_por_pid
proceso_t* buscar_proceso_por_pid(uint32_t pid) {
    pthread_mutex_lock(mutex_lista_procesos);
    for (int i = 0; i < list_size(lista_procesos); i++) {
        proceso_t* p = list_get(lista_procesos, i);
        if (p->pid == pid){
            pthread_mutex_unlock(mutex_lista_procesos);
            return p;
        }
    }
    pthread_mutex_unlock(mutex_lista_procesos);
    return NULL;
}

// DESTRUCCION DEL PROCESO 
void destruir_pseudocodigo(pseudocodigo_t* pc) {
    if (!pc) return;
    for (int i = 0; i < pc->cantidad; i++) {
        free(pc->instrucciones[i]);
    }
    free(pc->instrucciones);
    free(pc);
}

/* void destruir_tabla_pagina(tabla_pagina_t* tabla, int nivel_actual) {
    if (!tabla) return;
    if (tabla->es_entrada_final) {
        free(tabla->marcos);
    } else {
        for (int i = 0; i < ENTRADAS_POR_TABLA; i++) {
            destruir_tabla_pagina(tabla->subtablas[i], nivel_actual + 1);
        }
        free(tabla->subtablas);
    }
    free(tabla);
} */
//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
void destruir_tabla_pagina(tabla_pagina_t* tabla, int nivel_actual) {
    if (!tabla) return;

    if (tabla->es_entrada_final) {
        if (tabla->marcos != NULL) {
            free(tabla->marcos);
            tabla->marcos = NULL;
        }
    } else {
        for (int i = 0; i < tabla->cantidad_entradas; i++) { ///AAAAAAA
            if (tabla->subtablas[i] != NULL) {
                destruir_tabla_pagina(tabla->subtablas[i], nivel_actual + 1);
            }
        }
        free(tabla->subtablas);
        tabla->subtablas = NULL;
    }

    free(tabla);
}

void destruir_proceso(proceso_t* proceso) {
    if (!proceso) return;
    destruir_pseudocodigo(proceso->pseudocodigo);
    destruir_tabla_pagina(proceso->tabla_raiz, 0);
    free(proceso);
}

// LOGICA PSEUDUCODIGO DEL PROCESO
pseudocodigo_t* cargar_pseudocodigo(char* nombre_archivo) {
    log_trace(memoria_logger, "Cargando pseudocódigo desde: %s", nombre_archivo);
    char* path = string_duplicate(PATH_INSTRUCCIONES);
    log_trace(memoria_logger, "Ruta base: %s", path); 
    string_append(&path, nombre_archivo);
    log_trace(memoria_logger, "Ruta completa: %s", path); 

    FILE* archivo = fopen(path, "r");
    if (!archivo) {
        log_error(memoria_logger, "No se pudo abrir el archivo de pseudocódigo: %s", path);
        free(path);
        return NULL;
    }

    log_trace(memoria_logger, "Cargando pseudocódigo desde: %s", path);
    pseudocodigo_t* pc = malloc(sizeof(pseudocodigo_t));
    pc->instrucciones = NULL;
    pc->cantidad = 0;
    pc->proxima = 0;

    char* linea = NULL;
    size_t len = 0;
    while (getline(&linea, &len, archivo) != -1) {
        log_trace(memoria_logger, "Instrucción cargada: '%s'", linea); // <-- LOG DE INSTRUCCIÓN
        pc->instrucciones = realloc(pc->instrucciones, sizeof(char*) * (pc->cantidad + 1));
        pc->instrucciones[pc->cantidad] = strdup(linea);
        pc->cantidad++;
    }
    free(linea);
    fclose(archivo);
    free(path);
    return pc;
}

// funcion para liberar la memoria del pseudocodigo
const char* obtener_proxima_instruccion(pseudocodigo_t* pc) {
    if (pc->proxima >= pc->cantidad) return NULL;
    return pc->instrucciones[pc->proxima++];
}


proceso_t* crear_estructura_proceso(uint32_t pid, char* path, int cant_paginas) {
    proceso_t* nuevo = malloc(sizeof(proceso_t));
    nuevo->pid = pid;
    nuevo->pseudocodigo = cargar_pseudocodigo(path);
    nuevo->tabla_raiz = NULL;
    nuevo->accesos_tablas_paginas = 0;
    nuevo->instrucciones_solicitadas = 0;
    nuevo->bajadas_a_swap = 0;
    nuevo->subidas_a_memoria = 0;
    nuevo->lecturas_memoria = 0;
    nuevo->escrituras_memoria = 0;
    nuevo->cantidad_paginas = cant_paginas; // Inicializar cantidad de páginas
    return nuevo;
}