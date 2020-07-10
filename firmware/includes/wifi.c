/** 
 *   Modulo: wifi
 *   @file wifi.c
 *   Veja wifi.h para mais informações.
 ******************************************************************************/

/*******************************************************************************
 *                             MODULOS UTILIZADOS							   *
 ******************************************************************************/

/*
 * Inclusão de arquivos de cabeçalho da ferramenta de desenvolvimento.
 * Por exemplo: '#include <stdlib.h>'.
 */
#include <stdint.h>   /* Para as definições de uint8_t/uint16_t */
#include <stdbool.h>  /* Para as definições de true/false */
#include <string.h>

/*
 * Inclusão de arquivos de cabeçalho sem um arquivo ".c" correspondente.
 * Por exemplo: 
 * #include "stddefs.h" 
 * #include "template_header.h"
 */
#include "espressif/esp_common.h"

/*
 * Inclusão de arquivos de cabeçalho de outros módulos utilizados por este.
 * Por exemplo: '#include "serial.h"'.
 */
#include <FreeRTOS.h>
#include <task.h>
#include <awsiotmqtt.h>
#include <tools.h>
#include <dhcpserver/include/dhcpserver.h>
/* Network Stack */
#include "lwip/api.h"
#include "lwip/ip.h"

/*
 * Inclusão dos arquivos ".tab.h" deste módulo.
 * Por exemplo: 
 * #include "barcode.tab.h"
 * #include "template.tab.h"
 */

/*
 * Inclusão do arquivo de cabeçalho deste módulo.
 */
#include "wifi.h"

/*******************************************************************************
 *                     CONSTANTES E DEFINICOES DE MACRO						   *
 ******************************************************************************/
static const char * const auth_modes [] = {
    [AUTH_OPEN]         = "Open",
    [AUTH_WEP]          = "WEP",
    [AUTH_WPA_PSK]      = "WPA/PSK",
    [AUTH_WPA2_PSK]     = "WPA2/PSK",
    [AUTH_WPA_WPA2_PSK] = "WPA/WPA2/PSK"
};

/*******************************************************************************
 *                      ESTRUTURAS E DEFINIÇÕES DE TIPOS					   *
 ******************************************************************************/

/*******************************************************************************
 *                        VARIÁVEIS PUBLICAS (Globais)						   *
 ******************************************************************************/

/*******************************************************************************
 *                  DECLARACOES DE VARIAVEIS PRIVADAS (static)				   *
 ******************************************************************************/
static uint16_t *blk;
static struct netconn *client = NULL;

/*******************************************************************************
 *                   PROTOTIPOS DAS FUNCOES PRIVADAS (static)				   *
 ******************************************************************************/
static void scan_done_cb(void *arg, sdk_scan_status_t status);

/*******************************************************************************
 *                      IMPLEMENTACAO DAS FUNCOES PUBLICAS					   *
 ******************************************************************************/
/*
 * A documentação destas funções é realizada no arquivo ".h" deste módulo.
 */
void vServerWiFiTask(void *pvParameters)
{
	char apname[20];
	struct ip_info ap_ip;
	struct netbuf *inbuf;
	uint8_t counterr;
	uint8_t *pbuffer;

	memset(apname, 0, sizeof(apname));
	strcpy(apname, "slamp-");
	strcat(apname, get_my_id());

	/* Iniciando Ap para configuração. */
	sdk_wifi_set_opmode(STATIONAP_MODE);

	IP4_ADDR(&ap_ip.ip, 172, 16, 0, 1);
	IP4_ADDR(&ap_ip.gw, 172, 16, 0, 1);
	IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
	sdk_wifi_set_ip_info(1, &ap_ip);

	struct sdk_softap_config *cfg = (struct sdk_softap_config *)malloc(sizeof(struct sdk_softap_config));
	sdk_wifi_softap_get_config(cfg);

	sprintf((char *)cfg->ssid, apname);
	sprintf((char *)cfg->password, WIFI_AP_PASSSSID);
	cfg->ssid_hidden = 0;
	cfg->channel = 3;
	cfg->ssid_len = strlen(apname);
	cfg->authmode = AUTH_WPA2_PSK;
	cfg->max_connection = 2;
	cfg->beacon_interval = 100;
	sdk_wifi_softap_set_config(cfg);

	ip_addr_t first_client_ip;
	IP4_ADDR(&first_client_ip, 172, 16, 0, 2);
	dhcpserver_start(&first_client_ip, 4);
	dhcpserver_set_dns(&ap_ip.ip);
	dhcpserver_set_router(&ap_ip.ip);

	struct netconn *nc = netconn_new(NETCONN_TCP);
	if (!nc)
	{
		printf("Status monitor: Failed to allocate socket.\r\n");
		return;
	}
	netconn_bind(nc, IP_ANY_TYPE, WIFI_SRV_PORT);
	netconn_listen(nc);

	while(1)
	{
		err_t err = netconn_accept(nc, &client);

		if (err != ERR_OK)
		{
			if (client)	netconn_delete(client);
			continue;
		}

		client->recv_timeout = 1000;
        counterr = 0;

		do
		{
			err = netconn_recv(client, &inbuf);
			if (err == ERR_OK)
			{
				uint16_t szdata = 0;

				counterr++;

				if(netbuf_len(inbuf) < WIFI_TAMBUFFJSN)
				{
					netbuf_data(inbuf, (void**)&pbuffer, &szdata);

					if(*pbuffer == '{')
					{
						if(strstr(pbuffer, "scan") != NULL)
						{
							sdk_wifi_station_scan(NULL, scan_done_cb);
						}
						counterr = 0;
					}
				}
				if(counterr >= WIFI_CMD_ERR)
				{
					err = ERR_CLSD;
				}
			}
			else
			{
				// Tratar aqui, timeouts, desconexões, etc...
			}
			netbuf_delete(inbuf);
		} while((err == ERR_OK) || (err == ERR_TIMEOUT));

		netconn_close(client);		/* Close connection. */
	}
}

void vClientWifiTask(void *pvParameters)
{
    uint8_t status  = 0;
    bool update = true;
    bool taskstarted = false;
    struct sdk_station_config config =
    {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSSSID,
    };

    blk = (uint16_t *)pvParameters;

    printf("%s: connecting to WiFi\n\r", __func__);

    sdk_wifi_set_opmode(STATION_MODE);

    while(1)
    {
    	status = sdk_wifi_station_get_connect_status();

    	printf("\r\n%s: status = %d.", __func__, status );

    	if(update == true)
    	{
    		if(status == STATION_GOT_IP)
    		{
    			sdk_wifi_station_disconnect();
    			printf("\r\nDisconnected to config update.");
    			vTaskDelay(2000 / portTICK_PERIOD_MS);
    		}
    		printf("\r\nConfiguring...");
    		sdk_wifi_station_set_config(&config);
    		update = false;
    	}

        if(status != STATION_GOT_IP)
        {
        	update = false;
        	*blk = 500;

            if( status == STATION_WRONG_PASSWORD )
            {
            	printf("%s: wrong password\n\r", __func__);
            }
            else if( status == STATION_NO_AP_FOUND )
            {
            	printf("\r\n%s: AP not found.", __func__);
            }
            else if( status == STATION_CONNECT_FAIL )
            {
            	printf("\r\n%s: connection failed.", __func__);
            }
            sdk_wifi_station_connect();
        }
        else
        {
        	printf("\r\n%s: Connected.", __func__);

        	if(taskstarted == false)
        	{
        		xTaskCreate(vAWSIOTMQTTTask, "AmazonMQTT", (configMINIMAL_STACK_SIZE * 8), blk, 1, NULL);	/* Ativando comunicação MQTT com a Amazon. */
        		taskstarted = true;
        	}
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

/******************************************************************************
 *                    IMPLEMENTACAO DAS FUNCOES PRIVADAS					  *
 *****************************************************************************/
static void scan_done_cb(void *arg, sdk_scan_status_t status)
{
    char ssid[33]; // max SSID length + zero byte
    char jsonmsg[WIFI_TAMBUFFJSN];

    if (status != SCAN_OK)
    {
        printf("Error: WiFi scan failed\n");
        return;
    }

    struct sdk_bss_info *bss = (struct sdk_bss_info *)arg;
    // first one is invalid
    bss = bss->next.stqe_next;

    while (NULL != bss)
    {
        size_t len = strlen((const char *)bss->ssid);
        memcpy(ssid, bss->ssid, len);
        ssid[len] = 0;

        sprintf(&jsonmsg[0], "{\"rede\":{\"nome\":\"%s\",\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",\"rssi\":%02d,\"security\":\"%s\"}}" , ssid
        																															    , bss->bssid[0]
																																		, bss->bssid[1]
																																		, bss->bssid[2]
																																		, bss->bssid[3]
																																		, bss->bssid[4]
																																		, bss->bssid[5]
																																		, bss->rssi, auth_modes[bss->authmode]);
        netconn_write(client, &jsonmsg[0], strlen(jsonmsg), NETCONN_COPY);

        bss = bss->next.stqe_next;
    }
}

/******************************************************************************	
 *                                    EOF                                     *
 *****************************************************************************/
