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
#define FRAME_REGION_HEIGHT			(160)
#define FRAME_REGION_COUNT			(FRAME_HEIGHT / FRAME_REGION_HEIGHT)

#define GRABBER_TASK_STACK			(8192)

#define PAGES_LENGTH				(16)
#define PAGE_NAME_LENGTH			(32)

#define BASE_URL					"http://192.168.0.69/"
#define URL_LENGTH					(64)
#define ACTION_RESPONSE_BUFFER_SIZE	(32)
#define PAGE_LIST_BUFFER_SIZE		(512)

#define FRAME_PERIOD_MS				(250)
#define GET_PAGE_LIST_PERIOD_MS		(5000)

/****************************************************************
 * Local variables
 ****************************************************************/
char regionData[FRAME_WIDTH * FRAME_REGION_HEIGHT * 3 + 1];
char pages[PAGES_LENGTH][PAGE_NAME_LENGTH];
uint8_t activePageIndex = 0;
bool frameGrabberRunning = false;
bool disconnected = false;

/****************************************************************
 * Function declarations
 ****************************************************************/
void FrameGrabber_Task(void * pvParameter);

/****************************************************************
 * Function definitions
 ****************************************************************/
bool FrameGrabber_Init()
{
	uint8_t i;
	char pageListBuffer[PAGE_LIST_BUFFER_SIZE];
	char * pageItem = NULL;

	for (i = 0; i < PAGES_LENGTH; i++)
	{
		pages[i][0] = '\0';
	}

	TFT_fillScreen(TFT_BLACK);
	_fg = TFT_WHITE;
	_fg = TFT_BLACK;
	TFT_print("Connecting...", CENTER, CENTER);

	while (WebClient_Get(BASE_URL "list_pages/", 0, NULL, PAGE_LIST_BUFFER_SIZE, pageListBuffer) == false)
	{
		vTaskDelay(GET_PAGE_LIST_PERIOD_MS / portTICK_PERIOD_MS);
	}
	pageItem = strtok(pageListBuffer, "\n");
	while (pageItem != NULL)
	{
		ESP_LOGI("FrameGrabber", "Added page %s.", pageItem);
		FrameGrabber_AddPage(pageItem);
		pageItem = strtok(NULL, "\n");
	}

	return true;
}

bool FrameGrabber_Run()
{
	xTaskCreate(FrameGrabber_Task, "FrameGrabber_Task", GRABBER_TASK_STACK, NULL, 10, NULL);
	frameGrabberRunning = true;
	return true;
}

bool FrameGrabber_AddPage(char * page)
{
	uint8_t i;

	if (frameGrabberRunning) return false;

	for (i = 0; i < PAGES_LENGTH; i++)
	{
		if (pages[i][0] == '\0')
		{
			strcpy(pages[i], page);
			return true;
		}
	}

	return false;
}

bool FrameGrabber_NextPage()
{
	uint8_t i;
	bool foundPage = false;

	if (frameGrabberRunning == false) return false;

	for (i = activePageIndex + 1; i < PAGES_LENGTH; i++)
	{
		if (pages[i][0] != '\0')
		{
			activePageIndex = i;
			foundPage = true;
			break;
		}
	}

	if (foundPage == false)
	{
		for (i = 0; i < PAGES_LENGTH; i++)
		{
			if (pages[i][0] != '\0')
			{
				activePageIndex = i;
				foundPage = true;
				break;
			}
		}
	}

	return foundPage;
}

bool FrameGrabber_PageAction()
{
	char url[URL_LENGTH];
	char buffer[ACTION_RESPONSE_BUFFER_SIZE];

	sprintf(url, BASE_URL "%s?action", pages[activePageIndex]);
	if (WebClient_Get(url, 0, NULL, ACTION_RESPONSE_BUFFER_SIZE, buffer) == false)
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
		if (pages[activePageIndex][0] != '\0')
		{
			uint8_t i;
			color_t * colorData = &regionData;
			char url[URL_LENGTH];

			for (i = 0; i < FRAME_REGION_COUNT; i++)
			{
				uint8_t j;

				sprintf(url, BASE_URL "%s?region=%d", pages[activePageIndex], i);
				if (WebClient_Get(url, 0, NULL, FRAME_WIDTH * FRAME_REGION_HEIGHT * 3 + 1, regionData) == false)
				{
					// Set all black if request fails
					memset(regionData, 0, FRAME_WIDTH * FRAME_REGION_HEIGHT * 3);
					disconnected = true;
				}
				else
				{
					disconnected = false;
				}

				if (regionData[0] == 'N')
					ESP_LOGI("blah", "%s", regionData);
				if (memcmp("Needs Refresh", regionData, 13) == 0)
				{
					ESP_LOGI("blah", "Full Refresh going on");
					i = 255;
					continue;
				}

				for (j = 0; j < FRAME_REGION_HEIGHT; j++)
				{
					TFT_drawFastHLineBuffer(0, i * FRAME_REGION_HEIGHT + j, FRAME_WIDTH, colorData + (FRAME_WIDTH * j));
				}
				if (disconnected)
				{
					_fg = TFT_WHITE;
					_bg = TFT_BLACK;
					TFT_print("Disconnected", CENTER, CENTER);
				}
			}
		}
		vTaskDelay(FRAME_PERIOD_MS / portTICK_PERIOD_MS);
	}
}
