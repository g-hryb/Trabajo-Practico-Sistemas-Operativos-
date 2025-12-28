#ifndef MEMORY_DUMP_H
#define MEMORY_DUMP_H

#include "gestor_memoria.h"

bool crear_archivo_dump(proceso_t* proceso);
t_list* obtener_marcos_asignados_para_dump(proceso_t* proceso);
void recolectar_marcos_para_dump(tabla_pagina_t* tabla, int nivel_actual, t_list* lista_marcos, int cantidad_paginas);

#endif