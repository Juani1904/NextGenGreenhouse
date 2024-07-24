idf_component_register(SRCS "Blynk_MQTT_Connect.c"
                        INCLUDE_DIRS "include"
                        REQUIRES "mqtt" "driver" "esp_wifi" "nvs_flash" "Control_temperatura" "Control_humedad")