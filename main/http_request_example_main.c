#include <stdio.h>
#include <string.h>
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define WIFI_SSID "Altan"
#define WIFI_PASS "123456789"

#define FIREBASE_CURRENT "Current"
#define FIREBASE_LED "LED"

#define FIREBASE_READKEY "State"
#define FIREBASE_WRITEKEY "currentValue"

#define LED_GPIO_PIN GPIO_NUM_2

#define FIREBASE_AUTH "AIzaSyB0k6nIpVbgyxQX_lKUKd8F6ynwlPmtKds"

#define FIREBASE_URL_WRITE "https://smartsocket-97183-default-rtdb.firebaseio.com/" FIREBASE_CURRENT "/.json?auth=" FIREBASE_AUTH
#define FIREBASE_URL_READ "https://smartsocket-97183-default-rtdb.firebaseio.com/" FIREBASE_LED "/" FIREBASE_READKEY ".json?auth=" FIREBASE_AUTH

extern const uint8_t gtsr1_pem_start[] asm("_binary_gtsr1_pem_start");
extern const uint8_t gtsr1_pem_end[] asm("_binary_gtsr1_pem_end");

static const char *TAG = "FirebaseExample";

const int WIFI_CONNECTED_BIT = BIT0;
static EventGroupHandle_t wifi_event_group;

int state_value = 0;
int adc_value = 0;

// LED Control
void control_led(int state)
{
    gpio_set_level(LED_GPIO_PIN, state);
    ESP_LOGI(TAG, "LED %s", state ? "ON" : "OFF");
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        esp_ip4_addr_t *ip = &((ip_event_got_ip_t *)event_data)->ip_info.ip;
        ESP_LOGI(TAG, "Got IP: %s", ip4addr_ntoa(ip));
    }
}

void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

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

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE,
                                           pdTRUE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    }
    else
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password);
    }
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

            state_value = atoi(evt->data);
            ESP_LOGI(TAG, "Read state value: %d", state_value);

            printf("%.*s", evt->data_len, (char *)evt->data); // Print data to the console
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}
uint8_t current = 5;

static void write_owndata_to_firebase(void *pvParameter)
{
    while (1)
    {
        adc_value = adc1_get_raw(ADC1_CHANNEL_0);
        printf("ADC value is %d\n", adc_value);

        esp_http_client_config_t config = {
            .url = FIREBASE_URL_WRITE,
            .event_handler = _http_event_handler,
            .cert_pem = (char *)gtsr1_pem_start,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);

        esp_http_client_set_url(client, FIREBASE_URL_WRITE);
        esp_http_client_set_method(client, HTTP_METHOD_PUT);

        char put_data[200];
        snprintf(put_data, sizeof(put_data), "{\"Value\": \"%d\"}", adc_value);
        esp_http_client_set_post_field(client, put_data, strlen(put_data));

        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
                     esp_http_client_get_status_code(client),
                     esp_http_client_get_content_length(client));
        }
        else
        {
            ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void read_data_from_firebase(void *pvParameter)
{

    while (1)
    {
        esp_http_client_config_t config = {
            .url = FIREBASE_URL_READ,
            .event_handler = _http_event_handler,
            .cert_pem = (char *)gtsr1_pem_start,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                     esp_http_client_get_status_code(client),
                     esp_http_client_get_content_length(client));

            control_led(state_value);
        }
        else
        {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_12);

    wifi_init_sta();

    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);

    xTaskCreate(&write_owndata_to_firebase, "write_owndata_to_firebase", 8192, NULL, 5, NULL);

    xTaskCreate(&read_data_from_firebase, "read_data_from_firebase", 8192, NULL, 4, NULL);
}
