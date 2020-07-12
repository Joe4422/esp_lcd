#ifndef SPOTIFY_SPOTIFY_H_
#define SPOTIFY_SPOTIFY_H_

/****************************************************************
 * Includes
 ****************************************************************/
#include <stdbool.h>

/****************************************************************
 * Defines, consts
 ****************************************************************/
#define MAX_NAME_LENGTH	(32)
#define MAX_ALBUM_LENGTH (32)
#define MAX_ARTIST_LENGTH (32)
#define MAX_ART_URL_LENGTH (128)

/****************************************************************
 * Typedefs, structs, enums
 ****************************************************************/
typedef struct song
{
	char name[MAX_NAME_LENGTH];
	char album[MAX_ALBUM_LENGTH];
	char artist[MAX_ARTIST_LENGTH];
	char art_url[MAX_ART_URL_LENGTH];
	bool isPlaying;
} song_t;

/****************************************************************
 * Function declarations
 ****************************************************************/
bool Spotify_Init(char * refreshToken, char * clientToken);

bool Spotify_GetCurrentSong(song_t * song);

#endif /* SPOTIFY_SPOTIFY_H_ */
