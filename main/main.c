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

// WEBCLIENT includes
#include "webclient/client.h"

// FRAMEGRABBER includes
#include "framegrabber/grabber.h"

// Project includes
#include "config.h"
#include "wifi_login.h"
#include "button.h"

/****************************************************************
 * Defines, consts
 ****************************************************************/
#define GPIO_BUTTON_PIN				(12)

#define LONG_PRESS_LENGTH			(400 / portTICK_PERIOD_MS)
#define TAP_TIMEOUT					(600 / portTICK_PERIOD_MS)


/****************************************************************
 * Local variables
 ****************************************************************/
uint32_t lastButtonPressTime = 0;
uint32_t holdStartTime = 0;
uint8_t tapCount = 0;

/****************************************************************
 * Function declarations
 ****************************************************************/
bool TFT_Init();

/****************************************************************
 * Function definitions
 ****************************************************************/
void app_main()
{
	button_event_t buttonEvent;

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
	if (WebClient_Init("192.168.0.69") == false)
	{
		ESP_LOGE("Startup", "Web client initialisation failed!");
		while (1) vTaskDelay(portTICK_PERIOD_MS);
	}
	ESP_LOGI("Startup", "Web client initialised.");

	ESP_LOGI("Startup", "Initialising frame grabber...");
	if (FrameGrabber_Init() == false)
	{
		ESP_LOGE("Startup", "Web client initialisation failed!");
		while (1) vTaskDelay(portTICK_PERIOD_MS);
	}
	ESP_LOGI("Startup", "Frame grabber initialised.");

	ESP_LOGI("Startup", "Initialising buttons...");
	QueueHandle_t button_events = button_init(PIN_BIT(GPIO_BUTTON_PIN));
	ESP_LOGI("Startup", "Buttons initialised.");

	ESP_LOGI("Startup", "Starting frame grabber task...");
	if (FrameGrabber_Run() == false)
	{
		ESP_LOGI("Startup", "Frame grabber task start failed.");
		while (1) vTaskDelay(portTICK_PERIOD_MS);
	}
	ESP_LOGI("Startup", "Frame grabber task started.");

	ESP_LOGI("Startup", "Starting main loop...");

	while (1)
	{
		uint32_t ticks = xTaskGetTickCount();
		if (xQueueReceive(button_events, &buttonEvent, 0))
		{
			if (buttonEvent.pin == GPIO_BUTTON_PIN)
			{
				if (buttonEvent.event == BUTTON_DOWN)
				{
					holdStartTime = buttonEvent.timestamp;
				}
				else if (buttonEvent.event == BUTTON_UP)
				{
					if (buttonEvent.timestamp >= (holdStartTime + LONG_PRESS_LENGTH))
					{
						// Long press
						FrameGrabber_PageAction();
						ESP_LOGI("Buttons", "Registered long press. Hold started at %d, finished at %d.", holdStartTime, buttonEvent.timestamp);
					}
					else if ((tapCount == 0) || (buttonEvent.timestamp < (lastButtonPressTime + TAP_TIMEOUT)))
					{
						// Register tap
						tapCount++;
						ESP_LOGI("Buttons", "Registered tap number %d. Last press at %d, this press at %d.", tapCount, lastButtonPressTime, buttonEvent.timestamp);
						lastButtonPressTime = buttonEvent.timestamp;
					}
				}

			}
		}
		if (tapCount > 0 && ticks >= (lastButtonPressTime + TAP_TIMEOUT))
		{
			ESP_LOGI("Buttons", "Fulfilling tap count %d. %d >= %d.", tapCount, ticks, lastButtonPressTime + TAP_TIMEOUT);
			switch (tapCount)
			{
				case 1:
					FrameGrabber_NextPage();
					break;
				case 2:
					FrameGrabber_LastPage();
					break;
				default:
					break;
			}
			tapCount = 0;
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
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
