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

// Project includes
#include "../../main/config.h"

/****************************************************************
 * Defines, consts
 ****************************************************************/
#define HEADER_LIFETIME	(MS_TO_TICKS(2000))

/****************************************************************
 * Local variables
 ****************************************************************/
page_t * startPage = NULL;
page_t * endPage = NULL;
page_t * activePage = NULL;

xTaskHandle taskHandle_UpdateActivePage;

uint32_t updateCounter = 0;

bool needsNextPage = false;

uint16_t header_lifetime = HEADER_LIFETIME;

/****************************************************************
 * Function declarations
 ****************************************************************/
void UpdateActivePage();

void DrawHeader(bool fullSize);

void SwitchNextPage();

/****************************************************************
 * Function definitions
 ****************************************************************/
bool Pager_Init()
{
	return true;
}

void Pager_Tick()
{
	if (startPage != NULL && activePage != NULL && endPage != NULL)
	{
		if (needsNextPage == true)
		{
			needsNextPage = false;
			SwitchNextPage();
		}

		if (updateCounter == activePage->update_period)
		{
			ESP_LOGI("Pager", "Updating page %s.", activePage->name);
			activePage->renderPage(false);
			DrawHeader(header_lifetime > 0);
			updateCounter = 0;
		}
		else
		{
			updateCounter++;
			if (header_lifetime > 0)
			{
				header_lifetime--;
				if (header_lifetime == 0)
				{
					activePage->renderPage(true);
					DrawHeader(false);
				}
			}
		}
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
		page->renderPage(true);
		DrawHeader(true);
		header_lifetime = HEADER_LIFETIME;
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
	TFT_fillScreen(TFT_BLACK);
	TFT_setFont(DEFAULT_FONT, NULL);
	_fg = TFT_WHITE;
	_bg = TFT_BLACK;
	TFT_print("Loading...", CENTER, CENTER);
	DrawHeader(true);
	if (activePage->initPage != NULL) activePage->initPage();
	activePage->renderPage(true);
	DrawHeader(true);
	updateCounter = 0;
	header_lifetime = HEADER_LIFETIME;

	ESP_LOGI("Pager", "Setting active page to %s.", activePage->name);
}

void DrawHeader(bool fullSize)
{
	if (fullSize)
	{
		TFT_fillRect(0, 140, 128, 20, activePage->header_bg);
		_bg = activePage->header_bg;
		TFT_setFont(DEFAULT_FONT, NULL);
		_fg = activePage->header_fg;
		TFT_print(activePage->name, CENTER, 144);
	}
	else
	{
		TFT_drawLine(0, 159, 127, 159, activePage->header_bg);
	}
}
