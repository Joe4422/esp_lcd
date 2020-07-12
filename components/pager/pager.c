/****************************************************************
 * Includes
 ****************************************************************/
#include "pager.h"

// cstdlib includes
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP-IDF includes
#include "esp_log.h"
#include "sdkconfig.h"

/****************************************************************
 * Defines, consts
 ****************************************************************/
#define DELAY_INTERVAL_MS				(10)

/****************************************************************
 * Local variables
 ****************************************************************/
page_t * startPage = NULL;
page_t * endPage = NULL;
page_t * activePage = NULL;

xTaskHandle taskHandle_UpdateActivePage;

uint8_t updateCounter = 0;

bool needsNextPage = false;

/****************************************************************
 * Function declarations
 ****************************************************************/
void UpdateActivePage();

void DrawHeader();

void SwitchNextPage();

/****************************************************************
 * Function definitions
 ****************************************************************/
bool Pager_Init()
{
	return true;
}

bool Pager_StartLoop()
{
	while (1)
	{
		UpdateActivePage();
	}
}

bool Pager_AddPage(page_t * page)
{
	ESP_LOGI("Pager", "Adding page %s...", page->name);

	if (startPage == NULL || activePage == NULL || endPage == NULL)
	{
		// Set startPage, endPage and activePage to page
		startPage = page;
		activePage = page;
		endPage = page;

		startPage->nextPage = page;
		page->initPage();
		page->renderPage();
		DrawHeader();
	}
	else
	{
		// Say we've got pages [X] and {Z}, where [?] is endPage and {?} is startPage, and we want to insert Y after endPage
		// Currently, [X] -> {Z}
		// We want X -> [Y] -> {Z}

		// Change [X] -> {Z} to [X] -> Y
		endPage->nextPage = page;

		// Change [X] -> Y to X -> [Y]
		endPage = page;
		// Change X -> [Y] to X -> [Y] -> {Z}
		page->nextPage = startPage;
	}

	ESP_LOGI("Pager", "Page %s added.", page->name);
	return true;
}

bool Pager_NextPage()
{
	if (activePage->nextPage == NULL)
	{
		ESP_LOGE("Pager", "Next page was null so couldn't switch active!");
		return false;
	}
	else
	{
		needsNextPage = true;
		return true;
	}
}

void SwitchNextPage()
{
	if (activePage->deInitPage != NULL) activePage->deInitPage();
	activePage = activePage->nextPage;
	if (activePage->initPage != NULL) activePage->initPage();
	activePage->renderPage();
	DrawHeader();
	updateCounter = 0;

	ESP_LOGI("Pager", "Setting active page to %s.", activePage->name);
}

void UpdateActivePage()
{
	if (startPage != NULL && activePage != NULL && endPage != NULL)
	{
		if (needsNextPage == true)
		{
			needsNextPage = false;
			SwitchNextPage();
		}

		if (updateCounter == activePage->update_period_s)
		{
			ESP_LOGI("Pager", "Updating page %s.", activePage->name);
			activePage->renderPage();
			DrawHeader();
			updateCounter = 0;
		}
		else
		{
			updateCounter++;
		}
	}
	vTaskDelay(DELAY_INTERVAL_MS * portTICK_PERIOD_MS);
}

void DrawHeader()
{
	TFT_fillRect(0, 140, 128, 20, activePage->theme);
	_bg = activePage->theme;
	TFT_setFont(DEFAULT_FONT, NULL);
	_fg = TFT_WHITE;
	TFT_print(activePage->name, CENTER, 144);
}
