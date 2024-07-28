/*Script para conexión a internet y envio/recepción de datos a Blynk Iot por protocolo MQTT */

#include <stdio.h> //Sirve para la entrada y salida de datos
#include <stdint.h> //Sirve para definir tipos de datos
#include <stddef.h> //Sirve para definir tipos de datos
#include <string.h> //Sirve para manipular cadenas de caracteres
#include "esp_wifi.h" //Libreria para configurar el modulo wifi
#include "esp_system.h" //Libreria para configurar el sistema
#include "nvs_flash.h" //Libreria para configurar la memoria flash
#include "esp_event.h" //Libreria para configurar eventos
#include "esp_netif.h" //Libreria para configurar la interfaz de red (Stack TCP/IP)
#include <driver/gpio.h> //Libreria para configurar los pines GPIO

#include "freertos/FreeRTOS.h" //Libreria para configurar el sistema operativo FreeRTOS
#include "freertos/task.h" //Libreria para configurar las tareas
#include "freertos/semphr.h" //Libreria para configurar los semaforos
#include "freertos/queue.h" //Libreria para configurar las colas
#include "freertos/event_groups.h" //Libreria para configurar los grupos de eventos

#include "lwip/sockets.h" //Libreria para configurar los sockets
#include "lwip/dns.h" //Libreria para configurar el servidor DNS
#include "lwip/netdb.h" //Libreria para configurar la base de datos de red
#include "lwip/err.h" //Libreria para configurar los errores
#include "lwip/sys.h" //Libreria para configurar el sistema

#include "esp_log.h" //Libreria para configurar los logs
#include "mqtt_client.h" //Libreria para configurar el cliente MQTT

#include "passwords.h" //Libreria para configurar todas las credenciales/contraseñas del proyecto

#include "Control_temperatura.h" //Libreria para configurar el control de temperatura
#include "Control_humedad.h" //Libreria para configurar el control de humedad


/**
 * @brief Funcion para conectar al servidor MQTT
 * @details Funcion que se encarga de conectar al servidor MQTT, y se suscribe a los topics necesarios
 */
void conecta_servidor(void);

/**
 * @brief Funcion para inicializar la estacion WIFI
 */
static void conecta_wifi(void);

/**
 * @brief Funcion para inicializar el cliente MQTT
 */
static void inicia_cliente_mqtt(void);

/**
 * @brief Función para apuntar a los parámetros de temperatura
 * @details Función que apunta a la direccion de memoria del struct de parametros de temperatura para poder acceder a ellos y leerlos/modificarlos
 * desde los métodos del script de Blynk_MQTT_Connect
 * @param parametros estructura de parámetros de temperatura que le pasamos a la función para apuntar a dichas variables con un puntero
 */
void apunta_parametros_temperatura(param_cont_temperatura* parametros);

/**
 * @brief Función para apuntar a los parámetros de humedad
 * @details Función que apunta a la direccion de memoria del struct de parametros de humedad para poder acceder a ellos y leerlos/modificarlos
 * desde los métodos del script de Blynk_MQTT_Connect
 * @param parametros estructura de parámetros de humedad que le pasamos a la función para apuntar a dichas variables con un puntero
 */
void apunta_parametros_humedad(param_cont_humedad* parametros);

/**
 * @brief Funcion para enviar datos a Blynk
 * @details Recibe la data de Blynk, y manda el dato a los actuadores, determinando a que actuador debe enviarse
 * @param event evento de MQTT
 */
void recibe_Blynk(esp_mqtt_event_handle_t event);

/**
 * @brief Funcion para recibir datos de Blynk
 * @details Recibe la data de los sensores, y manda el dato a Blynk, determinando a que topic debe enviarse
 * @param cmd_id identificador del comando. Determina que hacer con el dato que se envia
 * @param data datos del sensor
 */
void envia_Blynk(char *cmd_id, char *data);

/**
 * @brief Handler de eventos WIFI
 *
 * @param arg variable de argumento
 * @param event_base base de eventos
 * @param event_id id del evento
 * @param event_data datos del evento 
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/**
 * @brief Funcion para inicializar el cliente MQTT
 * 
 * @param handler_args argumentos del handler
 * @param base base de eventos
 * @param event_id id del evento
 * @param event_data datos del evento
 * 
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);


/**
 * @brief Funcion para loggear errores MQTT
 * @details La función recibe un mensaje y un código de error. Si el código es distinto de cero, muestra el mensaje y el código de error
 * @param mensaje 
 * @param codigo_error 
 */
static void print_error(const char *mensaje, int codigo_error); //Funcion para loggear errores
