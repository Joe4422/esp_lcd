/****************************************************************
 * Includes
 ****************************************************************/
#include "page_template.h"

// ESP-IDF includes
#include "esp_log.h"
#include "sdkconfig.h"

// TFT includes
#include "tft/tft.h"

// Project includes
#include "../../main/config.h"

/****************************************************************
 * Function declarations
 ****************************************************************/
bool Page_Template_InitPage();
bool Page_Template_DeInitPage();
void Page_Template_RenderPage(bool force);

/****************************************************************
 * Global variables
 ****************************************************************/
page_t PAGE_TEMPLATE =
{
	.nextPage =			NULL,
	.name =				"Template",
	.initPage =			Page_Template_InitPage,
	.deInitPage =		Page_Template_DeInitPage,
	.renderPage =		Page_Template_RenderPage,
	.header_fg =		(color_t){ 0, 0, 0 },
	.header_bg =		(color_t){ 255, 255, 255 },
	.update_period =	MS_TO_TICKS(60000)
};

/****************************************************************
 * Function definitions
 ****************************************************************/
bool Page_Template_InitPage()
{
	return true;
}

bool Page_Template_DeInitPage()
{
	return true;
}

void Page_Template_RenderPage(bool force)
{
	TFT_fillScreen(TFT_BLACK);
	_fg = TFT_WHITE;
	_bg = TFT_BLACK;
	TFT_setFont(DEFAULT_FONT, NULL);
	TFT_print("Template Page", CENTER, CENTER);
	TFT_setFont(DEJAVU24_FONT, NULL);
	TFT_print("[|:)", CENTER, 100);
}
