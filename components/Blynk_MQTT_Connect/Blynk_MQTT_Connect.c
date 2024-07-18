#include <stdio.h>
#include "Blynk_MQTT_Connect.h"

/*------------------------------------------------------CONFIGURACIÓN WIFI------------------------------------------------------------ */

static int intentos_conexion = 0; //Numero de intentos de conexion al AP (Punto de acceso/Router)

/* Creamos un event group de FreeRTOS */
//Un event group es un conjunto de bits que las aplicaciones pueden usar y darles un significado
static EventGroupHandle_t WIFI_EVENT_GROUP;
//El event group nos habilita multiples bits para cada evento, pero solo nos importan dos eventos:
#define WIFI_CONNECTED_BIT BIT0 //Nos logramos conectar al AP (Punto de acceso/Router)
#define WIFI_FAIL_BIT      BIT1 //No nos logramos conectar al AP (Punto de acceso/Router)

/*------------------------------------------------------CONFIGURACIÓN CLIENTE MQTT------------------------------------------------------------ */

//Definimos la estructura cliente como variable global
esp_mqtt_client_handle_t client;

//-------------------------------------------------MAIN: conecta_servidor------------------------------------------------------------

void conecta_servidor(void)
{
    //Definimos los logs de la aplicacion MQTT
    ESP_LOGI(MQTT_TAG, "[APP] Inicio..");
    ESP_LOGI(MQTT_TAG, "[APP] Memoria libre: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(MQTT_TAG, "[APP] Versión IDF: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    //Utilizamos la funcion de la libreria NVS (Non Volatile Storage) que nos permite almacenar datos en la memoria flash
    //En esta memoria se van a almacenar temporalmente sobre todo la información de los logs, eventos, etc

    //Inicializamos la memoria flash
    esp_err_t ret = nvs_flash_init();
    //Si no hay espacio en la memoria flash o hay una nueva version encontrada, borramos la memoria flash
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    //Verificamos que el return de la inicializacion de la memoria flash sea correcto (ESP_OK)
    ESP_ERROR_CHECK(ret);
    
    //Utilizamos ESP-NETIF para crear una capa de abstraction para la aplicación, por encima del stack TCP/IP, para no tener que lidiar con el stack directamente
    ESP_ERROR_CHECK(esp_netif_init());

    //Creamos un loop de eventos del tipo "default" para atajar los eventos de la aplicacion (Eventos WIFI y Eventos MQTT)
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    //Inicializamos la estacion WIFI
    conecta_wifi();

    //Inicializamos el cliente MQTT
    inicia_cliente_mqtt();
    
}


static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) { //Revisamos si la base del evento es del tipo wifi. Y ademas, si el ide del evento es de tipo WIFI_EVENT_STA_START
        esp_wifi_connect(); //Si es asi, llamamos a la funcion esp_wifi_connect() para conectar la estacion WIFI al AP (Punto de acceso/Router)
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { //Revisamos si la base del evento es del tipo wifi. Y ademas, si el id del evento es de tipo WIFI_EVENT_STA_DISCONNECTED
        if (intentos_conexion < ESP_MAXIMUM_RETRY) { //Si es asi, verificamos si el numero de intentos de conexion es menor al maximo de intentos
            esp_wifi_connect(); //Intentamos conectar tantas veces como indique la variable ESP_MAXIMUM_RETRY
            intentos_conexion++;
            ESP_LOGI(WIFI_TAG, "Intento %d de reconexión al punto de acceso",intentos_conexion);
        } else {
            xEventGroupSetBits(WIFI_EVENT_GROUP, WIFI_FAIL_BIT); //En caso de no lograr conectarnos, seteamos el bit de fallo del evento group en 1
        }
        ESP_LOGI(WIFI_TAG,"Conexión fallida al punto de acceso");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { //Si el evento es de tipo IP_EVENT y el id del evento es de tipo IP_EVENT_STA_GOT_IP
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data; //Recuperamos la informacion del evento (en este caso la ip asignada) en la variable event
        ESP_LOGI(WIFI_TAG, "IP asignada " IPSTR, IP2STR(&event->ip_info.ip)); //Printeamos la ip asignada
        intentos_conexion = 0; //Reseteamos el numero de intentos de conexion
        xEventGroupSetBits(WIFI_EVENT_GROUP, WIFI_CONNECTED_BIT); //Seteamos el bit de conexion exitosa en 1
    }
}

void conecta_wifi(void)
{
    WIFI_EVENT_GROUP = xEventGroupCreate(); //Creamos un grupo de eventos para la conexion WIFI

    esp_netif_create_default_wifi_sta();  //Funcion encargada de abortar la creacion de la interfaz de red en caso de error

    //Definimos el parametro de configuracion de la estacion WIFI.
    //Este es un struct que contiene la configuracion de la estacion WIFI
    //Le pasamos la configuracion por defecto mediante la llamada a la funcion WIFI_INIT_CONFIG_DEFAULT()
    wifi_init_config_t config_wifi_inicio = WIFI_INIT_CONFIG_DEFAULT();
    
    //Llamamos a la funcion esp_wifi_init() para inicializar el modulo WIFI, pasandole como parametro la direccion de mem de la variable de config
    ESP_ERROR_CHECK(esp_wifi_init(&config_wifi_inicio));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t config_wifi = {
        .sta = {
            .ssid = WIFI_SSID, //Usuario WIFI
            .password = WIFI_PASS, //Contraseña WIFI

            //NOTA
            /* El umbral de autenticación (authmode) se restablece a WPA2 de forma predeterminada si la contraseña cumple con los estándares de WPA2 (longitud de contraseña => 8).
             * Si desea conectar el dispositivo a redes WEP/WPA obsoletas, configure el valor del umbral
             * en WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK y establezca la contraseña con una longitud y formato que cumpla con los estándares
             * de WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) ); //Establecemos el modo de la estacion WIFI. Esto para conectarnos A UN PUNTO DE ACCESO, NO CREAR UNO
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config_wifi) ); //Establecemos la configuracion de la estacion WIFI
    ESP_ERROR_CHECK(esp_wifi_start()); //Iniciamos el modulo WIFI

    ESP_LOGI(WIFI_TAG, "Establecimiento de estación WIFI finalizado");

    //Definimos la variable bits_estado_wifi, en la cual se almacenan los bits de estado de la conexion WIFI
    //Con la funcion xEventGroupWaitBits() esperamos a que se conecte al AP (Punto de acceso/Router) y ver cual es el resultado
    //Si en el wifi_event_handler la conexion fue exitosa, salta el flag de conexion exitosa (WIFI_CONNECTED_BIT)
    //Si en el wifi_event_handler la conexion fue fallida, salta el flag de conexion fallida (WIFI_FAIL_BIT)
    EventBits_t bits_estado_wifi = xEventGroupWaitBits(WIFI_EVENT_GROUP,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    
    //Analizamos los bits de estado de la conexion WIFI y printeamos el resultado
    if (bits_estado_wifi & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI_TAG, "Conectado al punto de acceso SSID: %s Pass: %s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits_estado_wifi & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFI_TAG, "Fallo al conectar al SSID: %s, password: %s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(WIFI_TAG, "EVENTO INESPERADO");
    }
}




/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */

//Event handler de los eventos MQTT
//El event loop automaticamente le pasa como parametros la base de los evetos, el id del evento y los datos del evento
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Evento proveniente de event loop Base=%s, Event ID=%" PRIi32 "", base, event_id);
    //Almacenamos los datos del evento en la variable event
    //Esta es un struct que contiene toda la informacion del evento. Los que nos interesan son el topic, el dato y el tamaño de ambos
    esp_mqtt_event_handle_t event = event_data; 
    esp_mqtt_client_handle_t client = event->client; //Cliente MQTT
    int MSG_ID;

    //Creamos un switch para manejar el comportamiento segun los diferentes eventos
    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED: //Evento de conexión inicial
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");

        //Nos suscribimos a los topics que nos interesan
        /*-------------------------------------------------------DOWNLINK TOPICS-------------------------------------------------------- */
        //Los topics de downlink son los topics a los que nos suscribimos para recibir datos de Blynk. Cada topic corresponde a un DATASTREAM (actuador del dashboard de blynk)

        MSG_ID = esp_mqtt_client_subscribe(client, "downlink/ds/Switch", 0); //SWITCH
        
        
        //Se podria colocar un mensaje de subscripcion exitosa, sin embargo en el evento de suscripcion ya se printea un mensaje de suscripcion exitosa
        //ESP_LOGI(MQTT_TAG, "Suscripcion exitosa, MSG_ID= %d", MSG_ID);
        /* Dejo comentado el ejemplo de como desuscribirse de un topic
        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "Desubscripcion exitosa, msg_id=%d", msg_id); */
        break;

    case MQTT_EVENT_DISCONNECTED: //Evento de desconexión
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED: //Evento de suscripción
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, MSG_ID= %d", event->msg_id);
        MSG_ID = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(MQTT_TAG, "Subscripción exitosa, MSG_ID= %d", MSG_ID);
        break;

    case MQTT_EVENT_UNSUBSCRIBED: //Evento de desuscripción
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, MSG_ID= %d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:  //Evento de publicación
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, MSG_ID= %d", event->msg_id);
        break;

    case MQTT_EVENT_DATA: //Evento de recepción de datos. Este es el evento que más nos interesa aparte del de inicio de conexión
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");
        //Printeamos el topic y el dato que llega en el evento. Esto para probar, luego se comentará
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        //Llamamos a la funcion recibe_Blynk para procesar el dato que llega en el evento y enviarlo al actuador fisico correspondiente
        recibe_Blynk(event);
        break;

    case MQTT_EVENT_ERROR: //Evento de error
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        //Printeamos el error que llega en el evento
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err); //Tipo de Error 1
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err); //Tipo de Error 2
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno); //Tipo de Error 3
            ESP_LOGI(MQTT_TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno)); //Tipo de Error 4
        }
        break;
        
    default:
        ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
        break;
    }
}

//Funcion para loggear errores
//Si el codigo de error es distinto de 0 (es decir, existe error) se printea el mensaje de error
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(MQTT_TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void inicia_cliente_mqtt(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://ny3.blynk.cloud:1883",
        .credentials.authentication.password= BLYNK_AUTH_TOKEN,
        .credentials.username= BLYNK_USERNAME,
        .credentials.client_id= BLYNK_USERNAME,
        .session.keepalive = 45,
        .session.disable_clean_session = false
    };


    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}







//MUX
void recibe_Blynk(esp_mqtt_event_handle_t event)
{
    //Usamos la funcion strcmp para comparar la cadena de texto que llega en el topic con un string
    //Si ambos son iguales la funcion entrega un 0
    //Es necesario pasarle el tamaño de la cadena de texto que llega en el topic, para evitar el ruido
    if (strncmp(event->topic, "downlink/ds/Switch", event->topic_len) == 0)
    {
        //Ejemplo LED
        gpio_set_direction(GPIO_NUM_32, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_NUM_32, atoi(event->data));

    }
}

//DEMUX
void envia_Blynk(char *id_sensor, char *data){
    //ds/Switch Value
    if (strcmp(id_sensor, "test") == 0)
    {
        esp_mqtt_client_publish(client, "ds/EVENTOS_PROCESO", data, 0, 0, 0);
    }
    else if(strcmp(id_sensor, "temp") == 0)
    {
        esp_mqtt_client_publish(client, "ds/temp", data, 0, 0, 0);

    }
    
}



