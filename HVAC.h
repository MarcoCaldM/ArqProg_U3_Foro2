 // FileName:        HVAC.h
 // Dependencies:    None
 // Processor:       MSP432
 // Board:           MSP432P401R
 // Program version: CCS V8.3 TI
 // Company:         Texas Instruments
 // Description:     Incluye librer�as, define ciertas macros, as� como llevar un control de versiones.
 // Authors:         Jos� Luis Chac�n M. y Jes�s Alejandro Navarro Acosta.
 // Updated:         11/2018

#ifndef _hvac_h_
#define _hvac_h_

#pragma once

#define __MSP432P401R__
#define  __SYSTEM_CLOCK    48000000 // Frecuencias funcionales recomendadas: 12, 24 y 48 Mhz.

/* Archivos de cabecera importantes. */
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Archivos de cabecera POSIX. */
#include <pthread.h>
#include <semaphore.h>
#include <ti/posix/tirtos/_pthread.h>
#include <ti/sysbios/knl/Task.h>

/* Archivos de cabecera RTOS. */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Event.h>

/* Archivos de cabecera de drivers de Objetos. */
#include "Drivers_obj/BSP.h"

// Definiciones del estado 'normal' de los botones externos a la tarjeta (solo hay dos botones).
#define GND 0
#define VCC 1
#define NORMAL_STATE_EXTRA_BUTTONS GND  // Aqui se coloca GND o VCC.

// Definiciones del sistema.
#define MAX_MSG_SIZE 64
#define MAX_ADC_VALUE 16383
#define MAIN_UART (uint32_t)(EUSCI_A0)

// Definiciones del RTOS.
#define THREADSTACKSIZE1 1500
#define THREADSTACKSIZE2 1500
#define THREADSTACKSIZE3 1500

// Definici�n de delay para threads de entradas y salidas.
#define DELAY 4000

/* Enumeradores para la descripci�n del sistema. */

enum FAN        // Para el fan (abanico).
{
    On,
    Auto,
};

enum SYSTEM     // Para el sistema cuando FAN est� en auto (cool, off y heat, o no considerar ninguno y usar fan only).
{
    Cool,
    Off,
    Heat,
    FanOnly,
};

struct EstadoEntradas       // Estructura que incluye el estado de las entradas.
{
    uint8_t  SystemState;
    uint8_t     FanState;
} EstadoEntradas;

/* Funciones. */

/* Funci�n de interrupci�n para botones de setpoint. */
extern void INT_SWI(void);

/* Funciones de inicializaci�n. */
extern boolean HVAC_InicialiceIO   (void);
extern boolean HVAC_InicialiceADC  (void);
extern boolean HVAC_InicialiceUART (void);

/* Funciones principales. */
extern void HVAC_ActualizarEntradas(void);
extern void HVAC_ActualizarSalidas(void);
extern void HVAC_Heartbeat(void);
extern void HVAC_PrintState(void);

/* Funciones para los estados Heat y Cool. */
extern void HVAC_Heat(void);
extern void HVAC_Cool(void);

/* Funciones para incrementar o disminuir setpoint. */
extern void HVAC_SetPointUp(void);
extern void HVAC_SetPointDown(void);

/* Funci�n especial que imprime el mensaje asegurando que no habr� interrupciones y por ende,
 * un funcionamiento no �ptimo.                                                             */
extern void print(char* message);

pthread_t           entradas_thread, salidas_thread, heartbeat_thread;

#endif
