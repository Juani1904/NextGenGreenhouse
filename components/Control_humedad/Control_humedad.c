#include <stdio.h>
#include "Control_humedad.h"

#define GPIO_HUMEDAD (ADC1_CHANNEL_6) //Pin de conexion del sensor de humedad. GPIO 34 (Input only)
#define GPIO_NIVEL_TANQUE (ADC1_CHANNEL_7) //Pin de conexion del sensor de nivel de agua. GPIO 35 (Input only)
#define GPIO_BOMBA (GPIO_NUM_32) //Pin de conexion de la bomba de agua

static const char *H_CONTROL = "Control humedad"; //Tag para los logs


void adc_pins_init(void){
    //Configuramos los channel ADC 6 y 7 (GPIO 34 y 35 respectivamente) para tomar las medidas de humedad
    adc1_config_width(ADC_WIDTH_BIT_10); //Seteamos una resolución de 10 bit, para mediciones de 0 a 1023 valores
    adc1_config_channel_atten(GPIO_HUMEDAD, ADC_ATTEN_DB_11); //Usamos una atenuación de 11 dB, para obtener mediciones de 150 ~ 2450 mV
    adc1_config_channel_atten(GPIO_NIVEL_TANQUE, ADC_ATTEN_DB_11);
}


void mide_humedad(param_cont_humedad *parametros){

    //Tomamos las medidas de humedad
    int16_t humedad_raw = adc1_get_raw(GPIO_HUMEDAD);
    //Convertimos las medidas de humedad a porcentaje
    parametros->humedad = humedad_raw*100/1023;

}

void enciende_bomba(void){
    //Mostramos un mensaje en los logs
    ESP_LOGI(H_CONTROL,"Bomba encendida");
    //Configuramos el pin de la bomba como salida
    gpio_set_direction(GPIO_BOMBA, GPIO_MODE_OUTPUT);
    //Encendemos la bomba
    gpio_set_level(GPIO_BOMBA, 1);

}

void apaga_bomba(void){
    //Mostramos un mensaje en los logs
    ESP_LOGI(H_CONTROL,"Bomba apagada");
    //Apagamos la bomba
    gpio_set_level(GPIO_BOMBA, 0);
}

void mide_nivel_tanque(param_cont_humedad *parametros){
    //Tomamos las medidas de nivel de agua
    int16_t nivel_tanque_raw = adc1_get_raw(GPIO_NIVEL_TANQUE);
    //Convertimos las medidas de nivel de agua a porcentaje
    parametros->nivel_tanque = nivel_tanque_raw*100/1023;

}
