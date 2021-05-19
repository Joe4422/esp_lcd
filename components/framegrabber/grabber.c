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
#define MAX_FAILS					(5)

/****************************************************************
 * Local variables
 ****************************************************************/
char regionData[FRAME_BUFFER_SIZE];
bool frameGrabberRunning = false;
bool disconnected = false;
uint8_t fails = 0;

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

bool FrameGrabber_NextWidget()
{
	return FrameGrabber_SendMessage("widget_next");
}

bool FrameGrabber_LastWidget()
{
	return FrameGrabber_SendMessage("widget_last");
}

bool FrameGrabber_WidgetAction()
{
	return FrameGrabber_SendMessage("widget_action");
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

		if (WebClient_Get("widget_get_frame", FRAME_BUFFER_SIZE, regionData) == false)
		{
			if (++fails >= MAX_FAILS)
			{
				gpio_set_level(PIN_NUM_BCKL, PIN_BCKL_OFF);
			}
		}
		else
		{
			fails = 0;
			gpio_set_level(PIN_NUM_BCKL, PIN_BCKL_ON);
			for (i = 0; i < FRAME_HEIGHT; i++)
			{
				TFT_drawFastHLineBuffer(0, i, FRAME_WIDTH, colorData + (FRAME_WIDTH * i));
			}
		}

		vTaskDelay(FRAME_PERIOD_MS / portTICK_PERIOD_MS);
	}
}
