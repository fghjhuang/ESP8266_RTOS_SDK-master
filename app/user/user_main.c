/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"
#include "udp_client.h"
/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/

/***********************************常量***************************************/

/***********************************变量***************************************/
char name[5]="david";
char readname[5];
os_timer_t reconnect_timer;
bool reset_smartconfig_status=false;
/*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}
/*
 * smartconfig任务回调
 * */
uint8 onetime=0;
void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
    switch(status) {
        case SC_STATUS_WAIT:
            printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            printf("SC_STATUS_GETTING_SSID_PSWD\n");
            sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = pdata;
            if(onetime==0)
            {
            	onetime=1;
            	wifi_station_set_config(sta_conf);
	        wifi_station_disconnect();
	        wifi_station_connect();

            }

            break;
        case SC_STATUS_LINK_OVER:
            printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
				//SC_TYPE_ESPTOUCH
                uint8 phone_ip[4] = {0};
                memcpy(phone_ip, (uint8*)pdata, 4);
                printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
                onetime=0;
            } else {
            	//SC_TYPE_AIRKISS - support airkiss v2.0
				//airkiss_start_discover();
			}
            smartconfig_stop();
            reset_smartconfig_status = false;
            break;
    }

}
/*
 * smartconfig任务开始
 * */
void ICACHE_FLASH_ATTR
smartconfig_task(void *pvParameters)
{
    smartconfig_start(smartconfig_done);

    vTaskDelete(NULL);
}
/*
 * gpio口初始化
 * */
void ICACHE_FLASH_ATTR
gpioinit()
{
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_GPIO5);
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(5));

}
/*
 * wificonnect任务开始
 * */
void ICACHE_FLASH_ATTR
resetconfig_task(void *pvParameters) {
	gpioinit();
	while (1) {
		uint8 level = GPIO_INPUT_GET(GPIO_ID_PIN(5));
		if (level == 0) {
			vTaskDelay(3000 / portTICK_RATE_MS);
			level = GPIO_INPUT_GET(GPIO_ID_PIN(5));
			if (level == 0) {
				if(!reset_smartconfig_status)
				{
					printf("reset to do smartconfig task\r\n");
					wifi_station_disconnect();//先断开连接函数
					os_timer_disarm(&reconnect_timer);
					reset_smartconfig_status = true;//开始复位重新smartconfig状态位
					xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL,
							2, NULL);				//smartconfig任务
				}


			}
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
	vTaskDelete(NULL);
}
/*
 * 测试任务
 * */
uint8 type=0;
void ICACHE_FLASH_ATTR
task1(void *pvParameters)
{
	gpioinit();


    printf("task 1\n");
    for (;;)
    {

#if 1
    	uint8 level=GPIO_INPUT_GET(GPIO_ID_PIN(5));
    	if(level==0)
    	{
    		vTaskDelay(3000 / portTICK_RATE_MS);
    		level=GPIO_INPUT_GET(GPIO_ID_PIN(5));
    		if(level==0)
    		{
    			xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL, 2, NULL);//smartconfig任务
    		}
    	}

#endif

#if 0
    	uint8 level=GPIO_INPUT_GET(GPIO_ID_PIN(5));
    	if(level==0)
    	{
    		printf("press\r\n");
    	}else{
    		printf("not press\r\n");
    	}
    	switch(type)
    	{
    	case 0:
    		GPIO_OUTPUT_SET(GPIO_ID_PIN(4),1);
    		type++;
    		break;
    	case 1:
    		GPIO_OUTPUT_SET(GPIO_ID_PIN(4),0);
    		type=0;
    		break;
    	}

        vTaskDelay(2000 / portTICK_RATE_MS);
#endif
    }
    vTaskDelete(NULL);
}
/*
 * wifi重新连接定时器
 * */
void ICACHE_FLASH_ATTR Wifi_reconned(void *arg) {
	os_timer_disarm(&reconnect_timer);
	printf("Wifi reconnecting！\r\n");
	wifi_station_connect();//重新连接，连接成功或者失败会在系统信息那里有回调
}
/*
 * 系统信息回调
 * */
void wifi_event_handler_cb(System_Event_t *event)
{
    if (event == NULL) {
        return;
    }

    switch (event->event_id) {
        case EVENT_STAMODE_GOT_IP://连接成功并成功获取ip
        	os_timer_disarm(&reconnect_timer);
        	printf("Wifi connect success!msg\r\n");
    		sntp_setservername(0, "0.cn.pool.ntp.org");
    		sntp_setservername(1, "1.cn.pool.ntp.org");
    		sntp_setservername(2, "2.cn.pool.ntp.org");
    		sntp_init();
    		udpClient();
            break;
        case EVENT_STAMODE_DISCONNECTED://连接断开
        	if(reset_smartconfig_status)
        	{

        	}else{
        		wifi_station_disconnect();//先断开连接函数
        		os_timer_arm(&reconnect_timer,10000,NULL);
        		printf("Wifi will reconnect after 10s \r\n");
        	}

            break;
        default:
            break;
    }
}
/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	struct station_config s_staconf;
	uart_init_new();//初始化uart

	wifi_set_opmode(STATION_MODE);//设备模式
	wifi_station_get_config_default(&s_staconf);//读取是否保存有wifi的配置信息

	    if (sizeof(s_staconf.ssid) != 0) //保存有信息，就扫描是否有该wifi，有就进行连接
	    {
		printf("wifi connect start\r\n");

		os_timer_setfn(&reconnect_timer, Wifi_reconned, NULL); //配置重新连接的定时器
		wifi_station_connect(); //进行连接
		wifi_set_event_handler_cb(wifi_event_handler_cb); //注册系统信息回调函数
		xTaskCreate(resetconfig_task, "resetconfig_task", 256, NULL, 2, NULL); //resetconfig_task任务
	    } else {//没有保存就进行smartconfig
	      printf("smartcfg\r\n");
	      xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL, 2, NULL);//smartconfig任务
	    }
	    xTaskCreate(task1, "task1", 256, NULL, 2, NULL);


}

