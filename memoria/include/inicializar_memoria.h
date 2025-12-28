#ifndef INICIALIZAR_MEMORIA_H_
#define INICIALIZAR_MEMORIA_H_

#include "gestor_memoria.h"
//#include "memoria_proceso.h"

void inicializar_memoria();
void inicializar_logger_memoria();
void inicializar_config_memoria();
void loggear_config_memoria();
void inicializar_lista_procesos();
void inicializar_frames_maximos(); //17/6

// Para crear el espacio de usuario
void inicializar_espacio_de_usuario();

void inicializar_dump_path();

void inicializar_semaforos();


void inicializar_swap();

#endif