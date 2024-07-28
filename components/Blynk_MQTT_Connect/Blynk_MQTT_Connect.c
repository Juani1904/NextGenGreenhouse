#include <stdio.h>
#include "Blynk_MQTT_Connect.h"

/*------------------------------------------------------WIFI------------------------------------------------------------ */
#define WIFI_CONNECTED_BIT BIT0 // Nos logramos conectar al AP (Punto de acceso/Router)
#define WIFI_FAIL_BIT BIT1      // No nos logramos conectar al AP (Punto de acceso/Router)
#define ESP_MAXIMUM_RETRY 10    // Numero maximo de intentos de conexion

static int intentos_conexion = 0; // Numero de intentos de conexion al AP (Punto de acceso/Router)
/* Creamos un event group de FreeRTOS */
// Un event group es un conjunto de bits que las aplicaciones pueden usar y darles un significado
static EventGroupHandle_t WIFI_EVENT_GROUP;
// El event group nos habilita multiples bits para cada evento, pero solo nos importan dos eventos:

// Tag para logs
static const char *WIFI_TAG = "Estacion WIFI";

/*------------------------------------------------------CLIENTE MQTT------------------------------------------------------------ */

// Definimos la estructura cliente como variable global
esp_mqtt_client_handle_t client;

// Tag para logs
static const char *MQTT_TAG = "Cliente MQTT";

/*-----------------------------------------------------Control de temperatura-----------------------------------------*/

// Declaramos un puntero a la estructura de parametros de control de temperatura
param_cont_temperatura *parametros_temperatura_Blynk;

/*-----------------------------------------------------Control de humedad-----------------------------------------*/

// Declaramos un puntero a la estructura de parametros de control de humedad
param_cont_humedad *parametros_humedad_Blynk;

/*------------------------------------------------------------------------------------------------------------------------------*/

void conecta_servidor(void)
{
    //Printeamos log de inicio de conexion al servidor
    ESP_LOGI(MQTT_TAG, "Inicio");
    ESP_LOGI(MQTT_TAG, "Memoria libre: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(MQTT_TAG, "Versión IDF: %s", esp_get_idf_version());

    // Utilizamos la funcion de la libreria NVS (Non Volatile Storage) que nos permite almacenar datos en la memoria flash
    // En esta memoria se van a almacenar temporalmente sobre todo la información de los logs, eventos, etc

    // Inicializamos la memoria flash
    esp_err_t ret = nvs_flash_init();

    // Si no hay espacio en la memoria flash o hay una nueva version encontrada, borramos la memoria flash
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    // Utilizamos ESP-NETIF para crear una capa de abstraction para la aplicación, por encima del stack TCP/IP, para no tener que lidiar con el stack directamente
    esp_netif_init();

    // Creamos un loop de eventos del tipo "default" para atajar los eventos de la aplicacion (Eventos WIFI y Eventos MQTT)
    esp_event_loop_create_default();

    // Inicializamos la estacion WIFI
    conecta_wifi();

    // Inicializamos el cliente MQTT
    inicia_cliente_mqtt();
}

static void conecta_wifi(void)
{
    WIFI_EVENT_GROUP = xEventGroupCreate(); // Creamos un grupo de eventos para la conexion WIFI

    esp_netif_create_default_wifi_sta(); // Se crea la estación wifi con netif

    // Definimos el parametro de configuracion de la estacion WIFI.
    // Este es un struct que contiene la configuracion de la estacion WIFI
    // Le pasamos la configuracion por defecto mediante la llamada a la funcion WIFI_INIT_CONFIG_DEFAULT()
    wifi_init_config_t config_wifi_inicio = WIFI_INIT_CONFIG_DEFAULT();

    // Llamamos a la funcion esp_wifi_init() para inicializar el modulo WIFI, pasandole como parametro la direccion de mem de la variable de config
    esp_wifi_init(&config_wifi_inicio);

    //Configuramos el llamado al handler de eventos de WIFI
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_got_ip);

    // Configuramos la estacion WIFI
    wifi_config_t config_wifi = {
        .sta = {
            .ssid = WIFI_SSID,     // Usuario WIFI
            .password = WIFI_PASS, // Contraseña WIFI
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Modo de autenticacion WPA2
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH, //Param configuracion seguridad WPA3 SAE
            .sae_h2e_identifier = "", //Este parametro debe ser siempre un string vacio
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);               // Establecemos el modo de la estacion WIFI. Esto para conectarnos A UN PUNTO DE ACCESO, NO CREAR UNO
    esp_wifi_set_config(WIFI_IF_STA, &config_wifi); // Establecemos la configuracion de la estacion WIFI
    esp_wifi_start();                               // Iniciamos el modulo WIFI

    ESP_LOGI(WIFI_TAG, "Establecimiento de estación WIFI finalizado");

    // Definimos la variable bits_estado_wifi, en la cual se almacenan los bits de estado de la conexion WIFI
    // Con la funcion xEventGroupWaitBits() esperamos a que se conecte al AP (Punto de acceso/Router) y ver cual es el resultado
    // Si en el wifi_event_handler la conexion fue exitosa, salta el flag de conexion exitosa (WIFI_CONNECTED_BIT)
    // Si en el wifi_event_handler la conexion fue fallida, salta el flag de conexion fallida (WIFI_FAIL_BIT)
    EventBits_t bits_estado_wifi = xEventGroupWaitBits(WIFI_EVENT_GROUP,
                                                       WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                                       pdFALSE,
                                                       pdFALSE,
                                                       portMAX_DELAY);

    // Analizamos los bits de estado de la conexion WIFI y print    eamos el resultado
    if (bits_estado_wifi & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(WIFI_TAG, "Conectado al punto de acceso SSID: %s Pass: %s",
                 WIFI_SSID, WIFI_PASS);
    }
    else if (bits_estado_wifi & WIFI_FAIL_BIT)
    {
        ESP_LOGI(WIFI_TAG, "Fallo al conectar al SSID: %s, password: %s",
                 WIFI_SSID, WIFI_PASS);
    }
    else
    {
        ESP_LOGE(WIFI_TAG, "EVENTO INESPERADO");
    }
}

static void inicia_cliente_mqtt(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://ny3.blynk.cloud:1883",
        .credentials.authentication.password = BLYNK_AUTH_TOKEN,
        .credentials.username = BLYNK_USERNAME,
        .credentials.client_id = BLYNK_USERNAME,
        .session.keepalive = 45,
        .session.disable_clean_session = false};

    client = esp_mqtt_client_init(&mqtt_cfg);
    
    // Configuramos el llamado al handler de eventos MQTT
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void apunta_parametros_temperatura(param_cont_temperatura *parametros)
{
    parametros_temperatura_Blynk = parametros;
}

void apunta_parametros_humedad(param_cont_humedad *parametros)
{
    parametros_humedad_Blynk = parametros;
}

void recibe_Blynk(esp_mqtt_event_handle_t event)
{
    // Usamos la funcion strcmp para comparar la cadena de texto que llega en el topic con un string
    // Si ambos son iguales la funcion entrega un 0
    // Es necesario pasarle el tamaño de la cadena de texto que llega en el topic, para evitar el ruido
    if (strncmp(event->topic, "downlink/ds/temp_ideal", event->topic_len) == 0)
    {
        int tempValue; // Valor temporal para almacenar el dato de sscanf
        // Usamos sscanf para tomar las 2 primeras letras del dato que llega en el evento. Si llegan más de 2 letras, no las toma
        sscanf(event->data, "%2d", &tempValue);
        parametros_temperatura_Blynk->temperatura_ideal = tempValue;
        parametros_temperatura_Blynk->limite_sup_temp = parametros_temperatura_Blynk->temperatura_ideal + 5;
        parametros_temperatura_Blynk->limite_inf_temp = parametros_temperatura_Blynk->temperatura_ideal - 5;
    }

    // Bloque de código para recibir el dato de reseteo del controlador
    if (strncmp(event->topic, "downlink/ds/RESET", event->topic_len) == 0)
    {
        int resetValue; // Almacenamos el valor de reseteo puede ser 1 o 0
        sscanf(event->data, "%1d", &resetValue);
        if (resetValue == 1)
        {
            // Enviamos un 0 al mismo topic para resetear el valor
            envia_Blynk("RESET", "0");
            // Esperamos 1 segundo para que el mensaje llegue a Blynk
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            // Liberamos las memorias dinamicas utilizadas
            free(parametros_temperatura_Blynk);
            free(parametros_humedad_Blynk);
            // Reseteamos la esp
            esp_restart();
        }
    }
}

void envia_Blynk(char *cmd_id, char *data)
{
    // MENSAJES DE ESTADO
    if (strcmp(cmd_id, "mensaje_estado") == 0)
    {
        esp_mqtt_client_publish(client, "ds/mensaje_estado", data, 0, 0, 0);
    }
    // TEMPERATURA MEDIDA
    else if (strcmp(cmd_id, "temp") == 0)
    {
        esp_mqtt_client_publish(client, "ds/temp", data, 0, 0, 0);
    }
    // HUMEDAD MEDIDA
        else if (strcmp(cmd_id, "humedad") == 0)
    {
        esp_mqtt_client_publish(client, "ds/humedad", data, 0, 0, 0);
    }
    // PETICIÓN DE SINCRONIZACIÓN DE DATOS
    else if (strcmp(cmd_id, "sync") == 0)
    {
        // Como nos interesa saber el estado de la sincronización, usamos un QoS=1 y revisamos el return de la función
        int8_t msg_id;
        int8_t QoS = 1;
        msg_id = esp_mqtt_client_publish(client, "get/ds/all", NULL, 0, QoS, 0);
        // Revisamos que la sincronización sea exitosa
        if (msg_id == -1)
        {
            ESP_LOGI(MQTT_TAG, "Error en la sincronización");
        }
        else
        {
            ESP_LOGI(MQTT_TAG, "Sincronización exitosa");
        }
    }
    else if (strcmp(cmd_id, "RESET") == 0)
    {
        esp_mqtt_client_publish(client, "ds/RESET", data, 0, 0, 0);
    }
    else
    {
        ESP_LOGE(MQTT_TAG, "Comando no reconocido");
    }
}

static void print_error(const char *mensaje, int codigo_error)
{
    // Funcion para loggear errores
    // Si el codigo de error es distinto de 0 (es decir, existe error) se printea el mensaje de error
    if (codigo_error != 0)
    {
        ESP_LOGE(MQTT_TAG, "Last error %s: 0x%x", mensaje, codigo_error);
    }
}

/*-----------------------------------------------EVENTS HANDLERS----------------------------------------------------------*/

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {                       // Revisamos si la base del evento es del tipo wifi. Y ademas, si el ide del evento es de tipo WIFI_EVENT_STA_START
        esp_wifi_connect(); // Si es asi, llamamos a la funcion esp_wifi_connect() para conectar la estacion WIFI al AP (Punto de acceso/Router)
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    { // Revisamos si la base del evento es del tipo wifi. Y ademas, si el id del evento es de tipo WIFI_EVENT_STA_DISCONNECTED
        if (intentos_conexion < ESP_MAXIMUM_RETRY)
        {                       // Si es asi, verificamos si el numero de intentos de conexion es menor al maximo de intentos
            esp_wifi_connect(); // Intentamos conectar tantas veces como indique la variable ESP_MAXIMUM_RETRY
            intentos_conexion++;
            ESP_LOGI(WIFI_TAG, "Intento %d de reconexión al punto de acceso", intentos_conexion);
        }
        else
        {
            xEventGroupSetBits(WIFI_EVENT_GROUP, WIFI_FAIL_BIT); // En caso de no lograr conectarnos, seteamos el bit de fallo del evento group en 1
        }
        ESP_LOGI(WIFI_TAG, "Conexión fallida al punto de acceso");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {                                                                         // Si el evento es de tipo IP_EVENT y el id del evento es de tipo IP_EVENT_STA_GOT_IP
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;           // Recuperamos la informacion del evento (en este caso la ip asignada) en la variable event
        ESP_LOGI(WIFI_TAG, "IP asignada " IPSTR, IP2STR(&event->ip_info.ip)); // Printeamos la ip asignada
        intentos_conexion = 0;                                                // Reseteamos el numero de intentos de conexion
        xEventGroupSetBits(WIFI_EVENT_GROUP, WIFI_CONNECTED_BIT);             // Seleccionamos el bit de conexion exitosa del event group
    }
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Evento proveniente de event loop Base=%s, Event ID=%" PRIi32 "", base, event_id);
    // Almacenamos los datos del evento en la variable event
    // Esta es un struct que contiene toda la informacion del evento. Los que nos interesan son el topic, el dato y el tamaño de ambos
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client; // Cliente MQTT
    int MSG_ID=0;

    // Creamos un switch para manejar el comportamiento segun los diferentes eventos
    switch ((esp_mqtt_event_id_t)event_id)
    {

        case MQTT_EVENT_CONNECTED: // Evento de conexión inicial
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
            // Nos suscribimos a los topics que nos interesan
            /*-------------------------------------------------------DOWNLINK TOPICS-------------------------------------------------------- */
            // Los topics de downlink son los topics a los que nos suscribimos para recibir datos de Blynk. Cada topic corresponde a un DATASTREAM (actuador del dashboard de blynk)
            MSG_ID = esp_mqtt_client_subscribe(client, "downlink/ds/temp_ideal", 0); // TEMPERATURA IDEAL
            MSG_ID = esp_mqtt_client_subscribe(client, "downlink/ds/RESET", 0); // RESET
            break;

        case MQTT_EVENT_DISCONNECTED: // Evento de desconexión
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED: // Evento de suscripción
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, MSG_ID= %d", event->msg_id);
            ESP_LOGI(MQTT_TAG, "Subscripción exitosa, MSG_ID= %d", MSG_ID);
            break;

        case MQTT_EVENT_UNSUBSCRIBED: // Evento de desuscripción
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, MSG_ID= %d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED: // Evento de publicación
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, MSG_ID= %d", event->msg_id);
            break;

        case MQTT_EVENT_DATA: // Evento de recepción de datos.
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");
            // Dejamos comentado el printeo de los datos que llegan en el evento para debuggear
            //printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            //printf("DATA=%.*s\r\n", event->data_len, event->data);
            // Llamamos a la funcion recibe_Blynk para procesar el dato que llega en el evento y enviarlo al actuador fisico correspondiente
            recibe_Blynk(event);
            break;

        case MQTT_EVENT_ERROR: // Evento de error
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
            // Printeamos el error que llega en el evento
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
            {
                print_error("Reported from esp-tls", event->error_handle->esp_tls_last_esp_err);                    // Tipo de Error 1
                print_error("Reported from tls stack", event->error_handle->esp_tls_stack_err);                     // Tipo de Error 2
                print_error("Captured as transport's socket errno", event->error_handle->esp_transport_sock_errno); // Tipo de Error 3
                ESP_LOGI(MQTT_TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));       // Tipo de Error 4
            }
            break;

        default:
            ESP_LOGI(MQTT_TAG, "Otros eventos MQTT: %d", event->event_id);
            break;
    }
}






