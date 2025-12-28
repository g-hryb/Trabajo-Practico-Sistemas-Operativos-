#ifndef MANEJO_MEMORIA_H_
#define MANEJO_MEMORIA_H_
#include "gestor_memoria.h"


bool hay_espacio_libre(int tamanio);
void enviar_mensaje_ok_a_kernel(int fd_kernel, int cod_op);
void enviar_mensaje_error_a_kernel(int fd_kernel, int cod_op);
tabla_pagina_t* crear_tablas_proceso(int nivel_actual, int paginas_restantes);
void inicializar_espacio_de_usuario();
void inicializar_frames_maximos(); //17/6
int asignar_marco_libre();
bool asignar_marcos_a_tabla(tabla_pagina_t* tabla, int nivel_actual, int cantidad_marcos);
//void inicializar_


int espacio_libre_en_memoria();

#endif