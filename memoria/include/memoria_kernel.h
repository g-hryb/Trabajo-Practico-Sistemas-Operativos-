#ifndef MEMORIA_KERNEL_H
#define MEMORIA_KERNEL_H

#include "gestor_memoria.h"
//#include "memoria_proceso.h"

void atender_memoria_kernel();
void enviar_mensaje_ok_hay_espacio(int fd_kernel);
/*
typedef enum{
    PEDIDO_ESPACIO_LIBRE_KM = 2
} cod_op_kernel_2;
*/

#endif // MEMORIA_KERNEL_H