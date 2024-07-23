#include <stdio.h>
#include "Control_temperatura.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#define MAX_DEVICES (1) //Definimos maximo de dispositivos one wire de la linea. Como solo tenemos uno, ponemos 1
//Definimos la resolucion de la medicion del sensor. 12 bits es la mejor resolucion
#define DS18B20_RESOLUTION (DS18B20_RESOLUTION_12_BIT)
//Definimos el tiempo de muestreo del sensor DS18B20
#define PERIODO_MUESTREO (1000)
#define GPIO_DS18B20_0 (GPIO_NUM_13) //Pin de conexion de One Wire del sensor DS18B20

static const char *T_SENSOR_TAG = "Sensor DS18B20";
//Creamos el bus de OneWire, utilizando el driver RMT
OneWireBus *oneWireBus;
owb_rmt_driver_info rtmDriverInfo;

DS18B20_Info *dispositivo;

void comprueba_sensor_temperatura(void){
    
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
        ESP_LOGI(T_SENSOR_TAG,"Sensor conectado. Código: %s\n", codigoRomString);
    }
    else
    {
        ESP_LOGE(T_SENSOR_TAG,"No se encuentra ningún dispositivo. Estado: %d", status);
    }
}

void inicia_sensor_temperatura(void){
    // Reservamos memoria para el dispositivo DS18B20
    dispositivo = ds18b20_malloc();
    //Inicializamos el dispositivo DS18B20
    ds18b20_init_solo(dispositivo, oneWireBus);
    //Habilitamos la comprobacion CRC para todas las lecturas
    ds18b20_use_crc(dispositivo, true);
    //Seteamos la resolucion de la medicion del sensor
    ds18b20_set_resolution(dispositivo, DS18B20_RESOLUTION);
}

void mide_temperatura(param_cont_temperatura *parametros){

    //Ejecuta la función del DS18B20 CONVERT T [0x44] para iniciar la conversión de temperatura a los 12 bits
    ds18b20_convert_all(oneWireBus);
    //Espera a que la conversión de temperatura se complete con éxito
    ds18b20_wait_for_conversion(dispositivo);
    //Leemos la temperatura del sensor
    DS18B20_ERROR error = 0;
    error = ds18b20_read_temp(dispositivo, &parametros->temperatura);
    if (error != DS18B20_OK)
    {
        ESP_LOGE(T_SENSOR_TAG,"Ocurrió un error leyendo la temperatura\n");
    }
}

void enciende_climatizador(bool calor){
    if(calor){
        ESP_LOGI(T_SENSOR_TAG,"Encendiendo climatizador en modo calor\n");
    }else{
        ESP_LOGI(T_SENSOR_TAG,"Encendiendo climatizador en modo refrigeración\n");
    }
}

void apagar_climatizador(void){
    ESP_LOGI(T_SENSOR_TAG,"Apagando climatizador\n");
}

TaskFunction_t controla_ventilador(void *diferencia_temp){
    while(1){
        ESP_LOGI(T_SENSOR_TAG,"Controlando ventilador\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
}



