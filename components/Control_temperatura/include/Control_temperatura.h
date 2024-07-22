#ifndef CONTROL_TEMPERATURA_H
#define CONTROL_TEMPERATURA_H
//-------------------------------------Control temperatura-------------------------------------
//Componente donde se almacenan las funciones de sensado de temperatura, acción de ventiladores y de bloque climatizador
#include <stdint.h>
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

/**
 * @brief Parametros de control de temperatura
 */
typedef struct{
    float temperatura;
    uint8_t temperatura_ideal;
    uint8_t limite_inf_temp;
    uint8_t limite_sup_temp;
    uint8_t ventilador;
    uint8_t climatizador;}param_cont_temperatura;

/**
 * @brief Funcion para comprobar la conexion con el sensor DS18B20
 *
 */
void comprueba_sensor_temperatura(void);

/**
 * @brief Funcion para inicializar el sensor DS18B20
 * @note Esta funcion reserva la memoria dinamica para el dispositivo DS18B20,setea la resolución y decide si se usará CRC o no
 */
void inicia_sensor_temperatura(void);

/**
 * @brief Funcion para medir la temperatura con sensor DS18B20
 * @param parametros Estructura con los parametros de control de temperatura
 */
void mide_temperatura(param_cont_temperatura *parametros);

/**
 * @brief Funcion para encender el bloque climatizador
 * @param calor Indica si ingesa calor o frio. Si es true, ingresa calor, si es false, egresa calor (refrigeracion)
 */
void enciende_climatizador(bool calor);

/**
 * @brief Funcion para controlar el ventilador por PWM
 * @param diferencia_temp Diferencia absoluta entre la temperatura ideal y la temperatura actual
 */
void controla_ventilador(float diferencia_temp);

#endif