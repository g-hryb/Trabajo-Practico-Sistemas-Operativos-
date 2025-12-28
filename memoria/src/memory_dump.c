#include "../include/memory_dump.h"

bool crear_archivo_dump(proceso_t* proceso) {

    char nombre_archivo[100];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[40];
    strftime(timestamp, sizeof(timestamp), "%d-%m-%Y_%H-%M-%S", tm_info);
    // Armá el path completo usando DUMP_PATH (de tu config)
    snprintf(nombre_archivo, sizeof(nombre_archivo), "%s%d-%s.dmp", DUMP_PATH, proceso->pid, timestamp);
    log_trace(memoria_logger, "Nombre de archivo dump: %s", nombre_archivo); //COMPLETAMENTE PROBADO HASTA ACA, DEVUELVE POR EJEMPLO:
    /// home/utnso/dump_files/3-14-07-2025_16-40-45.dmp anachi

    
    FILE* archivo_dump = fopen(nombre_archivo, "w+b");
    if (!archivo_dump) {
        log_error(memoria_logger, "No se pudo crear el archivo de dump: %s", nombre_archivo);
        return false;
    }
    log_trace(memoria_logger, "Archivo dump creado: %s", nombre_archivo);
    t_list* lista_marcos_dump = obtener_marcos_asignados_para_dump(proceso);
    for (int i = 0; i < list_size(lista_marcos_dump); i++) {
        int* marco_ptr = list_get(lista_marcos_dump, i);
        int marco = *marco_ptr;
        fwrite(espacio_usuario + (marco * TAM_PAGINA), TAM_PAGINA, 1, archivo_dump);
        free(marco_ptr);
    }
    list_destroy(lista_marcos_dump);
    fclose(archivo_dump);
    log_trace(memoria_logger, "Archivo dump creado exitosamente: %s", nombre_archivo);
    return true;
}
//son iguales que las que estan en memoria_swap, pero para el dump, lo unico que cambia es que no se libera el marco en la 
//tabla de paginas ni en el bitmap, porque no se esta borrando el proceso, solo se lo esta dumpeando
t_list* obtener_marcos_asignados_para_dump(proceso_t* proceso) {
    t_list* lista_marcos_dump = list_create();
    recolectar_marcos_para_dump(proceso->tabla_raiz, 0, lista_marcos_dump, proceso->cantidad_paginas);
    return lista_marcos_dump;
}

void recolectar_marcos_para_dump(tabla_pagina_t* tabla, int nivel_actual, t_list* lista_marcos, int paginas_restantes) {
    if (!tabla) return;
    if (tabla->es_entrada_final) {
        int entradas_final = paginas_restantes < ENTRADAS_POR_TABLA ? paginas_restantes : ENTRADAS_POR_TABLA;
        for (int i = 0; i < entradas_final; i++) {
            int marco = tabla->marcos[i];
            if (marco != -1) {
                int* marco_ptr = malloc(sizeof(int));
                *marco_ptr = marco;
                list_add(lista_marcos, marco_ptr);
            }
        }
    } else {
        int paginas_por_subtabla = pow(ENTRADAS_POR_TABLA, CANTIDAD_NIVELES - nivel_actual - 1);
        int subtablas_necesarias = (paginas_restantes <= paginas_por_subtabla) ? 1 : ceil((double)paginas_restantes / paginas_por_subtabla);
        int paginas_restantes_local = paginas_restantes;
        for (int i = 0; i < subtablas_necesarias && paginas_restantes_local > 0; i++) {
            int paginas_en_esta_subtabla = paginas_restantes_local < paginas_por_subtabla ? paginas_restantes_local : paginas_por_subtabla;
            recolectar_marcos_para_dump(tabla->subtablas[i], nivel_actual + 1, lista_marcos, paginas_en_esta_subtabla);
            paginas_restantes_local -= paginas_en_esta_subtabla;
        }
    }
}