/*
 *  LibNoPoll: A websocket library
 *  Copyright (C) 2013 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build Websocket enabled solutions
 *  contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         Edificio Alius A, Oficina 102,
 *         C/ Antonio Suarez Nº 10,
 *         Alcalá de Henares 28802 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/nopoll
 */
#include <nopoll/nopoll.h>
#include "ssl_compat-1.0.h"
#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "cJSON.h"
//#define local_host_ip		"192.168.88.236"
#define hostip "192.168.88.236"
#define local_host_url		"ws://192.168.88.236:9054"
#define local_host_port		"9054"
char local_host_ip[100];
noPollCtx  * ctx;
noPollConn * conn;
int          iter;
nopoll_bool debug = nopoll_false;
nopoll_bool show_critical_only = nopoll_false;
LOCAL xQueueHandle	Web_QueueStop = NULL;
 char *gethousedata="{\"type\":x0000,\"UserName\":\"xxxxxxxxx\",\"Password\":\"123123q\"}";
 typedef struct
 {
          char *Password;
          char *UserName;
          int VMHomeId;
          int VMHomeName;
          char success;
          int type;
 } PERSON;
//#define local_host_ports	"9443"

void __report_critical (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data)
{
        if (level == NOPOLL_LEVEL_CRITICAL) {
  	        printf ("CRITICAL: %s\n", log_msg);
	}
	return;
}

noPollCtx * create_ctx (void) {
	
	/* create a context */
	noPollCtx * ctx = nopoll_ctx_new ();
	nopoll_log_enable (ctx, debug);
	nopoll_log_color_enable (ctx, debug);

	/* configure handler */
	if (show_critical_only)
	        nopoll_log_set_handler (ctx, __report_critical, NULL);
	return ctx;
}
void initConnection()
{
	/* create context */
	ctx = create_ctx();

	/* check connections registered */
	if (nopoll_ctx_conns(ctx) != 0) {
		printf(
				"ERROR: expected to find 0 registered connections but found: %d\n",
				nopoll_ctx_conns(ctx));
	} /* end if */

	nopoll_ctx_unref(ctx);

	/* reinit again */
	ctx = create_ctx();
	printf("connect to ip\r\n");
	printf(local_host_ip);
	printf("\r\n");
		printf("\r\n");
	/* call to create a connection */
	conn = nopoll_conn_new(ctx, local_host_ip, local_host_port, NULL,
			local_host_url, NULL, NULL);
	if (!nopoll_conn_is_ok(conn)) {
		printf(
				"ERROR: Expected to find proper client connection status, but found error.. (conn=%p, conn->session=%d, NOPOLL_INVALID_SOCKET=%d, errno=%d, strerr=%s)..\n",
				conn, (int) nopoll_conn_socket(conn),
				(int) NOPOLL_INVALID_SOCKET, errno, strerror(errno));
	}
}


void ICACHE_FLASH_ATTR
revData_task(void *pvParameters)
{
	noPollMsg * msg;
	while (1) {
		iter = 0;
		while ((msg = nopoll_conn_get_msg(conn)) == NULL) {

			if (!nopoll_conn_is_ok(conn)) {
				printf(
						"ERROR: received websocket connection close during wait reply..\n");
			}
			nopoll_sleep(10000);
			if (iter > 10)
				break;
		}
		printf("Recieve data:%s\n", (const char *) nopoll_msg_get_payload(msg));
		cJSON*root=cJSON_Parse((char *) nopoll_msg_get_payload(msg));
		cJSON*item=cJSON_GetObjectItem(root,"UserName");
		printf(item->valuestring);
		printf("\r\n");
		cJSON_Delete(root);
		nopoll_msg_unref(msg);
	}
	vTaskDelete(NULL);
}
nopoll_bool test_01 (void) {

	initConnection();
	xTaskCreate(revData_task, "revData_task", 512, NULL, 2, NULL);
		printf ("Test 01: sending basic content..\n");

		/* send content text(utf-8) */
		if (nopoll_conn_send_text (conn, gethousedata,strlen(gethousedata)) != strlen(gethousedata)) {
			printf ("ERROR: Expected to find proper send operation..\n");
			return nopoll_false;
		}
		vTaskDelay(2000 / portTICK_RATE_MS);

	return nopoll_true;
}


LOCAL int websocket_main (char *argv)
{
	int iterator = *argv;
	printf("interator:%d\n",iterator);
	switch (iterator) {
		case 1:
			if (test_01()) {
				printf("Test 01: Simple request/reply [   OK   ]\n");
			} else {
				printf("Test 01: Simple request/reply [ FAILED ]\n");
			}

			break;
		default:
			break;
	}

	/* call to cleanup */
	nopoll_cleanup_library ();
	printf ("All tests ok!!\n");

	return 0;
}

LOCAL void websocket_task(void *pvParameters)
{

	bool ValueFromReceive = false;
	portBASE_TYPE xStatus;
	vTaskDelay(2000 / portTICK_RATE_MS);
	websocket_main((char*)pvParameters);
	while (1) {
		xStatus = xQueueReceive(Web_QueueStop,&ValueFromReceive,0);
		if (xStatus == pdPASS && ValueFromReceive == true){
			printf("websocket_task exit signal\n");
			/* finish connection */
			nopoll_conn_close (conn);

			/* finish */
			nopoll_ctx_unref (ctx);
			break;
		}
		/* send content text(utf-8) */
		if (nopoll_conn_send_text (conn, gethousedata,strlen(gethousedata)) != strlen(gethousedata)) {
			printf ("ERROR: Expected to find proper send operation..\n");
		}
		vTaskDelay(3000 / portTICK_RATE_MS);
		printf("websocket_task111\n");
	}
	vQueueDelete(Web_QueueStop);
	Web_QueueStop = NULL;
	vTaskDelete(NULL);
	printf("delete the websocket_task\n");
}

/*start the websocket task*/
void websocket_start(void *optarg,char *localip,unsigned char len)
{
	//uint8 ip[len];
	memcpy(local_host_ip,localip,len);
	//local_host_ip=localip;
	printf(local_host_ip);
	printf("\r\n");
	printf("\r\n");
	printf(hostip);
	printf("\r\n");
	printf("\r\n");

	if (Web_QueueStop == NULL){
		Web_QueueStop = xQueueCreate(1,1);
	}

	if (Web_QueueStop != NULL){
		xTaskCreate(websocket_task, "websocket_task", 2048, optarg, 2, NULL);
	}
}

/*stop the websocket task*/
sint8 websocket_stop(void)
{
	bool ValueToSend = true;
	portBASE_TYPE xStatus;

	if (Web_QueueStop == NULL)
		return -1;

	xStatus = xQueueSend(Web_QueueStop,&ValueToSend,0);
	if (xStatus != pdPASS)
		return -1;
	else
		return pdPASS;
}
/* end-of-file-found */
