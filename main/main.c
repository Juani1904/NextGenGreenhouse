/*Incluimos las librerias necesarias para el funcionamiento de nuestro script*/

#include <stdio.h> //Sirve para la entrada y salida de datos
#include <stdint.h> //Sirve para definir tipos de datos
#include "freertos/FreeRTOS.h" //Libreria para configurar el sistema operativo FreeRTOS
#include "freertos/task.h" //Libreria para configurar las tareas

#include "esp_log.h" //Libreria para configurar los logs

#include "Control_temperatura.h" //Libreria para configurar el control de temperatura

#include "Blynk_MQTT_Connect.h" //Libreria para configurar la conexion con el servidor Blynk



//Definimos algunos parametros que se utilizaran en este script

/*----------------------------------LOGS y TAREAS----------------------------------------*/
static const char *MAIN_TAG = "Main";
#define STACK_SIZE 2048 //Definimos el tamaño de la pila de las tareas en bytes

/*--------------------------------CONTROL TEMPERATURA-----------------------------------------------*/
//Definimos el tiempo de muestreo del sensor DS18B20
#define PERIODO_MUESTREO (1000)
//Pin de conexion de One Wire del sensor DS18B20
#define GPIO_DS18B20_0 (GPIO_NUM_13) 
//Declaramos variable parámetros temperatura
param_cont_temperatura parametros_temperatura;

/*--------------------------------CONTROL HUMEDAD-----------------------------------------------*/



//Declaramos las funciones que se utilizaran en este script
esp_err_t crea_tareas(void);



void app_main(void)
{
    conecta_servidor();
    int8_t contador=0;
    char contadorC[10];
    /* for (int i=0 ;i<10; i++){
        sprintf(contadorC,"%d",contador);
        envia_Blynk("test","Hola mundo");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        contador++;
        
    } */

    //Creamos las tareas del sistema
    crea_tareas();
    
}

void vTaskMideTemperatura(void *pvParameters){
    //Seteamos la resistencia de pull up interna.
    gpio_set_pull_mode(GPIO_DS18B20_0, GPIO_PULLUP_ONLY);
    //Aplicamos un delay de 2 segundos para que el sensor se estabilice
    vTaskDelay(pdMS_TO_TICKS(2000));
    //Comprobamos la conexion con el sensor
    comprueba_sensor_temperatura();
    //Iniciamos el sensor
    inicia_sensor_temperatura();
    while(1){
    //Medimos la temperatura
    mide_temperatura(&parametros_temperatura);
    //Lo enviamos a Blynk
    char temperaturaC[10];
    sprintf(temperaturaC,"%.1f",parametros_temperatura.temperatura);
    envia_Blynk("temp",temperaturaC);
    printf("Temperature reading: %.1f\n", parametros_temperatura.temperatura);
    vTaskDelay(pdMS_TO_TICKS(PERIODO_MUESTREO));
    }
    

}

    
esp_err_t crea_tareas(void){
    //Con esta funcion creamos las tareas que se ejecutaran en el sistema, aparte de la tarea principal

    //Creamos la tarea de sensor de temperatura. PRIORIDAD: 3 (Por ahora)
    static uint8_t parametrosTempTask; //Si quisieramos pasar algun parametro a la tarea
    TaskHandle_t xTempHandle; //Esta variable apunta a la tarea creada. Nos sirve para modificar la tarea (pausar, eliminar, etc)
    xTaskCreate(&vTaskMideTemperatura, "mide_temperatura",STACK_SIZE, &parametrosTempTask, 5, &xTempHandle);

    //Mas tareas....

    return ESP_OK;
}
