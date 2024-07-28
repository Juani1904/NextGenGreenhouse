#ifndef CONTROL_TEMPERATURA_H
#define CONTROL_TEMPERATURA_H
//-------------------------------------Control temperatura-------------------------------------
//Componente donde se almacenan las funciones de sensado de temperatura, acción de ventiladores y de bloque climatizador
#include <stdint.h>
#include "owb.h" //Liberia para configurar OneWire
#include "owb_rmt.h" //Libreria para configurar OneWire con RMT
#include "ds18b20.h" //Libreria para configurar el sensor DS18B20
#include "freertos/FreeRTOS.h" //Libreria para configurar el sistema operativo FreeRTOS
#include "esp_log.h" //Libreria para configurar los logs
#include <driver/ledc.h> //Libreria para configurar PWM

/**
 * @brief Parametros de control de temperatura
 */
typedef struct{
    float temperatura;
    uint8_t temperatura_ideal;
    uint8_t limite_inf_temp;
    uint8_t limite_sup_temp;
    uint8_t diferencia_temp;
    }param_cont_temperatura;

/**
 * @brief Funcion para comprobar la conexion con el sensor DS18B20
 *
 */
void comprueba_sensor_temperatura(gpio_num_t pin_sensor);

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
void enciende_climatizador(bool calor,gpio_num_t pin_climatizador);

/**
 * @brief Funcion para apagar el bloque climatizador
 * 
 */
void apagar_climatizador(gpio_num_t pin_climatizador_calor, gpio_num_t pin_climatizador_frio);

/**
 * @brief Funcion para controlar el ventilador por PWM
 * @param diferencia_temp Diferencia absoluta entre la temperatura ideal y la temperatura actual
 */
TaskFunction_t controla_ventilador(param_cont_temperatura *parametros);

/**
 * @brief Funcion para apagar el ventilador
 * 
 */
void apaga_ventilador(void);

#endif