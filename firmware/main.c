/** 
 *   Modulo: main
 *   @file main.c
 *   Veja main.h para mais informações.
 ******************************************************************************/

/*******************************************************************************
 *                             MODULOS UTILIZADOS							   *
 ******************************************************************************/

/*
 * Inclusão de arquivos de cabeçalho da ferramenta de desenvolvimento.
 * Por exemplo: '#include <stdlib.h>'.
 */
#include <espressif/esp_common.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_wifi.h>
#include <esp8266.h>

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
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <wifi.h>
/*
 * Inclusão do arquivo de cabeçalho deste módulo.
 */
#include "main.h"

/*******************************************************************************
 *                     CONSTANTES E DEFINICOES DE MACRO						   *
 ******************************************************************************/
#define LED_PIN 			2
#define BLN_NONET			100

/*******************************************************************************
 *                      ESTRUTURAS E DEFINIÇÕES DE TIPOS					   *
 ******************************************************************************/

/*******************************************************************************
 *                        VARIÁVEIS PUBLICAS (Globais)						   *
 ******************************************************************************/
static uint16_t blinkperiod = BLN_NONET;

/*******************************************************************************
 *                  DECLARACOES DE VARIAVEIS PRIVADAS (static)				   *
 ******************************************************************************/

/*******************************************************************************
 *                   PROTOTIPOS DAS FUNCOES PRIVADAS (static)				   *
 ******************************************************************************/
static void blinkenTask(void *pvParameters);

/*******************************************************************************
 *                      IMPLEMENTACAO DAS FUNCOES PUBLICAS					   *
 ******************************************************************************/
/*
 * A documentação destas funções é realizada no arquivo ".h" deste módulo.
 */

void user_init(void)
{
	sdk_system_update_cpu_freq(160);	/* Velocidade padrao do Clock */

    uart_set_baud(0, 115200);			/* Configuração da serial. */

    xTaskCreate(blinkenTask, "StatusTask", (configMINIMAL_STACK_SIZE * 2), NULL, 2, NULL);					/* Tarefa de sinalização de problemas. */
//    xTaskCreate(vClientWifiTask, "WifiClient", (configMINIMAL_STACK_SIZE * 6), &blinkperiod, 1, NULL);	/* Ativando WiFi. */
    xTaskCreate(vServerWiFiTask, "WifiServer", (configMINIMAL_STACK_SIZE * 20), &blinkperiod, 1, NULL);		/* Ativando WiFi. */
}

/******************************************************************************
 *                    IMPLEMENTACAO DAS FUNCOES PRIVADAS					  *
 *****************************************************************************/

/* Tarefa de piscar o LED para sinalizar o que está acontecendo. */
static void blinkenTask(void *pvParameters)
{
	gpio_enable(LED_PIN, GPIO_OUTPUT);

	while(1)
	{
		gpio_write(LED_PIN, 1);
		vTaskDelay(blinkperiod / portTICK_PERIOD_MS);
		gpio_write(LED_PIN, 0);
		vTaskDelay(blinkperiod / portTICK_PERIOD_MS);
	}
}

/******************************************************************************	
 *                                    EOF                                     *
 *****************************************************************************/
