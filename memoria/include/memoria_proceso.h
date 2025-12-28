#ifndef MEMORIA_PROCESO_H
#define MEMORIA_PROCESO_H

#include "gestor_memoria.h"
//#include "memoria_configuracion.h" // O donde esté tabla_pagina_t

extern t_list* lista_procesos;

// Funciones de gestión de procesos
void inicializar_lista_procesos();
proceso_t* crear_proceso(uint32_t pid); // esto habria que borrarlo pero por las dudas lo dejo
proceso_t* buscar_proceso_por_pid(uint32_t pid);
void destruir_proceso(proceso_t* proceso);

proceso_t* crear_estructura_proceso(uint32_t pid, char* path, int cantidad_paginas);

void destruir_pseudocodigo(pseudocodigo_t* pc);
void destruir_tabla_pagina(tabla_pagina_t* tabla, int nivel_actual);

#endif