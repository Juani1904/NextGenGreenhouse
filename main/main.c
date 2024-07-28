#include <stdio.h>               //Sirve para la entrada y salida de datos
#include <stdint.h>              //Sirve para definir tipos de datos
#include "freertos/FreeRTOS.h"   //Libreria para configurar el sistema operativo FreeRTOS
#include "freertos/task.h"       //Libreria para configurar las tareas
#include "esp_log.h"             //Libreria para configurar los logs
#include "Control_temperatura.h" //Libreria para configurar el control de temperatura
#include "Control_humedad.h"     //Libreria para configurar el control de humedad
#include "Blynk_MQTT_Connect.h"  //Libreria para configurar la conexion con el servidor Blynk

// Definimos algunos parametros que se utilizaran en este script

/*----------------------------------LOGS y TAREAS----------------------------------------*/
static const char *MAIN_TAG = "Main";
#define STACK_SIZE 2048 // Definimos el tamaño de la pila de las tareas en bytes
// Definimos una global Key para implementar un mutex
SemaphoreHandle_t GlobalKey = NULL;

/*--------------------------------CONTROL TEMPERATURA-----------------------------------------------*/
// Definimos el tiempo de muestreo del sensor DS18B20
#define PERIODO_MUESTREO_TEMPERATURA (1000)
// Pin de conexion de One Wire del sensor DS18B20
#define GPIO_TEMPERATURA (GPIO_NUM_13)
// Definimos la instruccion para encender el climatizador
#define CALOR (1)
#define FRIO (0)
//Definimos de conexion del climatizador
#define GPIO_CLIMATIZADOR_CALOR (GPIO_NUM_26)
//Definimos pin de conexion del climatizador
#define GPIO_CLIMATIZADOR_FRIO (GPIO_NUM_27)
/*--------------------------------CONTROL HUMEDAD-----------------------------------------------*/
#define PERIODO_MUESTREO_HUMEDAD (1000)
//Pin de conexion del sensor de humedad. GPIO 34 (Input only)
#define GPIO_HUMEDAD (ADC1_CHANNEL_6)
//Pin de conexion del sensor de nivel de agua. GPIO 35 (Input only)
#define GPIO_NIVEL_TANQUE (ADC1_CHANNEL_7)
//Pin de conexion de la bomba de agua. GPIO 32
#define GPIO_BOMBA (GPIO_NUM_32) 


// Declaramos las funciones que se utilizaran en este script
esp_err_t crea_tareas(void);

void app_main(void)
{
    // Nos conectamos a internet y a Blynk.Cloud
    conecta_servidor();
    // Creamos las tareas del sistema
    crea_tareas();
    // Creamos el mutex
    GlobalKey = xSemaphoreCreateMutex(); // Definimos el mutex
    // Enviamos a Blynk la solicitud para que sincronice los datos con las variables del sistema
    envia_Blynk("sync", NULL);
}

void vTaskControlTemperatura(void *pvParameters)
{
    // Reservamos un espacio de memoria e incializamos el puntero parametros_temperatura, que apunta al primer elemento de ese espacio
    param_cont_temperatura *parametros_temperatura = (param_cont_temperatura *)malloc(sizeof(param_cont_temperatura));
    // Enviamos al script de Blynk el puntero a la estructura de parametros de temperatura
    apunta_parametros_temperatura(parametros_temperatura);
    // Seteamos la resistencia de pull up interna.
    gpio_set_pull_mode(GPIO_TEMPERATURA, GPIO_PULLUP_ONLY);
    // Aplicamos un delay de 2 segundos para que el sensor se estabilice
    vTaskDelay(pdMS_TO_TICKS(2000));
    // Comprobamos la conexion con el sensor
    comprueba_sensor_temperatura(GPIO_TEMPERATURA);
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
        // Calculamos diferencia de temperatura ideal vs medida
        parametros_temperatura->diferencia_temp = abs(parametros_temperatura->temperatura - parametros_temperatura->temperatura_ideal);
        // Revisamos que la temperatura este dentro del intervalo deseado
        if (parametros_temperatura->temperatura > parametros_temperatura->limite_sup_temp)
        {
            // Enviamos mensaje de estado a Blynk
            if (xSemaphoreTake(GlobalKey, portMAX_DELAY) == pdTRUE)
            {
                envia_Blynk("mensaje_estado", "Climatizando");
                vTaskDelay(pdMS_TO_TICKS(2000));
                xSemaphoreGive(GlobalKey);
            }
            // Si la temperatura es mayor a la deseada, encendemos el climatizador en calor
            if (!estado_climatizador)
            {
                // Seteamos el flag en true
                estado_climatizador = true;
                // Encendemos el climatizador
                enciende_climatizador(FRIO,GPIO_CLIMATIZADOR_FRIO);
            }
            // Creamos una tarea con freeRTOS para el control del ventilador por PWM
            // Mas adelante cuando identifiquemos que la temperatura vuelve a la normalidad, eliminamos la tarea

            if (!estado_ventilador)
            {
                xTaskCreate(controla_ventilador, "controla_ventilador", STACK_SIZE, parametros_temperatura, 5, &xVentiladorHandle);
                estado_ventilador = true;
            }
        }
        else if (parametros_temperatura->temperatura < parametros_temperatura->limite_inf_temp)
        {
            // Enviamos mensaje de estado a Blynk
            if (xSemaphoreTake(GlobalKey, portMAX_DELAY) == pdTRUE)
            {
                envia_Blynk("mensaje_estado", "Climatizando");
                vTaskDelay(pdMS_TO_TICKS(2000));
                xSemaphoreGive(GlobalKey);
            }
            // Si la temperatura es menor a la deseada, encendemos el climatizador en frio
            if (!estado_climatizador)
            {
                // Seteamos el flag en true
                estado_climatizador = true;
                // Encendemos el climatizador
                enciende_climatizador(CALOR,GPIO_CLIMATIZADOR_CALOR);
            }
            // Creamos una tarea con freeRTOS para el control del ventilador por PWM
            // Mas adelante cuando identifiquemos que la temperatura vuelve a la normalidad, eliminamos la tarea
            if (!estado_ventilador)
            {
                xTaskCreate(controla_ventilador, "controla_ventilador", STACK_SIZE, parametros_temperatura, 5, &xVentiladorHandle);
                estado_ventilador = true;
            }
        }
        else
        {
            // Enviamos mensaje de estado a Blynk
            if (xSemaphoreTake(GlobalKey, portMAX_DELAY) == pdTRUE)
            {
                envia_Blynk("mensaje_estado", "Temperatura normal");
                vTaskDelay(pdMS_TO_TICKS(2000));
                xSemaphoreGive(GlobalKey);
            }
            // Si la temperatura esta dentro del intervalo deseado, apagamos el climatizador
            if (estado_climatizador)
            {
                // Seteamos el flag en false
                estado_climatizador = false;
                // Apagamos el climatizador
                apagar_climatizador(GPIO_CLIMATIZADOR_CALOR, GPIO_CLIMATIZADOR_FRIO);
            }
            if (estado_ventilador)
            {
                // Seteamos el flag en false
                estado_ventilador = false;
                // Terminamos la tarea de control del ventilador
                vTaskDelete(xVentiladorHandle);
                apaga_ventilador();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(PERIODO_MUESTREO_TEMPERATURA));
    }
}

void vTaskControlHumedad(void *pvParameters)
{
    // Reservamos un espacio de memoria e incializamos el puntero parametros_humedad, que apunta al primer elemento de ese espacio
    param_cont_humedad *parametros_humedad = (param_cont_humedad *)malloc(sizeof(param_cont_humedad));
    // Enviamos al script de Blynk el puntero a la estructura de parametros de humedad
    apunta_parametros_humedad(parametros_humedad);
    // Inicializamos los pines del ADC
    adc_pins_init(GPIO_HUMEDAD, GPIO_BOMBA);
    // Definimos un booleano de estado para la bomba de agua
    bool estado_bomba = false;
    while (1)
    {
        // Medimos la humedad
        mide_humedad(parametros_humedad,GPIO_HUMEDAD);
        // Lo enviamos a Blynk
        char humedadC[10];
        sprintf(humedadC, "%d", parametros_humedad->humedad);
        envia_Blynk("humedad", humedadC);
        printf("Humedad invernadero: %d\n", parametros_humedad->humedad);

        // Revisamos la humedad de la tierra, si es menor al 50% debemos accionar el sistema de riego
        if (parametros_humedad->humedad < 50)
        {
            // Revisamos el nivel de agua en el tanque, si es mayor al 60% debemos accionar la bomba de agua, si no mostramos un mensaje
            mide_nivel_tanque(parametros_humedad,GPIO_HUMEDAD);
            printf("Nivel de agua en el tanque: %d\n", parametros_humedad->nivel_tanque);
            if (parametros_humedad->nivel_tanque > 60)
            {
                // Enviamos mensaje de estado a Blynk
                if (xSemaphoreTake(GlobalKey, portMAX_DELAY) == pdTRUE)
                {
                    envia_Blynk("mensaje_estado", "Regando");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    xSemaphoreGive(GlobalKey);
                }
                // Encendemos la bomba de agua
                if (!estado_bomba)
                {
                    estado_bomba = true;
                    enciende_bomba(GPIO_BOMBA);
                }
            }
            else
            {
                // Enviamos mensaje de estado a Blynk
                if (xSemaphoreTake(GlobalKey, portMAX_DELAY) == pdTRUE)
                {
                    envia_Blynk("mensaje_estado", "Nivel de tanque de agua bajo. Por favor rellenar");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    xSemaphoreGive(GlobalKey);
                }
                ESP_LOGW(MAIN_TAG, "Nivel de agua en el tanque bajo");
            }
        }
        else
        {
            // Enviamos mensaje de estado a Blynk
            if (xSemaphoreTake(GlobalKey, portMAX_DELAY) == pdTRUE)
            {
                envia_Blynk("mensaje_estado", "Humedad normal");
                vTaskDelay(pdMS_TO_TICKS(2000));
                xSemaphoreGive(GlobalKey);
            }
            // Apagamos la bomba de agua
            if (estado_bomba)
            {
                estado_bomba = false;
                apaga_bomba(GPIO_BOMBA);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(PERIODO_MUESTREO_HUMEDAD));
    }
}

esp_err_t crea_tareas(void)
{
    // Con esta funcion creamos las tareas que se ejecutaran en el sistema, aparte de la tarea principal

    // Creamos la tarea de control de temperatura. PRIORIDAD: 3
    static uint8_t parametrosTempTask; // Si quisieramos pasar algun parametro a la tarea
    TaskHandle_t xTempHandle;          // Esta variable apunta a la tarea creada. Nos sirve para modificar la tarea (pausar, eliminar, etc)
    xTaskCreate(&vTaskControlTemperatura, "control_temperatura", STACK_SIZE, &parametrosTempTask, 3, &xTempHandle);

    // Creamos la tarea de control de humedad. PRIORIDAD: 4
    static uint8_t parametrosHumedadTask;
    TaskHandle_t xHumedadHandle;
    xTaskCreate(&vTaskControlHumedad, "control_humedad", STACK_SIZE, &parametrosHumedadTask, 4, &xHumedadHandle);


    return ESP_OK;
}
