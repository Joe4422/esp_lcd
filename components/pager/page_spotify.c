/****************************************************************
 * Includes
 ****************************************************************/
#include "page_spotify.h"

// cstdlib includes
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

// SPOTIFY includes
#include "spotify/spotify.h"
#include "spotify/spotify_tokens.h"

// WEB_CLIENT includes
#include "web_client/client.h"

// ESP-IDF includes
#include "esp_log.h"
#include "sdkconfig.h"

// TFT includes
#include "tft/tft.h"

// Project includes
#include "../../main/config.h"

/****************************************************************
 * Defines, consts
 ****************************************************************/
#define ART_BUFFER_SIZE	(100000)
#define ART_DIMS		(75)
#define ART_X			(27)
#define ART_Y			(44)

#define BACKGROUND		(TFT_BLACK)
#define FOREGROUND		(TFT_WHITE)

#define TRACK_INFO_BUFFER_SIZE	(128)

/****************************************************************
 * Function declarations
 ****************************************************************/
bool Page_Spotify_InitPage();
bool Page_Spotify_DeInitPage();
void Page_Spotify_RenderPage(bool force);
void Page_Spotify_Action();

void RenderDisplay();

/****************************************************************
 * Global variables
 ****************************************************************/
page_t PAGE_SPOTIFY =
{
	.nextPage = NULL,
	.name = "Spotify",
	.initPage = Page_Spotify_InitPage,
	.deInitPage = Page_Spotify_DeInitPage,
	.renderPage = Page_Spotify_RenderPage,
	.action = Page_Spotify_Action,
	.header_fg = (color_t){ 255, 255, 255 },
	.header_bg = (color_t){ 30, 215, 96 },
	.update_period = MS_TO_TICKS(5000)
};

/****************************************************************
 * Local variables
 ****************************************************************/
static song_t currentSong;
static char * artBuffer;
static bool showArtLarge = false;

/****************************************************************
 * Function definitions
 ****************************************************************/
bool Page_Spotify_InitPage()
{
	if (Spotify_Init(SPOTIFY_REFRESH_TOKEN, SPOTIFY_CLIENT_TOKEN) == false) return false;

	artBuffer = malloc(sizeof(char) * ART_BUFFER_SIZE);
	if (artBuffer == NULL) return false;

	return true;
}

bool Page_Spotify_DeInitPage()
{
	free(artBuffer);

	currentSong.album[0] = '\0';
	currentSong.art_url[0] = '\0';
	currentSong.artist[0] = '\0';
	currentSong.name[0] = '\0';

	return true;
}

void Page_Spotify_RenderPage(bool force)
{
	song_t requestSong;
	bool refreshArt = true;

	ESP_LOGI("Page_Spotify", "Refreshing Spotify page.");

	if (Spotify_GetCurrentSong(&requestSong) == false)
	{
		ESP_LOGE("Page_Spotify", "Could not get current song!");
		return;
	}

	if (!force)
	{
		if (strcmp(requestSong.album, currentSong.album) == 0 &&
			strcmp(requestSong.art_url, currentSong.art_url) == 0 &&
			strcmp(requestSong.artist, currentSong.artist) == 0 &&
			strcmp(requestSong.name, currentSong.name) == 0 &&
			requestSong.isPlaying == currentSong.isPlaying)
		{
			ESP_LOGI("Page_Spotify", "Page does not need to update.");
			return;
		}
	}

	if (strcmp(requestSong.art_url, currentSong.art_url) == 0) refreshArt = false;

	currentSong = requestSong;

	if (refreshArt)
	{
//		ESP_LOGI("Page_Spotify", "Refreshing album art...");
//		if (WebClient_Get(currentSong.art_url, 0, NULL, ART_BUFFER_SIZE, artBuffer) == false)
//		{
//			ESP_LOGE("Page_Spotify", "Error refreshing album art for album %s!", currentSong.album);
//		}
	}

	RenderDisplay();
}

void Page_Spotify_Action()
{
	ESP_LOGI("Page_Spotify", "Switching display mode.");
	showArtLarge = !showArtLarge;
}

void RenderDisplay()
{
	char track_info[TRACK_INFO_BUFFER_SIZE];
	ESP_LOGI("Page_Spotify", "Redrawing display...");
	char urlBuf[128];

	if (showArtLarge)
	{
		ESP_LOGI("Page_Spotify", "Drawing large art...");
		TFT_fillScreen(BACKGROUND);
		if (TFT_jpg_image(CENTER, CENTER, 1, NULL, (uint8_t *)artBuffer, ART_BUFFER_SIZE) == 0)
		{
			TFT_setFont(DEJAVU24_FONT, NULL);
			TFT_print("?", CENTER, CENTER);
		}
	}
	else
	{
		ESP_LOGI("Page_Spotify", "Drawing normal view...");
		sprintf(track_info, "%s\n%s", currentSong.name, currentSong.artist);

		// Print song info
		text_wrap = 1;
		TFT_fillScreen(BACKGROUND);
		TFT_setFont(DEF_SMALL_FONT, NULL);
		_fg = FOREGROUND;
		_bg = BACKGROUND;
		TFT_print(track_info, 0, 0);
		TFT_print(currentSong.album, CENTER, 123);

		color_t colors[400];

//		if (TFT_jpg_image(ART_X + 2, ART_Y + 1, 2, NULL, (uint8_t *)artBuffer, ART_BUFFER_SIZE) == 0)
//		{
//			TFT_setFont(DEJAVU24_FONT, NULL);
//			TFT_print("?", CENTER, CENTER);
//		}
//		TFT_drawRect(ART_X, ART_Y, ART_DIMS, ART_DIMS, BACKGROUND);
		uint16_t y;
		uint16_t x;

		for (y = 0; y < 100 / 4; y++)
		{
			sprintf(urlBuf, "http://192.168.0.69/page_spotify?get_art_line=%d", y);
			WebClient_Get(urlBuf, 0, NULL, ART_BUFFER_SIZE, artBuffer);
			for (x = 0; x < 400; x++)
			{
				colors[x].r = artBuffer[x * 3];
				colors[x].g = artBuffer[x * 3 + 1];
				colors[x].b = artBuffer[x * 3 + 2];
//				TFT_drawPixel(x, y, (color_t){artBuffer[x * 3], artBuffer[x * 3 + 1], artBuffer[x * 3 + 2]}, 1);
//				TFT_drawFastHLineBuffer(x, y, 100, color)
//				//ESP_LOGI("aaaaaa", "Drawing (%d %d %d) to %d %d", artBuffer[x * 3], artBuffer[x * 3 + 1], artBuffer[x * 3 + 2], x, y);
			}
			TFT_drawFastHLineBuffer(0, y * 4, 100, colors);
			TFT_drawFastHLineBuffer(0, y * 4 + 1, 100, colors + 100);
			TFT_drawFastHLineBuffer(0, y * 4 + 2, 100, colors + 200);
			TFT_drawFastHLineBuffer(0, y * 4 + 3, 100, colors + 300);
		}
	}

	ESP_LOGI("Page_Spotify", "Finished drawing.");
}
