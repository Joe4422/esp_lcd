/****************************************************************
 * Includes
 ****************************************************************/
#include "spotify.h"

// cstdlib includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ESP-IDF includes
#include "esp_log.h"
#include "sdkconfig.h"

// CJSON includes
#include "cjson/cjson.h"

// WEB_CLIENT includes
#include "web_client/client.h"

/****************************************************************
 * Defines, consts
 ****************************************************************/
#define MAX_TOKEN_LENGTH	(148)
#define JSON_BUFFER_LENGTH	(5000)
#define FIELD_BUFFER_LENGTH	(512)

#define CURRENTLY_PLAYING_URL	("https://api.spotify.com/v1/me/player/currently-playing")
#define REFRESH_TOKEN_URL		("https://accounts.spotify.com/api/token?grant_type=refresh_token&refresh_token=")

/****************************************************************
 * Local variables
 ****************************************************************/
static char access_token[MAX_TOKEN_LENGTH];
static char refresh_token[MAX_TOKEN_LENGTH];
static char client_token[MAX_TOKEN_LENGTH] = "Basic ";

/****************************************************************
 * Function declarations
 ****************************************************************/
bool Spotify_RefreshAccessToken();

/****************************************************************
 * Function definitions
 ****************************************************************/
bool Spotify_Init(char * refreshToken, char * clientToken)
{
	strcpy(refresh_token, refreshToken);
	strcpy(client_token + 6, clientToken);

	return Spotify_RefreshAccessToken();
}

bool Spotify_GetCurrentSong(song_t * song)
{
	Spotify_RefreshAccessToken();

	char * buffer = malloc(JSON_BUFFER_LENGTH);
	header_t headers[1] = { (header_t){ "Authorization", access_token } };
	WebClient_Get(CURRENTLY_PLAYING_URL, 1, headers, JSON_BUFFER_LENGTH, buffer);

	ESP_LOGI("Spotify", "Attempting to parse JSON...");
	// Parse JSON data
	cJSON * json = cJSON_Parse(buffer);
	free(buffer);
	if (json == NULL) { ESP_LOGE("Spotify", "Json not found!"); return false; }
	cJSON * item = cJSON_GetObjectItemCaseSensitive(json, "item");
	if (item == NULL) { ESP_LOGE("Spotify", "Item not found!"); return false; }
	cJSON * album = cJSON_GetObjectItemCaseSensitive(item, "album");
	if (album == NULL) { ESP_LOGE("Spotify", "Album not found!"); return false; }
	cJSON * artists = cJSON_GetObjectItemCaseSensitive(album, "artists");
	if (artists == NULL) { ESP_LOGE("Spotify", "Artists not found!"); return false; }
	cJSON * images = cJSON_GetObjectItemCaseSensitive(album, "images");
	if (images == NULL) { ESP_LOGE("Spotify", "Images not found!"); return false; }

	cJSON * trackName = cJSON_GetObjectItemCaseSensitive(item, "name");
	if (trackName == NULL) { ESP_LOGE("Spotify", "Track name not found!"); return false; }
	strcpy(song->name, trackName->valuestring);

	cJSON * albumName = cJSON_GetObjectItemCaseSensitive(album, "name");
	if (albumName == NULL) { ESP_LOGE("Spotify", "Album name not found!"); return false; }
	strcpy(song->album, albumName->valuestring);

	cJSON * artistName = cJSON_GetObjectItemCaseSensitive(artists->child, "name");
	if (artistName == NULL) { ESP_LOGE("Spotify", "Artist name not found!"); return false; }
	strcpy(song->artist, artistName->valuestring);

	cJSON * artURL = cJSON_GetObjectItemCaseSensitive(images->child->next, "url");
	if (artURL == NULL) { ESP_LOGE("Spotify", "Art URL not found!"); return false; }
	strcpy(song->art_url, artURL->valuestring);

//	cJSON * isPlaying = cJSON_GetObjectItemCaseSensitive(json, "is_playing");
//	if (isPlaying == NULL) { ESP_LOGE("Spotify", "Is Playing not found!"); return false; }
//	song->isPlaying = (isPlaying->valuestring[0] == 't');

	cJSON_Delete(json);

	return true;
}

bool Spotify_RefreshAccessToken()
{
	char * buffer = malloc(JSON_BUFFER_LENGTH);
	char * url = malloc(FIELD_BUFFER_LENGTH);
	header_t headers[1] = { (header_t){ "Authorization", client_token } };

	// Initialise field
	size_t fields_length = sprintf(url, "%s%s", REFRESH_TOKEN_URL, refresh_token);

	// Fetch JSON
	WebClient_Post(url, 0, NULL, 1, headers, JSON_BUFFER_LENGTH, buffer);
	ESP_LOGI("Spotify", "%s", buffer);

	free(url);

	// Parse JSON
	cJSON * json = cJSON_Parse(buffer);
	free(buffer);

	if (json == NULL) { ESP_LOGE("Spotify", "Json not found!"); return false; }
	cJSON * json_access_token = cJSON_GetObjectItemCaseSensitive(json, "access_token");
	if (json_access_token == NULL) { ESP_LOGE("Spotify", "Access token not found!"); return false; }
	sprintf(access_token, "Bearer %s", json_access_token->valuestring);

	cJSON_Delete(json);
	return true;
}
