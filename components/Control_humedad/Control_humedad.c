#include <stdio.h>
#include "Control_humedad.h"

//Tag para los logs
static const char *H_CONTROL = "Control humedad"; //Tag para los logs


void adc_pins_init(gpio_num_t pin_sensor, gpio_num_t pin_bomba){
    //Configuramos los channel ADC 6 y 7 (GPIO 34 y 35 respectivamente) para tomar las medidas de humedad
    adc1_config_width(ADC_WIDTH_BIT_10); //Seteamos una resolución de 10 bit, para mediciones de 0 a 1023 valores
    adc1_config_channel_atten(pin_sensor, ADC_ATTEN_DB_11); //Usamos una atenuación de 11 dB, para obtener mediciones de 150 ~ 2450 mV
    adc1_config_channel_atten(pin_bomba, ADC_ATTEN_DB_11);
}


void mide_humedad(param_cont_humedad *parametros, gpio_num_t pin_sensor){

    //Tomamos las medidas de humedad
    int16_t humedad_raw = adc1_get_raw(pin_sensor);
    //Convertimos las medidas de humedad a porcentaje
    //Dado que el sensor entrega el valor maximo para un suelo TOTALMENTE SECO y el minimo sumergido en agua entonces la formula sera
    //Restamos 100 al valor de humedad_raw a modo a ajuste 
    parametros->humedad = 100-(humedad_raw-100)*100/1023;

}

void enciende_bomba(gpio_num_t pin_sensor){
    //Mostramos un mensaje en los logs
    ESP_LOGI(H_CONTROL,"Bomba encendida");
    //Configuramos el pin de la bomba como salida
    gpio_set_direction(pin_sensor, GPIO_MODE_OUTPUT);
    //Encendemos la bomba
    gpio_set_level(pin_sensor, 1);

}

void apaga_bomba(gpio_num_t pin_bomba){
    //Mostramos un mensaje en los logs
    ESP_LOGI(H_CONTROL,"Bomba apagada");
    //Apagamos la bomba
    gpio_set_level(pin_bomba, 0);
}

void mide_nivel_tanque(param_cont_humedad *parametros, gpio_num_t pin_sensor){
    //Tomamos las medidas de nivel de agua
    int16_t nivel_tanque_raw = adc1_get_raw(pin_sensor);
    //Convertimos las medidas de nivel de agua a porcentaje
    parametros->nivel_tanque = (nivel_tanque_raw)*40/420+60;

}
