/****************************************************************
 * Includes
 ****************************************************************/
#include "grabber.h"

// cstdlib includes
#include <stdint.h>
#include <string.h>

// TFT includes
#include "tft/tft.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// WEBCLIENT includes
#include "webclient/client.h"

// ESP-IDF includes
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

/****************************************************************
 * Defines, consts
 ****************************************************************/
#define FRAME_WIDTH					(128)
#define FRAME_HEIGHT				(160)
#define BYTES_PER_PIXEL				(3)
#define FRAME_BUFFER_SIZE			(FRAME_WIDTH * FRAME_HEIGHT * BYTES_PER_PIXEL)

#define GRABBER_TASK_STACK			(8192)

#define RESPONSE_BUFFER_SIZE		(32)

#define FRAME_PERIOD_MS				(40)

/****************************************************************
 * Local variables
 ****************************************************************/
char regionData[FRAME_BUFFER_SIZE];
uint8_t activePageIndex = 0;
bool frameGrabberRunning = false;
bool disconnected = false;

/****************************************************************
 * Function declarations
 ****************************************************************/
void FrameGrabber_Task(void * pvParameter);

bool FrameGrabber_SendMessage(char * message);

/****************************************************************
 * Function definitions
 ****************************************************************/
bool FrameGrabber_Init()
{
	TFT_fillScreen(TFT_BLACK);
	_fg = TFT_WHITE;
	_fg = TFT_BLACK;
	TFT_print("Connecting...", CENTER, CENTER);

	return true;
}

bool FrameGrabber_Run()
{
	xTaskCreate(FrameGrabber_Task, "FrameGrabber_Task", GRABBER_TASK_STACK, NULL, 10, NULL);
	frameGrabberRunning = true;
	return true;
}

bool FrameGrabber_NextPage()
{
	return FrameGrabber_SendMessage("page_next");
}

bool FrameGrabber_LastPage()
{
	return FrameGrabber_SendMessage("page_last");
}

bool FrameGrabber_PageAction()
{
	return FrameGrabber_SendMessage("page_action");
}

bool FrameGrabber_SendMessage(char * message)
{
	char buffer[RESPONSE_BUFFER_SIZE];

	if (WebClient_Get(message, RESPONSE_BUFFER_SIZE, buffer) == false)
	{
		return false;
	}
	else if (strcmp("Failure", buffer) == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void FrameGrabber_Task(void * pvParameter)
{
	while (1)
	{
		uint8_t i;
		color_t * colorData = &regionData;

		//ESP_LOGI("Grabber", "Grabbing data.");
		if (WebClient_Get("page_get_frame", FRAME_BUFFER_SIZE, regionData) == false)
		{
			// Set all black if request fails
			memset(regionData, 0, FRAME_BUFFER_SIZE);
			disconnected = true;
		}
		else
		{
			disconnected = false;
		}
		//ESP_LOGI("Grabber", "Grabbed data. Blitting page.");

		for (i = 0; i < FRAME_HEIGHT; i++)
		{
			TFT_drawFastHLineBuffer(0, i, FRAME_WIDTH, colorData + (FRAME_WIDTH * i));
		}
		if (disconnected)
		{
			_fg = TFT_WHITE;
			_bg = TFT_BLACK;
			TFT_print("Disconnected", CENTER, CENTER);
		}
		//ESP_LOGI("Grabber", "Blitted page.");

		vTaskDelay(FRAME_PERIOD_MS / portTICK_PERIOD_MS);
	}
}
