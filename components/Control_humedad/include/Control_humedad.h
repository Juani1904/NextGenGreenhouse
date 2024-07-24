//Incluimos el head
#ifndef CONTROL_HUMEDAD_H
#define CONTROL_HUMEDAD_H

//Librerias
#include <stdio.h>
#include <stdint.h>
#include <driver/adc.h> //Libreria para configurar el ADC y tomar medidas analogicas
#include "freertos/FreeRTOS.h" //Libreria para configurar el sistema operativo FreeRTOS
#include "freertos/task.h" //Libreria para configurar las tareas
#include "esp_log.h" //Libreria para configurar los logs

/**
 * @brief Parametros de control de humedad
 */
typedef struct{
    uint8_t humedad;
    uint8_t nivel_tanque;
    }param_cont_humedad;


/**
 * @brief Función para inicializar los canales del ADC
 *
 * @param parametros Estructura con los parametros de control de humedad

 */
void adc_pins_init(void);



/**
 * @brief Funcion para tomar las medidas de humedad con el sensor FC-28
 *
 * @param parametros Estructura con los parametros de control de humedad

 */
void mide_humedad(param_cont_humedad *parametros);

/**
 * @brief Funcion para encender la bomba de agua
 * 
 */
void enciende_bomba(void);

/**
 * @brief Funcion para apagar la bomba de agua
 * 
 */
void apaga_bomba(void);

/**
 * @brief Funcion para medir el nivel del tanque con el sensor de nivel
 * @param parametros Estructura con los parametros de control de humedad
 */
void mide_nivel_tanque(param_cont_humedad *parametros);



#endif


