/****************************************************************
 * Includes
 ****************************************************************/
// cstdlib includes
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// ESP-IDF includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"

/****************************************************************
 * Defines, consts
 ****************************************************************/
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/****************************************************************
 * Local variables
 ****************************************************************/
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

const char * WIFI_LOG_TAG = "WiFi";

static int max_retries = 10;
static int retry_num = 0;


/****************************************************************
 * Function definitions
 ****************************************************************/
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_num < max_retries) {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(WIFI_LOG_TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_LOG_TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_LOG_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

bool WiFi_Init(const char * ssid, const char * password, int max_retry)
{
	esp_err_t ret;
	max_retries = max_retry;

	ESP_LOGI(WIFI_LOG_TAG, "Beginning to initialise WiFi...");

	// Create event group for WiFi
    wifi_event_group = xEventGroupCreate();

    // Initialise TCP/IP adapter
    tcpip_adapter_init();

    // Attempt to initialise ESP event loop
    ret = esp_event_loop_create_default();
    // Check if ESP event loop init succeeded
    if (ret != ESP_OK)
    {
    	// Init failed; return false
    	ESP_LOGE(WIFI_LOG_TAG, "ESP event loop init failed!");
    	return false;
    }

    // Get default WiFi init configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    // Attempt to init WiFi adapter
    ret = esp_wifi_init(&cfg);
    // Check if WiFi adapter init succeeded
    if (ret != ESP_OK)
    {
    	// Init failed; return false
    	ESP_LOGE(WIFI_LOG_TAG, "ESP WiFi init failed! (0x%04X)", ret);
    	return false;
    }

    // Attempt to register WiFi event handler
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    // Check if event handler register succeed
    if (ret != ESP_OK)
    {
    	// Register failed; return false
    	ESP_LOGE(WIFI_LOG_TAG, "WiFi event handler registration failed!");
    	return false;
    }
    // Attempt to register IP event handler
    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
    // Check if event handler register succeeded
    if (ret != ESP_OK)
	{
    	// Register failed; return false
    	ESP_LOGE(WIFI_LOG_TAG, "IP event handler registration failed!");
		return false;
	}

    // Initialise WiFi configuration
    wifi_config_t wifi_config = {
        .sta = {
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    // Copy WiFi SSID and password to wifi_config
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);

    // Attempt to set WiFi mode to Station
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    // Check if WiFi mode set succeeded
    if (ret != ESP_OK)
	{
    	// Mode set failed; return false
    	ESP_LOGE(WIFI_LOG_TAG, "Setting WiFi mode to Station failed!");
		return false;
	}
    // Attempt to set WiFi configuration
    ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    // Check if WiFi configuration set succeeded
    if (ret != ESP_OK)
	{
    	// Setting configuration failed; return false
    	ESP_LOGE(WIFI_LOG_TAG, "Setting WiFi configuration failed!");
		return false;
	}
    // Attempt to start WiFi adapter
    ret = esp_wifi_start();
    // Check if WiFi adapter start succeeded
    if (ret != ESP_OK)
	{
    	// WiFi adapter start failed; return false
    	ESP_LOGE(WIFI_LOG_TAG, "Starting WiFi adapter failed!");
		return false;
	}

    ESP_LOGI(WIFI_LOG_TAG, "WiFi initialisation finished; attempting to connect...");

    // Attempts to connect, and waits until either the connection succeeds or the maximum number of retries is met.
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    // Attempt to unregister IP handler
    ret = esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
    // Check if unregistration succeeded
    if (ret != ESP_OK)
	{
    	// Unregistration failed; return false
    	ESP_LOGE(WIFI_LOG_TAG, "IP handler unregistration failed!");
		return false;
	}
    // Attempt to unregister WiFi handler
    ret = esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
    // Check if unregistration succeeded
    if (ret != ESP_OK)
	{
    	// Unregistration failed; return false
    	ESP_LOGE(WIFI_LOG_TAG, "WiFi handler unregistration failed!");
		return false;
	}
    // Deletes WiFi event group
    vEventGroupDelete(wifi_event_group);

    // Check the outcome of the WiFi connection
    if (bits & WIFI_CONNECTED_BIT)
    {
    	// Connection succeeded; return true
        ESP_LOGI(WIFI_LOG_TAG, "Connection to SSID %s succeeded.", ssid);
        return true;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
    	// Connection failed; return false
        ESP_LOGE(WIFI_LOG_TAG, "Connection to SSID %s timed out!", ssid);
        return false;
    }
    else
    {
    	// Unexpected error; return false
        ESP_LOGE(WIFI_LOG_TAG, "Unexpected error during WiFi connection!");
        return false;
    }


}
