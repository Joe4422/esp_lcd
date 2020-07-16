/****************************************************************
 * Includes
 ****************************************************************/
#include "client.h"

// ESP-IDF includes
#include "esp_http_client.h"
#include "esp_log.h"
#include "sdkconfig.h"

/****************************************************************
 * Function definitions
 ****************************************************************/
bool WebClient_Init()
{
	return true;
}

bool WebClient_Get(char * url, size_t headerCount, header_t * headers, size_t bufferSize, char * buffer)
{
	esp_err_t err;
	bool success = true;
	size_t i;
	size_t content_length;

	// Initialise client
	esp_http_client_config_t config =
	{
		.url = url,
		.method = HTTP_METHOD_GET
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// Copy headers into client
	for (i = 0; i < headerCount; i++)
	{
		esp_http_client_set_header(client, headers[i].key, headers[i].value);
	}

	// Open connection
	err = esp_http_client_open(client, 0);

	// Read data out into buffer if successful
	if (err == ESP_OK)
	{
		content_length = esp_http_client_fetch_headers(client);
		//ESP_LOGI("WebClient", "Read %d bytes of data from %s.", content_length, url);
		if (content_length < bufferSize)
		{
			esp_http_client_read(client, buffer, content_length);
			buffer[content_length] = '\0';
		}
		else success = false;
	}
	else success = false;


	if (esp_http_client_get_status_code(client) != 200) success = false;

	esp_http_client_close(client);
	esp_http_client_cleanup(client);
	return success;
}

bool WebClient_Post(char * url, size_t fieldsLength, char * fields, size_t headerCount, header_t * headers, size_t bufferSize, char * buffer)
{
	esp_err_t err;
	bool success = true;
	size_t i;
	size_t content_length;

	// Initialise client
	esp_http_client_config_t config =
	{
		.url = url,
		.method = HTTP_METHOD_POST
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	err = esp_http_client_set_post_field(client, "", 0);

	// Copy headers into client
	for (i = 0; i < headerCount; i++)
	{
		esp_http_client_set_header(client, headers[i].key, headers[i].value);
	}

	// Open connection
	err = esp_http_client_open(client, 0);

	// Read data out into buffer if successful
	if (err == ESP_OK)
	{
		content_length = esp_http_client_fetch_headers(client);
		if (content_length < bufferSize)
		{
			esp_http_client_read(client, buffer, content_length);
			buffer[content_length] = '\0';
		}
		else success = false;
	}
	else success = false;


	if (esp_http_client_get_status_code(client) != 200) success = false;

	esp_http_client_close(client);
	esp_http_client_cleanup(client);
	return success;

}
