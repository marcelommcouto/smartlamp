/** 
 *   Modulo: awsiotmqtt
 *   @file awsiotmqtt.c
 *   Veja awsiotmqtt.h para mais informações.
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
#include <stdlib.h>
#include <stdio.h>

/*
 * Inclusão de arquivos de cabeçalho sem um arquivo ".c" correspondente.
 * Por exemplo: 
 * #include "stddefs.h" 
 * #include "template_header.h"
 */

/*
 * Inclusão de arquivos de cabeçalho de outros módulos utilizados por este.
 * Por exemplo: '#include "serial.h"'.
 */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <projdefs.h>
#include "ssl_connection.h"
#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>
#include <mbedtls/mbedtls/include/mbedtls/ssl.h>
#include <tools.h>
#include <espressif/esp_common.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_wifi.h>

/*
 * Inclusão dos arquivos ".tab.h" deste módulo.
 * Por exemplo: 
 * #include "barcode.tab.h"
 * #include "template.tab.h"
 */

/*
 * Inclusão do arquivo de cabeçalho deste módulo.
 */
#include "awsiotmqtt.h"

/*******************************************************************************
 *                     CONSTANTES E DEFINICOES DE MACRO						   *
 ******************************************************************************/
#define LAMP_PIN			14
#define TOPICSIZE			64

/*******************************************************************************
 *                      ESTRUTURAS E DEFINIÇÕES DE TIPOS					   *
 ******************************************************************************/
// trusted root CA certificate - https://www.symantec.com/content/en/us/enterprise/verisign/roots/VeriSign-Class%203-Public-Primary-Certification-Authority-G5.pem
static char *ca_cert = "-----BEGIN CERTIFICATE-----\r\n"
"MIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\r\n"
"yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\r\n"
"ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\r\n"
"U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\r\n"
"ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\r\n"
"aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\r\n"
"MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\r\n"
"ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\r\n"
"biwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\r\n"
"U2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\r\n"
"aXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\r\n"
"nmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\r\n"
"t0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\r\n"
"SdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\r\n"
"BO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\r\n"
"rCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\r\n"
"NIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\r\n"
"BAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\r\n"
"BgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\r\n"
"aXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\r\n"
"MzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\r\n"
"p6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\r\n"
"5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\r\n"
"WE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\r\n"
"4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\r\n"
"hnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\r\n"
"-----END CERTIFICATE-----\r\n";

// AWS IoT client endpoint
static char *client_endpoint = "cliente.iot.us-east-1.amazonaws.com";

// AWS IoT device certificate (ECC)
static char *client_cert =
"-----BEGIN CERTIFICATE-----\r\n"
"-----END CERTIFICATE-----\r\n";

// AWS IoT device private key (ECC)
static char *client_key =
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"-----END RSA PRIVATE KEY-----\r\n";

/*******************************************************************************
 *                        VARIÁVEIS PUBLICAS (Globais)						   *
 ******************************************************************************/

/*******************************************************************************
 *                  DECLARACOES DE VARIAVEIS PRIVADAS (static)				   *
 ******************************************************************************/
static int ssl_reset;
static SSLConnection *ssl_conn;
static QueueHandle_t publish_queue = NULL;
static uint16_t *blk;

/*******************************************************************************
 *                   PROTOTIPOS DAS FUNCOES PRIVADAS (static)				   *
 ******************************************************************************/
static int mqtt_ssl_read(mqtt_network_t * n, unsigned char* buffer, int len, int timeout_ms);
static int mqtt_ssl_write(mqtt_network_t* n, unsigned char* buffer, int len, int timeout_ms);
static void topic_received(mqtt_message_data_t *md);

/*******************************************************************************
 *                      IMPLEMENTACAO DAS FUNCOES PUBLICAS					   *
 ******************************************************************************/
/*
 * A documentação destas funções é realizada no arquivo ".h" deste módulo.
 */

void vAWSIOTMQTTTask(void *pvParameters)
{
    int ret = 0;
    struct mqtt_network network;
    mqtt_client_t client = mqtt_client_default;
    char mqtt_client_id[20];
    uint8_t mqtt_buf[TOPICSIZE];
    uint8_t mqtt_readbuf[TOPICSIZE];
    mqtt_packet_connect_data_t data = mqtt_packet_connect_data_initializer;
    char msg[TOPICSIZE];
    char subtopic[48];
    char pubtopic[48];

    blk = (uint16_t *)pvParameters;

    memset(mqtt_client_id, 0, sizeof(mqtt_client_id));
    strcpy(mqtt_client_id, "slamp-");
    strcat(mqtt_client_id, get_my_id());

    gpio_enable(LAMP_PIN, GPIO_OUTPUT);

    publish_queue = xQueueCreate(2, TOPICSIZE);

    ssl_conn = (SSLConnection *) malloc(sizeof(SSLConnection));

    printf("\r\nMQTT: Started with id %s.\n\r", mqtt_client_id);

    while (1)
    {
    	if(sdk_wifi_station_get_connect_status() != STATION_GOT_IP)
        {
    		*blk = 1000;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        printf("%s: started\n\r", __func__);

        ssl_reset = 0;
        ssl_init(ssl_conn);
        ssl_conn->ca_cert_str = ca_cert;
        ssl_conn->client_cert_str = client_cert;
        ssl_conn->client_key_str = client_key;

        mqtt_network_new(&network);
        network.mqttread = mqtt_ssl_read;
        network.mqttwrite = mqtt_ssl_write;

        printf("%s: connecting to MQTT server %s ... ", __func__, client_endpoint);
        ret = ssl_connect(ssl_conn, client_endpoint, MQTT_PORT);

        if (ret)
        {
            printf("error: %d\n\r", ret);
            ssl_destroy(ssl_conn);
            continue;
        }
        printf("Done\n\r");
        mqtt_client_new(&client, &network, 5000, mqtt_buf, TOPICSIZE, mqtt_readbuf, TOPICSIZE);

        data.willFlag = 0;
        data.MQTTVersion = 4;
        data.cleansession = 1;
        data.clientID.cstring = mqtt_client_id;
        data.username.cstring = NULL;
        data.password.cstring = NULL;
        data.keepAliveInterval = 60;

        printf("Send MQTT connect ... ");
        ret = mqtt_connect(&client, &data);
        if (ret)
        {
            printf("Error: %d\n\r", ret);
            ssl_destroy(ssl_conn);
            continue;
        }
        printf("Done\r\n");

        sprintf(&subtopic[0], MQTT_SUB_TOPIC, get_my_id());
        printf("Topic to receive data: %s\r\n", subtopic);
        sprintf(&pubtopic[0], MQTT_PUB_TOPIC, get_my_id());
        printf("Topic to send data: %s\r\n", pubtopic);

        mqtt_subscribe(&client, subtopic, MQTT_QOS1, topic_received);

        *blk = 5000;

        xQueueReset(publish_queue);

        while (sdk_wifi_station_get_connect_status() == STATION_GOT_IP && !ssl_reset)
        {
            while (xQueueReceive(publish_queue, (void *) msg, 0) == pdTRUE)
            {
                mqtt_message_t message;

                message.payload = msg;
                message.payloadlen = strlen(msg);
                message.dup = 0;
                message.qos = MQTT_QOS1;
                message.retained = 0;
                ret = mqtt_publish(&client, pubtopic, &message);
                if (ret != MQTT_SUCCESS)
                {
                    printf("Error while publishing message: %d\n", ret);
                    break;
                }
                printf("Published: %s\r\n", msg);
            }

            ret = mqtt_yield(&client, 1000);
            if (ret == MQTT_DISCONNECTED) break;
        }
        printf("\r\nConnection dropped, request restart.");
        ssl_destroy(ssl_conn);
    }
}

/******************************************************************************
 *                    IMPLEMENTACAO DAS FUNCOES PRIVADAS					  *
 *****************************************************************************/
static void topic_received(mqtt_message_data_t *md)
{
    mqtt_message_t *message = md->message;
    bool sendresp = false;
    bool ison = false;
    int i;

    printf("\r\nReceived: ");
    for (i = 0; i < (int) message->payloadlen; ++i) printf("%c", ((char *) (message->payload))[i]);

    if (!strncmp(message->payload, "on", 2))
    {
		printf("\r\nTurning on LAMP.");
		gpio_write(LAMP_PIN, 1);
		sendresp = true;
		ison = true;
	}
    else if (!strncmp(message->payload, "off", 3))
	{
		printf("\r\nTurning off LAMP.");
		gpio_write(LAMP_PIN, 0);
		sendresp = true;
	}

    if(sendresp == true)
    {
    	char msg[TOPICSIZE];

    	sprintf(&msg[0],"{\"status\": \"%s\"}", ison ? "On" : "Off");

		xQueueSend(publish_queue, msg, 100);
    }
}

/******************************************************************************/
static int mqtt_ssl_read(mqtt_network_t * n, unsigned char* buffer, int len, int timeout_ms)
{
    int r = ssl_read(ssl_conn, buffer, len, timeout_ms);

    if (r <= 0 && (r != MBEDTLS_ERR_SSL_WANT_READ && r != MBEDTLS_ERR_SSL_WANT_WRITE && r != MBEDTLS_ERR_SSL_TIMEOUT))
    {
        printf("%s: TLS read error (%d), resetting\n\r", __func__, r);
        ssl_reset = 1;
    };
    return r;
}

/******************************************************************************/
static int mqtt_ssl_write(mqtt_network_t* n, unsigned char* buffer, int len, int timeout_ms)
{
    int r = ssl_write(ssl_conn, buffer, len, timeout_ms);

    if (r <= 0 && (r != MBEDTLS_ERR_SSL_WANT_READ && r != MBEDTLS_ERR_SSL_WANT_WRITE))
    {
        printf("%s: TLS write error (%d), resetting\n\r", __func__, r);
        ssl_reset = 1;
    }
    return r;
}

/******************************************************************************	
 *                                    EOF                                     *
 *****************************************************************************/
