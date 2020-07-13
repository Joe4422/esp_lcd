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
	.header_fg = (color_t){ 255, 255, 255 },
	.header_bg = (color_t){ 30, 215, 96 },
	.update_period = MS_TO_TICKS(5000)
};

/****************************************************************
 * Local variables
 ****************************************************************/
static song_t currentSong;
static char * artBuffer;

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
	char track_info[TRACK_INFO_BUFFER_SIZE];
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

	ESP_LOGI("Page_Spotify", "Old art URL: %s", currentSong.art_url);
	ESP_LOGI("Page_Spotify", "New art URL: %s", requestSong.art_url);

	if (strcmp(requestSong.art_url, currentSong.art_url) == 0) refreshArt = false;

	currentSong = requestSong;

	if (refreshArt)
	{
		ESP_LOGI("Page_Spotify", "Refreshing album art...");
		if (WebClient_Get(currentSong.art_url, 0, NULL, ART_BUFFER_SIZE, artBuffer) == false)
		{
			ESP_LOGE("Page_Spotify", "Error refreshing album art for album %s!", currentSong.album);
		}
	}

	ESP_LOGI("Page_Spotify", "Redrawing display...");

	sprintf(track_info, "%s\n%s", currentSong.name, currentSong.artist);

	// Print song info
	text_wrap = 1;
	TFT_fillScreen(BACKGROUND);
	TFT_setFont(DEF_SMALL_FONT, NULL);
	_fg = FOREGROUND;
	_bg = BACKGROUND;
	TFT_print(track_info, 0, 0);
	TFT_print(currentSong.album, CENTER, 123);

	if (TFT_jpg_image(ART_X + 2, ART_Y + 1, 2, NULL, (uint8_t *)artBuffer, ART_BUFFER_SIZE) == 0)
	{
		TFT_setFont(DEJAVU24_FONT, NULL);
		TFT_print("?", CENTER, CENTER);
	}
	TFT_drawRect(ART_X, ART_Y, ART_DIMS, ART_DIMS, BACKGROUND);
}
