/****************************************************************
 * Includes
 ****************************************************************/
#include "client.h"

// cstdlib includes
#include <string.h>

// ESP-IDF includes
#include "esp_system.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "tcpip_adapter.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

/****************************************************************
 * Defines, consts
 ****************************************************************/
#define PORT			(4567)
#define UDP_PACKET_SIZE	(1450)

/****************************************************************
 * Local variables
 ****************************************************************/
char addr_str[128];
uint8_t addr_family;
uint8_t ip_protocol;
int32_t sock;
struct sockaddr_in dest_addr;

/****************************************************************
 * Function definitions
 ****************************************************************/
bool WebClient_Init(char * ip)
{
	dest_addr.sin_addr.s_addr = inet_addr(ip);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(PORT);
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;
	inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

	sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
	if (sock < 0) return false;

	struct timeval to;
	to.tv_sec = 1;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));

	return true;
}

bool WebClient_Get(char * request, size_t bufferSize, char * buffer)
{
	//ESP_LOGI("WebClient", "Sending request %s.", request);
	uint32_t startTime = xTaskGetTickCount();
	int err = sendto(sock, request, strlen(request), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err < 0) return false;
	//ESP_LOGI("WebClient", "Sent request.");

	//ESP_LOGI("WebClient", "Expecting %d bytes of data. Waiting for response...", bufferSize);

	int len;
	int totalLen = 0;
	if (bufferSize <= UDP_PACKET_SIZE)
	{
		totalLen = lwip_read(sock, buffer, bufferSize);
	}
	else
	{
		int i = 0;
		for (i = 0; i < (bufferSize / UDP_PACKET_SIZE) + 1; i++)
		{
			len = lwip_recv(sock, buffer, bufferSize, 0);
			if (len < 0)
			{
				err = len;
				break;
			}
			buffer += len;
			totalLen += len;
		}
	}

	if (err < 0)
	{
		ESP_LOGE("WebClient", "LWIP Read ERROR!");
		return false;
	}

	//ESP_LOGI("WebClient", "Got response of %d bytes in %dms.", totalLen, (xTaskGetTickCount() - startTime) * portTICK_PERIOD_MS);
	return true;
}
