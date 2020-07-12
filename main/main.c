/****************************************************************
 * Includes
 ****************************************************************/
// cstdlib includes
#include <stdint.h>
#include <stdbool.h>

// ESP-IDF includes
#include "driver/gpio.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// TFT includes
#include "tft/tftspi.h"
#include "tft/tft.h"

// WIFI includes
#include "wifi_manager.h"

// Storage includes
#include "storage/storage_manager.h"

// Pager includes
#include "pager/pager.h"
#include "pager/page_spotify.h"

// WEB_CLIENT includes
#include "web_client/client.h"

// Project includes
#include "config.h"
#include "wifi_login.h"

/****************************************************************
 * Defines, consts
 ****************************************************************/
const uint8_t GPIO_INTERRUPT_PIN = 12;

/****************************************************************
 * Function declarations
 ****************************************************************/
bool TFT_Init();

bool GPIO_Init();

void GPIO_Interrupt_Handler(void * ptr);

/****************************************************************
 * Function definitions
 ****************************************************************/
void app_main()
{
	// Initialise display
	ESP_LOGI("Startup", "Initialising display...");
	if (TFT_Init() == false)
	{
		ESP_LOGE("Startup", "Display initialisation failed!");
		while (1) vTaskDelay(portTICK_PERIOD_MS);
	}
	ESP_LOGI("Startup", "Display initialised.");

	// Initialise storage
	ESP_LOGI("Startup", "Initialising storage...");
	if (Storage_Init() == false)
	{
		ESP_LOGE("Startup", "Storage initialisation failed!");
		while (1) vTaskDelay(portTICK_PERIOD_MS);
	}
	ESP_LOGI("Startup", "Storage initialised.");

	// Initialise WiFi
	ESP_LOGI("Startup", "Initialising WiFi...");

	TFT_fillScreen(TFT_BLACK);
	TFT_print("Starting WiFi...", CENTER, CENTER);

	if (WiFi_Init(WIFI_SSID, WIFI_PASSWORD, WIFI_MAX_RETRIES) == false)
	{
		TFT_fillScreen(TFT_BLACK);
		_fg = TFT_RED;
		TFT_print("WiFi failed!", CENTER, CENTER);

		ESP_LOGE("Startup", "WiFi initialisation failed!");

		while (1) vTaskDelay(portTICK_PERIOD_MS);
	}
	else
	{
		TFT_fillScreen(TFT_BLACK);
		_fg = TFT_GREEN;
		TFT_print("WiFi succeeded!", CENTER, CENTER);
		TFT_print("(" WIFI_SSID ")", CENTER, BOTTOM);

		ESP_LOGI("Startup", "WiFi initialised.");
	}


	ESP_LOGI("Startup", "Initialising web client...");
	if (WebClient_Init() == false)
	{
		ESP_LOGE("Startup", "Web client initialisation failed!");
		while (1) vTaskDelay(portTICK_PERIOD_MS);
	}
	ESP_LOGI("Startup", "Web client initialised.");

	ESP_LOGI("Startup", "Initialising pager...");
	if (WebClient_Init() == false || Pager_AddPage(&PAGE_SPOTIFY) == false)
	{
		ESP_LOGE("Startup", "Pager initialisation failed!");
		while (1) vTaskDelay(portTICK_PERIOD_MS);
	}
	ESP_LOGI("Startup", "Pager initialised.");

	ESP_LOGI("Startup", "Initialising GPIO interrupt...");
	if (GPIO_Init() == false)
	{
		ESP_LOGE("Startup", "GPIO interrupt initialisation failed!");
		while (1) vTaskDelay(portTICK_PERIOD_MS);
	}
	ESP_LOGI("Startup", "GPIO interrupt initialised.");

	ESP_LOGI("Startup", "Starting pager loop...");
	Pager_StartLoop();
}

bool TFT_Init()
{
	esp_err_t ret;

	// Display init
	tft_disp_type = DEFAULT_DISP_TYPE;
	_width = DEFAULT_TFT_DISPLAY_WIDTH;
	_height = DEFAULT_TFT_DISPLAY_HEIGHT;
	max_rdclock = 8000000;
	TFT_PinsInit();

	// SPI init
	spi_lobo_device_handle_t spi;
	spi_lobo_bus_config_t buscfg =
	{
		.miso_io_num = PIN_NUM_MISO,
		.mosi_io_num = PIN_NUM_MOSI,
		.sclk_io_num = PIN_NUM_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 6 * 1024
	};
	spi_lobo_device_interface_config_t devcfg =
	{
		.clock_speed_hz = 8000000,
		.mode = 0,
		.spics_io_num = -1,
		.spics_ext_io_num = PIN_NUM_CS,
		.flags = LB_SPI_DEVICE_HALFDUPLEX
	};
	ret = spi_lobo_bus_add_device(TFT_HSPI_HOST, &buscfg, &devcfg, &spi);
	if (ret != ESP_OK) return false;
	disp_spi = spi;

	// Further display init
	TFT_display_init();
	spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);

	TFT_setclipwin(2, 1, 130, 160);
	TFT_setFont(DEFAULT_FONT, NULL);
	_fg = TFT_WHITE;

	return true;
}

bool GPIO_Init()
{
	gpio_set_direction(GPIO_INTERRUPT_PIN, GPIO_MODE_INPUT);
	gpio_set_intr_type(GPIO_INTERRUPT_PIN, GPIO_INTR_HIGH_LEVEL);
	gpio_intr_enable(GPIO_INTERRUPT_PIN);
	gpio_install_isr_service(0);
	gpio_isr_handler_add(GPIO_INTERRUPT_PIN, GPIO_Interrupt_Handler, (void*) &GPIO_INTERRUPT_PIN);

	return true;
}

void GPIO_Interrupt_Handler(void * ptr)
{
	Pager_NextPage();
}
