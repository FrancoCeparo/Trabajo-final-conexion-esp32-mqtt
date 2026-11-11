
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// CONFIGURACION
/////////////////////////

#define LED GPIO_NUM_2
#define PULSADOR GPIO_NUM_4

#define WIFI_SSID     "megainternet"
#define WIFI_PASSWORD "epa"

#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t wifi_event_group;
static const char *TAG = "ESP32";

// CONFIGURACION MQTT
//////////////////////////

#define MQTT_BROKER "mqtt://122.198.09.3:1873"
#define MQTT_USER "quelo"
#define MQTT_PASSWORD "sabe"
#define NODO_ID 1

#define TOPIC_GENERAL "/ej02/cmd"
#define TOPIC_ID      "/ej02/id"
#define TOPIC_SENSOR  "/ej02/1/sensor"
#define TOPIC_LED     "/ej02/1/cmd"

static volatile bool mqtt_conectado = false;
static esp_mqtt_client_handle_t clienteMQTT;

//  MQTT
///////////////////////////////

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                             int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event =
        (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado al broker MQTT");

            mqtt_conectado = true;

            esp_mqtt_client_subscribe(
                event->client, TOPIC_GENERAL, 1);

            esp_mqtt_client_subscribe(
                event->client, TOPIC_LED, 1);

            ESP_LOGI(TAG, "Suscripciones realizadas");
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqtt_conectado = false;
            ESP_LOGW(TAG, "Desconectado del broker");
            break;

        case MQTT_EVENT_DATA:
{
    ESP_LOGI(TAG, "TOPICO: %.*s",
             event->topic_len, event->topic);

    ESP_LOGI(TAG, "MENSAJE: %.*s",
             event->data_len, event->data);

    if (event->topic_len == strlen(TOPIC_GENERAL) &&
        memcmp(event->topic, TOPIC_GENERAL,
               event->topic_len) == 0)
    {
        if (event->data_len == 5 &&
            memcmp(event->data, "getId", 5) == 0)
        {
            char id[5];
            sprintf(id, "%d", NODO_ID);

            esp_mqtt_client_publish(
                event->client, TOPIC_ID, id, 0, 1, 0
            );

            ESP_LOGI(TAG, "ID publicado: %s", id);
        }
    }

    if (event->topic_len == strlen(TOPIC_LED) &&
        memcmp(event->topic, TOPIC_LED,
               event->topic_len) == 0)
    {
        if (event->data_len == 5 &&
            memcmp(event->data, "ledOn", 5) == 0)
        {
            gpio_set_level(LED, 1);
            ESP_LOGI(TAG, "LED ENCENDIDO");
        }
        else if (event->data_len == 6 &&
                 memcmp(event->data, "ledOff", 6) == 0)
        {
            gpio_set_level(LED, 0);
            ESP_LOGI(TAG, "LED APAGADO");
        }
    }

    break;
}

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Error en MQTT");
            break;

        default:
            break;
    }
}
// CONEXION MQTT
////////////////////////////////////

void conectarMQTT(void)
{
    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = MQTT_BROKER,
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASSWORD,
        .network.timeout_ms = 10000
    };

    clienteMQTT = esp_mqtt_client_init(&mqtt_config);

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        clienteMQTT,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    ));

    ESP_ERROR_CHECK(esp_mqtt_client_start(clienteMQTT));
}

void publicarSensor(void *param)
{
    char mensaje[10];

    while (1)
    {
        if (mqtt_conectado)
        {
            int estado = gpio_get_level(PULSADOR);

            sprintf(mensaje, "%d", estado);

            int resultado = esp_mqtt_client_publish(
                clienteMQTT,
                TOPIC_SENSOR,
                mensaje,
                0,
                1,
                0
            );

            if (resultado >= 0)
            {
                ESP_LOGI(TAG, "Sensor publicado: %s", mensaje);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

// EVENTOS WIFI
////////////////////////////////

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WiFi desconectado. Reintentando...");

        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);

        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "Conectado. IP: " IPSTR,
                 IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// CONEXION WIFI
/////////////////////////////

void conectarWiFi(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL
    ));

    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL
    ));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_set_config(
        WIFI_IF_STA,
        &wifi_config
    ));

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Esperando conexion WiFi...");

    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );
}

// CONFIGURACION GPIO
///////////////////////

void configurarGPIO(void)
{
    gpio_config_t configLed = {
        .pin_bit_mask = 1ULL << LED,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&configLed));
    gpio_set_level(LED, 0);

    gpio_config_t configPulsador = {
        .pin_bit_mask = 1ULL << PULSADOR,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&configPulsador));
}

// PROGRAMA PRINCIPAL
//////////////------------------------------
void app_main(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    // Configuramos los GPIO
    ///////////////////

    configurarGPIO();

    // Prueba del LED
    ///////////////////

    ESP_LOGI(TAG, "Probando GPIO 2");

    gpio_set_level(LED, 1);
    vTaskDelay(pdMS_TO_TICKS(3000));
    gpio_set_level(LED, 0);

    // Conectamos WiFi
    //////////////////////////

    conectarWiFi();

    // Conectamos MQTT
    /////////////////////

    ESP_LOGI(TAG, "Iniciando MQTTlerdo...");
    conectarMQTT();

    /////// creamos tarea del sensor
    ///////////////////////

    xTaskCreate(
        publicarSensor,
        "Publicar Sensor",
        4096,
        NULL,
        1,
        NULL
    );

    ESP_LOGI(TAG, "cliente MQTT iniciado");

    while (1)
    {
        int estado = gpio_get_level(PULSADOR);

        printf("estado del pulsador: %d\n", estado);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
