//Incluimos el head
#ifndef CONTROL_HUMEDAD_H
#define CONTROL_HUMEDAD_H

//Librerias
#include <stdio.h>
#include <stdint.h>
#include <driver/adc.h>

/**
 * @brief Parametros de control de humedad
 */
typedef struct{
    uint8_t humedad;
    uint8_t nivel_tanque;
    }param_cont_humedad;


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


