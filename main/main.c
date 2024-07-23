/*Incluimos las librerias necesarias para el funcionamiento de nuestro script*/

#include <stdio.h>             //Sirve para la entrada y salida de datos
#include <stdint.h>            //Sirve para definir tipos de datos
#include "freertos/FreeRTOS.h" //Libreria para configurar el sistema operativo FreeRTOS
#include "freertos/task.h"     //Libreria para configurar las tareas

#include "esp_log.h" //Libreria para configurar los logs

#include "Control_temperatura.h" //Libreria para configurar el control de temperatura

#include "Blynk_MQTT_Connect.h" //Libreria para configurar la conexion con el servidor Blynk

// Definimos algunos parametros que se utilizaran en este script

/*----------------------------------LOGS y TAREAS----------------------------------------*/
static const char *MAIN_TAG = "Main";
#define STACK_SIZE 2048 // Definimos el tamaño de la pila de las tareas en bytes

/*--------------------------------CONTROL TEMPERATURA-----------------------------------------------*/
// Definimos el tiempo de muestreo del sensor DS18B20
#define PERIODO_MUESTREO (1000)
// Pin de conexion de One Wire del sensor DS18B20
#define GPIO_DS18B20_0 (GPIO_NUM_13)
// Definimos la instruccion para encender el climatizador
#define CALOR   1
#define FRIO    0

/*--------------------------------CONTROL HUMEDAD-----------------------------------------------*/

// Declaramos las funciones que se utilizaran en este script
esp_err_t crea_tareas(void);

void app_main(void)
{
    // Nos conectamos a internet y a Blynk.Cloud
    conecta_servidor();
    // Enviamos a Blynk la solicitud para que sincronice los datos con las variables del sistema
    envia_Blynk("sync", NULL);
    // Creamos las tareas del sistema
    crea_tareas();
}

void vTaskMideTemperatura(void *pvParameters)
{
    // Reservamos un espacio de memoria e incializamos el puntero parametros_temperatura, que apunta al primer elemento de ese espacio
    param_cont_temperatura *parametros_temperatura = (param_cont_temperatura *)malloc(sizeof(param_cont_temperatura));
    // Enviamos al script de Blynk el puntero a la estructura de parametros de temperatura
    apunta_parametros_temperatura(parametros_temperatura);

    // Seteamos la resistencia de pull up interna.
    gpio_set_pull_mode(GPIO_DS18B20_0, GPIO_PULLUP_ONLY);
    // Aplicamos un delay de 2 segundos para que el sensor se estabilice
    vTaskDelay(pdMS_TO_TICKS(2000));
    // Comprobamos la conexion con el sensor
    comprueba_sensor_temperatura();
    // Iniciamos el sensor
    inicia_sensor_temperatura();
    // Definimos un flag para indicar cuando el bloque climatizador se encuentre encendido o apagado
    bool estado_climatizador = false;
    // Definimos un flag para indicar cuando el ventilador se encuentre encendido o apagado
    bool estado_ventilador = false;
    // Declaramos el taskhandle del ventilador
    TaskHandle_t xVentiladorHandle;
    while (1)
    {
        // Medimos la temperatura
        mide_temperatura(parametros_temperatura);
        // Lo enviamos a Blynk
        char temperaturaC[10];
        sprintf(temperaturaC, "%.1f", parametros_temperatura->temperatura);
        envia_Blynk("temp", temperaturaC);
        printf("Temperatura invernadero: %.1f\n", parametros_temperatura->temperatura);
        printf("Temperatura ideal: %d\n", parametros_temperatura->temperatura_ideal);
        printf("Intervalo superior: %d\n", parametros_temperatura->limite_sup_temp);
        printf("Intervalo inferior: %d\n", parametros_temperatura->limite_inf_temp);
        //Calculamos diferencia de temperatura
        parametros_temperatura->diferencia_temp = abs(parametros_temperatura->temperatura - parametros_temperatura->limite_sup_temp);
        // Revisamos que la temperatura este dentro del intervalo deseado
        if (parametros_temperatura->temperatura > parametros_temperatura->limite_sup_temp)
        {
            // Si la temperatura es mayor a la deseada, encendemos el climatizador en calor
            if (!estado_climatizador)
            {
                // Seteamos el flag en true
                estado_climatizador = true;
                // Encendemos el climatizador
                enciende_climatizador(CALOR);
            }
            // Creamos una tarea con freeRTOS para el control del ventilador por PWM
            // Mas adelante cuando identifiquemos que la temperatura vuelve a la normalidad, eliminamos la tarea
            
            if (!estado_ventilador)
            {
                xTaskCreate(controla_ventilador, "controla_ventilador", STACK_SIZE, NULL, 5, &xVentiladorHandle);
                estado_ventilador = true;
            }
        }
        else if (parametros_temperatura->temperatura < parametros_temperatura->limite_inf_temp)
        {

            // Si la temperatura es menor a la deseada, encendemos el climatizador en frio
            if (!estado_climatizador)
            {
                // Seteamos el flag en true
                estado_climatizador = true;
                // Encendemos el climatizador
                enciende_climatizador(FRIO);
            }
            // Creamos una tarea con freeRTOS para el control del ventilador por PWM
            // Mas adelante cuando identifiquemos que la temperatura vuelve a la normalidad, eliminamos la tarea
            if (!estado_ventilador)
            {
                xTaskCreate(controla_ventilador, "controla_ventilador", STACK_SIZE, NULL, 5, &xVentiladorHandle);
                estado_ventilador = true;
            }
        }
        else
        {
            // Si la temperatura esta dentro del intervalo deseado, apagamos el climatizador
            if (estado_climatizador)
            {
                // Seteamos el flag en false
                estado_climatizador = false;
                // Apagamos el climatizador
                apagar_climatizador();
            }
            if (estado_ventilador)
            {
                // Seteamos el flag en false
                estado_ventilador = false;
                // Terminamos la tarea de control del ventilador
                vTaskDelete(xVentiladorHandle);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(PERIODO_MUESTREO));
    }
}

esp_err_t crea_tareas(void)
{
    // Con esta funcion creamos las tareas que se ejecutaran en el sistema, aparte de la tarea principal

    // Creamos la tarea de sensor de temperatura. PRIORIDAD: 3 (Por ahora)
    static uint8_t parametrosTempTask; // Si quisieramos pasar algun parametro a la tarea
    TaskHandle_t xTempHandle;          // Esta variable apunta a la tarea creada. Nos sirve para modificar la tarea (pausar, eliminar, etc)
    xTaskCreate(&vTaskMideTemperatura, "mide_temperatura", STACK_SIZE, &parametrosTempTask, 5, &xTempHandle);

    // Mas tareas....

    return ESP_OK;
}
