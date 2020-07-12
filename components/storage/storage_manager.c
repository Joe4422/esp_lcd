/****************************************************************
 * Includes
 ****************************************************************/
#include "storage_manager.h"

// cstdlib includes
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// ESP-IDF includes
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "sdkconfig.h"

/****************************************************************
 * Defines, consts
 ****************************************************************/
// Logging tag
const char* STORAGE_LOG_TAG =		"Storage Manager";

// Storage keywords
const char* STORAGE_NAMESPACE =		"storage";

/****************************************************************
 * Function declarations
 ****************************************************************/
bool ReadGenericBlob
(
	const char *	key,
	const size_t	blob_size,
	void * 			out
);

/****************************************************************
 * Local variables
 ****************************************************************/
nvs_handle_t storage_handle;

bool storage_init_done = false;

/****************************************************************
 * Function definitions
 ****************************************************************/
bool Storage_Init()
{
	ESP_LOGI(STORAGE_LOG_TAG, "Initialising NVS flash...");
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();

    // Check init outcome
    switch (ret)
    {
    case ESP_OK: // Init succeeded normally
    	ESP_LOGI(STORAGE_LOG_TAG, "Initialised NVS flash.");
		break;
    case ESP_ERR_NVS_NO_FREE_PAGES:		// Init failed but flash can be erased + re-initialised
    case ESP_ERR_NVS_NEW_VERSION_FOUND:
		ESP_LOGW(STORAGE_LOG_TAG, "NVS flash init failed! Erasing and trying again...");
		ret = nvs_flash_erase();
		// Check if erase failed
		if (ret != ESP_OK)
		{
			// Erase failed; return false
			ESP_LOGE(STORAGE_LOG_TAG, "NVS flash erase failed!");
			return false;
		}
		// Try and re-init
		ret = nvs_flash_init();
		// Check if re-init failed
		if (ret != ESP_OK)
		{
			// Re-init failed; return false
			ESP_LOGE(STORAGE_LOG_TAG, "NVS flash re-init after erase failed!");
			return false;
		}
		break;
	// Init failed for any other reason
    default:
		ESP_LOGE(STORAGE_LOG_TAG, "NVS flash init failed!");
		return false;
    }

    // Init succeeded normally or after erase + re-init
    storage_init_done = true;
	return true;
}

bool ReadGenericBlob
(
	const char *	key,
	const size_t	blob_size,
	void * 			out
)
{
    esp_err_t ret;

	// Initialise memory to load blob into
	void * blob_data = malloc(blob_size);

	// Copy existing data in out into blob_data
	memcpy(blob_data, out, blob_size);

	size_t real_blob_size;

	// Attempt to load blob from storage
	ret = nvs_get_blob(storage_handle, key, blob_data, &real_blob_size);
	// Check outcome of attempted load
	switch (ret)
	{
	// Load succeeds
	case ESP_OK:
		break;
	// Load fails because key does not exist
	case ESP_ERR_NVS_NOT_FOUND:
		// Attempt to create blob
		ret = nvs_set_blob(storage_handle, key, blob_data, blob_size);
		// Check if blob creation succeeded
		if (ret != ESP_OK)
		{
			// Blob creation failed; return false
			return false;
		}
		break;
	// Load fails for other reason
	default:
		return false;
	}

	// Copy data in blob_data into out
	memcpy(out, blob_data, blob_size);
	free(blob_data);
	return true;
}
