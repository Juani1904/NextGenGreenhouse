/*Incluimos las librerias necesarias para el funcionamiento de nuestro script*/

#include <stdio.h> //Sirve para la entrada y salida de datos
#include <stdint.h> //Sirve para definir tipos de datos
#include "freertos/FreeRTOS.h" //Libreria para configurar el sistema operativo FreeRTOS
#include "freertos/task.h" //Libreria para configurar las tareas

#include "esp_log.h" //Libreria para configurar los logs

//Librerias protocolo OneWire y DS18B20
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

#include "Blynk_MQTT_Connect.h" //Libreria para configurar la conexion con el servidor Blynk



//Definimos algunos parametros que se utilizaran en este script

/*----------------------------------LOGS y TAREAS----------------------------------------*/
static const char *MAIN_TAG = "Main";
#define STACK_SIZE 2048 //Definimos el tamaño de la pila de las tareas en bytes

/*--------------------------------SENSOR TEMPERATURA-----------------------------------------------*/
#define MAX_DEVICES (1) //Definimos maximo de dispositivos one wire de la linea. Como solo tenemos uno, ponemos 1
//Definimos la resolucion de la medicion del sensor. 12 bits es la mejor resolucion
#define DS18B20_RESOLUTION (DS18B20_RESOLUTION_12_BIT)
//Definimos el tiempo de muestreo del sensor DS18B20
#define PERIODO_MUESTREO (1000)
uint8_t umbralSuperiorTemp;
uint8_t umbralInferiorTemp;



//Definiciones de pines
#define GPIO_DS18B20_0 (GPIO_NUM_13) //Pin de conexion de One Wire del sensor DS18B20


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
    //Esta tarea se encarga de medir la temperatura cada cierto tiempo. Utilizaremos un sensor DS18B20
    float temperatura;
    //Seteamos la resistencia de pull up interna.
    gpio_set_pull_mode(GPIO_DS18B20_0, GPIO_PULLUP_ONLY);
    //Aplicamos un delay de 2 segundos para que el sensor se estabilice
    vTaskDelay(pdMS_TO_TICKS(2000));
    //Creamos el bus de OneWire, utilizando el driver RMT
    // Create OneWire bus and driver
    OneWireBus *oneWireBus;
    owb_rmt_driver_info rtmDriverInfo;
    //Inicializamos el bus de OneWire
    oneWireBus = owb_rmt_initialize(&rtmDriverInfo, GPIO_DS18B20_0, RMT_CHANNEL_1, RMT_CHANNEL_0);
    /*------------------------COMPROBACIÓN DE CONEXIÓN Y CODIGO-----------------------------------*/
    //Habilitamos la comprobacion CRC para el codigo ROM
    owb_use_crc(oneWireBus, true);
    // Leemos ROM del dispositivo para verificar que esta conectado y cual es su codigo
    OneWireBus_ROMCode codigoRom;
    char codigoRomString[17];
    owb_status status = owb_read_rom(oneWireBus, &codigoRom); //Leemos el codigo ROM del dispositivo. La llamada nos entrega el status
    //Si es correcto lo transformamos a string y lo mostramos. Si no mostramos un error
    if (status == OWB_STATUS_OK)
    {
        owb_string_from_rom_code(codigoRom, codigoRomString, sizeof(codigoRomString));
        ESP_LOGI(MAIN_TAG,"Sensor conectado. Código: %s\n", codigoRomString);
    }
    else
    {
        ESP_LOGE(MAIN_TAG,"No se encuentra ningún dispositivo. Status: %d", status);
    }

    /*--------------------------------------------CONEXION CON DS18B20---------------------------------------------------------------*/
    //Aqui realmente hacemos lo mismo que en el caso anterior, sin embargo esta vez es para conectarnos al sensor una vez que ya verificamos
    //que esta conectado

    // Reservamos memoria para el dispositivo DS18B20
    DS18B20_Info *dispositivo = ds18b20_malloc();
    //Inicializamos el dispositivo DS18B20
    ds18b20_init_solo(dispositivo, oneWireBus);
    //Habilitamos la comprobacion CRC para todas las lecturas
    ds18b20_use_crc(dispositivo, true);
    //Seteamos la resolucion de la medicion del sensor
    ds18b20_set_resolution(dispositivo, DS18B20_RESOLUTION);

    //Medimos la temperatura cada cierto tiempo
    while(1){
        // Convert temperature on the DS18B20 device
        ds18b20_convert_all(oneWireBus);

        // Wait for conversion to complete
        ds18b20_wait_for_conversion(dispositivo);

        // Read the temperature from the DS18B20 device
        DS18B20_ERROR error = 0;
        error = ds18b20_read_temp(dispositivo, &temperatura);
        if (error != DS18B20_OK)
        {
            printf("Ocurrión un error leyendo la temperatura\n");
        }
        else
        {
            //Lo enviamos a Blynk
            char temperaturaC[10];
            sprintf(temperaturaC,"%.1f",temperatura);
            envia_Blynk("temp",temperaturaC);
            printf("Temperature reading: %.1f\n", temperatura);
        }
        vTaskDelay(pdMS_TO_TICKS(PERIODO_MUESTREO));
        //Utilizamos la funcion de recall para que la alarma Th y Tl del sensor pase el dato al scratchpad
        owb_write_byte(dispositivo->bus, 0xEC); //Alarm Search
        uint8_t dato=0;
        owb_read_byte(dispositivo->bus, &dato);
        printf("Dato: %d\n",dato);
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
