#ifndef CPU_KERNEL_INTERRUPT
#define CPU_KERNEL_INTERRUPT

#include "gestor_cpu.h"

extern bool interrupcion_pendiente;


void atender_cpu_kernel_interrupt();


#endif // CPU_KERNEL_INTERRUPT