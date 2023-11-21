 // FileName:        HVAC_IO.c
 // Dependencies:    HVAC.h
 // Processor:       MSP432
 // Board:           MSP432P401R
 // Program version: CCS V8.3 TI
 // Company:         Texas Instruments
 // Description:     Funciones de control de HW a trav�s de estados y objetos.
 // Authors:         Jos� Luis Chac�n M. y Jes�s Alejandro Navarro Acosta.
 // Updated:         11/2018

#include "HVAC.h"

/* Definici�n de botones. */
#define TEMP_PLUS   BSP_BUTTON1     /* Botones de suma y resta al valor deseado, funcionan con interrupciones. */
#define TEMP_MINUS  BSP_BUTTON2

#define FAN_ON      BSP_BUTTON3     /* Botones para identificaci�n del estado del sistema. */
#define FAN_AUTO    BSP_BUTTON4
#define SYSTEM_COOL BSP_BUTTON5
#define SYSTEM_OFF  BSP_BUTTON6
#define SYSTEM_HEAT BSP_BUTTON7

/* Definici�n de leds. */
#define FAN_LED     BSP_LED1        /* Leds para denotar el estado de las salidas. */
#define HEAT_LED    BSP_LED2
#define HBeat_LED   BSP_LED3
#define COOL_LED    BSP_LED4

/* Variables sobre las cuales se maneja el sistema. */

float TemperaturaActual = 20;  // Temperatura.
float SetPoint = 25.0;         // V. Deseado.

char state[MAX_MSG_SIZE];      // Cadena a imprimir.

bool toggle = 0;               // Toggle para el heartbeat.
_mqx_int delay;                // Delay aplicado al heartbeat.
bool event = FALSE;

bool FAN_LED_State = 0;                                     // Estado led_FAN.
const char* SysSTR[] = {"Cool","Off","Heat","Only Fan"};    // Control de los estados.

/* Archivos sobre los cuales se escribe toda la informaci�n */
FILE _PTR_ input_port = NULL, _PTR_ output_port = NULL;                  // Entradas y salidas.
FILE _PTR_ fd_adc = NULL, _PTR_ fd_ch_T = NULL, _PTR_ fd_ch_H = NULL;    // ADC: ch_T -> Temperature, ch_H -> Pot.
FILE _PTR_ fd_uart = NULL;                                               // Comunicaci�n serial as�ncrona.

// Estructuras iniciales.

const ADC_INIT_STRUCT adc_init =
{
    ADC_RESOLUTION_DEFAULT,                                                     // Resoluci�n.
    ADC_CLKDiv8                                                                 // Divisi�n de reloj.
};

const ADC_INIT_CHANNEL_STRUCT adc_ch_param =
{
    TEMPERATURE_ANALOG_PIN,                                                      // Fuente de lectura, 'ANx'.
    ADC_CHANNEL_MEASURE_LOOP | ADC_CHANNEL_START_NOW | ADC_INTERNAL_TEMPERATURE, // Banderas de inicializaci�n (temperatura)
    50000,                                                                       // Periodo en uS, base 1000.
    ADC_TRIGGER_1                                                                // Trigger l�gico que puede activar este canal.
};

const ADC_INIT_CHANNEL_STRUCT adc_ch_param2 =
{
    AN1,                                                                         // Fuente de lectura, 'ANx'.
    ADC_CHANNEL_MEASURE_LOOP,                                                    // Banderas de inicializaci�n (pot).
    50000,                                                                       // Periodo en uS, base 1000.
    ADC_TRIGGER_2                                                                // Trigger l�gico que puede activar este canal.
};

static uint_32 data[] =                                                          // Formato de las entradas.
{                                                                                // Se prefiri� un solo formato.
     TEMP_PLUS,
     TEMP_MINUS,
     FAN_ON,
     FAN_AUTO,
     SYSTEM_COOL,
     SYSTEM_OFF,
     SYSTEM_HEAT,

     GPIO_LIST_END
};

static const uint_32 fan[] =                                                    // Formato de los leds, uno por uno.
{
     FAN_LED,
     GPIO_LIST_END
};

static const uint_32 heat[] =                                                   // Formato de los leds, uno por uno.
{
     HEAT_LED,
     GPIO_LIST_END
};

const uint_32 hbeat[] =                                                         // Formato de los leds, uno por uno.
{
     HBeat_LED,
     GPIO_LIST_END
};

static const uint_32 cool[] =                                                   // Formato de los leds, uno por uno.
{
     COOL_LED,
     GPIO_LIST_END
};


/**********************************************************************************
 * Function: INT_SWI
 * Preconditions: Interrupci�n habilitada, registrada e inicializaci�n de m�dulos.
 * Overview: Funci�n que es llamada cuando se genera
 *           la interrupci�n del bot�n SW1 o SW2.
 * Input: None.
 * Output: None.
 **********************************************************************************/
void INT_SWI(void)
{
    Int_clear_gpio_flags(input_port);

    ioctl(input_port, GPIO_IOCTL_READ, &data);

    if((data[0] & GPIO_PIN_STATUS) == 0)        // Lectura de los pines, �ndice cero es TEMP_PLUS.
        HVAC_SetPointUp();

    else if((data[1] & GPIO_PIN_STATUS) == 0)   // Lectura de los pines, �ndice uno es TEMP_MINUS.
        HVAC_SetPointDown();

    return;
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_InicialiceIO
* Returned Value   : boolean; inicializaci�n correcta.
* Comments         :
*    Abre los archivos e inicializa las configuraciones deseadas de entrada y salida GPIO.
*
*END***********************************************************************************/
boolean HVAC_InicialiceIO(void)
{
    // Estructuras iniciales de entradas y salidas.
    const uint_32 output_set[] =
    {
         FAN_LED   | GPIO_PIN_STATUS_0,
         HEAT_LED  | GPIO_PIN_STATUS_0,
         HBeat_LED | GPIO_PIN_STATUS_0,
         COOL_LED  | GPIO_PIN_STATUS_0,
         GPIO_LIST_END
    };

    const uint_32 input_set[] =
    {
        TEMP_PLUS,
        TEMP_MINUS,
        FAN_ON,
        FAN_AUTO,
        SYSTEM_COOL,
        SYSTEM_OFF,
        SYSTEM_HEAT,

        GPIO_LIST_END
    };

    // Iniciando GPIO.
    ////////////////////////////////////////////////////////////////////

    output_port =  fopen_f("gpio:write", (char_ptr) &output_set);
    input_port =   fopen_f("gpio:read", (char_ptr) &input_set);

    if (output_port) { ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, NULL); }   // Inicialmente salidas apagadas.
    ioctl (input_port, GPIO_IOCTL_SET_IRQ_FUNCTION, INT_SWI);               // Declarando interrupci�n.

    return (input_port != NULL) && (output_port != NULL);
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_InicialiceADC
* Returned Value   : boolean; inicializaci�n correcta.
* Comments         :
*    Abre los archivos e inicializa las configuraciones deseadas para
*    el m�dulo general ADC y dos de sus canales; uno para la temperatura, otro para
*    el heartbeat.
*
*END***********************************************************************************/

boolean HVAC_InicialiceADC (void)
{
    // Iniciando ADC y canales.
    ////////////////////////////////////////////////////////////////////

    fd_adc   = fopen_f("adc:",  (const char*) &adc_init);               // M�dulo.
    fd_ch_T =  fopen_f("adc:1", (const char*) &adc_ch_param);           // Canal uno, arranca al instante.
    fd_ch_H =  fopen_f("adc:2", (const char*) &adc_ch_param2);          // Canal dos.

    return (fd_adc != NULL) && (fd_ch_T != NULL) && (fd_ch_H != NULL);  // Valida que se crearon los archivos.
}


/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_InicialiceUART
* Returned Value   : boolean; inicializaci�n correcta.
* Comments         :
*    Abre los archivos e inicializa las configuraciones deseadas para
*    configurar el modulo UART (comunicaci�n as�ncrona).
*
*END***********************************************************************************/
boolean HVAC_InicialiceUART (void)
{
    // Estructura inicial de comunicaci�n serie.
    const UART_INIT_STRUCT uart_init =
    {
        /* Selected port */        1,
        /* Selected pins */        {1,2},
        /* Clk source */           SM_CLK,
        /* Baud rate */            BR_115200,

        /* Usa paridad */          NO_PARITY,
        /* Bits protocolo  */      EIGHT_BITS,
        /* Sobremuestreo */        OVERSAMPLE,
        /* Bits de stop */         ONE_STOP_BIT,
        /* Direccion TX */         LSB_FIRST,

        /* Int char's \b */        NO_INTERRUPTION,
        /* Int char's err�neos */  NO_INTERRUPTION
    };

    // Inicializaci�n de archivo.
    fd_uart = fopen_f("uart:", (const char*) &uart_init);

    return (fd_uart != NULL); // Valida que se crearon los archivos.
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_ActualizarEntradas
* Returned Value   : None.
* Comments         :
*    Actualiza los variables indicadores de las entradas sobre las cuales surgir�n
*    las salidas.
*
*END***********************************************************************************/
void HVAC_ActualizarEntradas(void)
{
    static bool ultimos_estados[] = {FALSE, FALSE, FALSE, FALSE, FALSE};        //PARA CONTROL DE EVENTOS

    ioctl(fd_ch_T, IOCTL_ADC_READ_TEMPERATURE, (pointer) &TemperaturaActual);   // Actualiza valor de temperatura.
    ioctl(input_port, GPIO_IOCTL_READ, &data);

    if((data[2] & GPIO_PIN_STATUS) != NORMAL_STATE_EXTRA_BUTTONS)        // Cambia el valor de las entradas FAN.
    {
        EstadoEntradas.FanState = On;
        EstadoEntradas.SystemState = FanOnly;
        if(ultimos_estados[0] == FALSE)                 //SI ULTIMO ESTADO FANSTATE NO ES ON
            event = TRUE;                               //BANDERA OCURRIO EVENTO

        ultimos_estados[0] = TRUE;                      //FANSTATE ON
        ultimos_estados[1] = FALSE;                     //FANSTATE AUTO
    }

    else if((data[3] & GPIO_PIN_STATUS) != NORMAL_STATE_EXTRA_BUTTONS)   // Cambia el valor de las entradas SYSTEM.
    {
        EstadoEntradas.FanState = Auto;                //PON fanstate en AUTO
        if(ultimos_estados[1] == FALSE)                //SI ULTIMO ESTADO FANSTATE NO ES AUTO
            event = TRUE;                              //BANDERA OCURRIO EVENTO

       ultimos_estados[0] = FALSE;                     //FANSTATE QUITAR ON
       ultimos_estados[1] = TRUE;                      //FANSTATE ULTIMO ESTADO PONER AUTO

        if((data[4] & GPIO_PIN_STATUS) != NORMAL_STATE_EXTRA_BUTTONS)   // Y as� sucesivamente para el resto de pines.
        {
            EstadoEntradas.SystemState = Cool;
            if(ultimos_estados[2] == FALSE)
                event = TRUE;
            ultimos_estados[2] = TRUE;
            ultimos_estados[3] = FALSE;
            ultimos_estados[4] = FALSE;
        }
        else if((data[5] & GPIO_PIN_STATUS) != NORMAL_STATE_EXTRA_BUTTONS)
        {
            EstadoEntradas.SystemState = Off;
            if(ultimos_estados[3] == FALSE)
                event = TRUE;
            ultimos_estados[2] = FALSE;
            ultimos_estados[3] = TRUE;
            ultimos_estados[4] = FALSE;
        }
        else if((data[6] & GPIO_PIN_STATUS) != NORMAL_STATE_EXTRA_BUTTONS)
        {
            EstadoEntradas.SystemState = Heat;
            if(ultimos_estados[4] == FALSE)
                event = TRUE;
            ultimos_estados[2] = FALSE;
            ultimos_estados[3] = FALSE;
            ultimos_estados[4] = TRUE;
        }
        else
        {
            EstadoEntradas.SystemState = Off;       //SI NO SE PRESIONO CUALQUIERA DELOS BOTONES PASADOS PON SystemState en OFF
            ultimos_estados[2] = FALSE;
            ultimos_estados[3] = FALSE;
            ultimos_estados[4] = FALSE;
        }
    }
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_ActualizarSalidas
* Returned Value   : None.
* Comments         :
*    Decide a partir de las entradas actualizadas las salidas principales,
*    y en ciertos casos, en base a una cuesti�n de temperatura, la salida del 'fan'.
*
*END***********************************************************************************/
void HVAC_ActualizarSalidas(void)
{
    // Cambia el valor de las salidas de acuerdo a entradas.

    if(EstadoEntradas.FanState == On)                               // Para FAN on.
    {
        FAN_LED_State = 1;
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG1, &fan);
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &heat);
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &cool);
    }

    else if(EstadoEntradas.FanState == Auto)                        // Para FAN automatico.
    {
        switch(EstadoEntradas.SystemState)
        {
        case Off:   ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &fan);
                    ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &heat);
                    ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &cool);
                    FAN_LED_State = 0;
                    //******************************************************
                    Task_setPri(((pthread_Obj*)salidas_thread)->task, -1);
                    Task_setPri(((pthread_Obj*)heartbeat_thread)->task, -1);
                    print("Salida y HeatBeat apagados\r\n");
                    //******************************************************
                    break;
        case Heat:  HVAC_Heat(); break;
        case Cool:  HVAC_Cool(); break;
        }
    }
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_Heat
* Returned Value   : None.
* Comments         :
*    Decide a partir de la temperatura actual y la deseada, si se debe activar el fan.
*    (La temperatura deseada debe ser mayor a la actual). El estado del fan debe estar
*    en 'auto' y este modo debe estar activado para entrar a la funci�n.
*
*END***********************************************************************************/
void HVAC_Heat(void)
{
    ioctl(output_port, GPIO_IOCTL_WRITE_LOG1, &heat);
    ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &cool);

    if(TemperaturaActual < SetPoint)                    // El fan se debe encender si se quiere una temp. m�s alta.
    {
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG1, &fan);
        FAN_LED_State = 1;
    }
    else
    {
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &fan);
        FAN_LED_State = 0;
    }
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_Cool
* Returned Value   : None.
* Comments         :
*    Decide a partir de la temperatura actual y la deseada, si se debe activar el fan.
*    (La temperatura deseada debe ser menor a la actual). El estado del fan debe estar
*    en 'auto' y este modo debe estar activado para entrar a la funci�n.
*
*END***********************************************************************************/
void HVAC_Cool(void)
{
    ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &heat);
    ioctl(output_port, GPIO_IOCTL_WRITE_LOG1, &cool);

    if(TemperaturaActual > SetPoint)                        // El fan se debe encender si se quiere una temp. m�s baja.
    {
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG1, &fan);
        FAN_LED_State = 1;
    }
    else
    {
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &fan);
        FAN_LED_State = 0;
    }
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_Heartbeat
* Returned Value   : None.
* Comments         :
*    Funci�n que prende y apaga una salida para notificar que el sistema est� activo.
*    El periodo en que se hace esto depende de una entrada del ADC en esta funci�n.
*
*END***********************************************************************************/
void HVAC_Heartbeat(void)               // Funci�n de 'alive' del sistema.
{
   _mqx_int val;
   boolean flag = 0;
   static boolean bandera_inicial = 0;

   if(bandera_inicial == 0)
   {
       // Entrando por primera vez, empieza a correr el canal de heartbeat.
       ioctl (fd_ch_H, IOCTL_ADC_RUN_CHANNEL, NULL);
       bandera_inicial = 1;
   }

   // Valor se guarda en val, flag nos dice si fue exitoso.
   flag =  (fd_adc && fread_f(fd_ch_H, &val, sizeof(val))) ? 1 : 0;

   if(flag != TRUE)
   {
       printf("Error al leer archivo. Cierre del programa\r\n");
       exit(1);
   }

    delay = 15000 + (100 * val / 4);            // Lectura del ADC por medio de la funci�n.
    //Nota: delay no puede ser mayor a 1,000,000 ya que luego se generan problemas en usleep.

    if(toggle)
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG1, &hbeat);
    else
        ioctl(output_port, GPIO_IOCTL_WRITE_LOG0, &hbeat);

    toggle ^= 1;                             // Toggle.

    usleep(delay);                           // Delay marcado por el heart_beat.
    return;
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_PrintState
* Returned Value   : None.
* Comments         :
*    Imprime via UART la situaci�n actual del sistema en t�rminos de temperaturas
*    actual y deseada, estado del abanico, del sistema y estado de las entradas.
*    Imprime cada cierto n�mero de iteraciones y justo despues de recibir un cambio
*    en las entradas, produci�ndose un inicio en las iteraciones.
*END***********************************************************************************/
void HVAC_PrintState(void)
{
    static uint_32 delay = DELAY;
    delay -= DELAY;

    if(delay <= 0 || event == TRUE)
    {
        event = FALSE;
        delay = SEC;

        sprintf(state,"Fan: %s, System: %s, SetPoint: %0.2f\n\r",
                    EstadoEntradas.FanState == On? "On":"Auto",
                    SysSTR[EstadoEntradas.SystemState],
                    SetPoint);
        print(state);

        sprintf(state,"Temperatura Actual: %0.2f�C %0.2f�F  Fan: %s\n\r\n\r",
                    TemperaturaActual,
                    ((TemperaturaActual*9.0/5.0) + 32),
                    FAN_LED_State?"On":"Off");
        print(state);
    }
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_SetPointUp
* Returned Value   : None.
* Comments         :
*    Sube el valor deseado (set point). Llamado por interrupci�n a causa del SW1.
*
*END***********************************************************************************/
void HVAC_SetPointUp(void)
{
    SetPoint += 0.5;
    event = TRUE;
}

/*FUNCTION******************************************************************************
*
* Function Name    : HVAC_SetPointDown
* Returned Value   : None.
* Comments         :
*    Baja el valor deseado (set point). Llamado por interrupci�n a causa del SW2.
*
*END***********************************************************************************/
void HVAC_SetPointDown(void)
{
    SetPoint -= 0.5;
    event = TRUE;
}

