
#include "esp_common.h"
#include "espconn.h"
#include "c_types.h"
#include "user_config.h"

#include "udp_client.h"

os_timer_t time1;//定时器
char test_mode = 1;
static struct espconn udp_client;//udp对象

void UdpRecvCb(void *arg, char *pdata, unsigned short len)
{
    struct espconn* udp_server_local = arg;
    DBG_LINES("UDP_RECV_CB");
    DBG_PRINT("UDP_RECV_CB len:%d ip:%d.%d.%d.%d port:%d\n", len, udp_server_local->proto.tcp->remote_ip[0],
            udp_server_local->proto.tcp->remote_ip[1], udp_server_local->proto.tcp->remote_ip[2],
            udp_server_local->proto.tcp->remote_ip[3], udp_server_local->proto.tcp->remote_port\
);
    os_timer_disarm(&time1);
    espconn_send(udp_server_local, pdata, len);
    uint32 current_stamp;
    current_stamp = sntp_get_current_timestamp();
    printf("sntp: %d, %s \n", current_stamp, sntp_get_real_time(current_stamp));

    websocket_start(&test_mode,pdata,len);
}
void UdpSendCb(void* arg)
{
    struct espconn* udp_server_local = arg;
    DBG_LINES("UDP_SEND_CB");
    DBG_PRINT("UDP_SEND_CB ip:%d.%d.%d.%d port:%d\n", udp_server_local->proto.udp->local_ip[0],
            udp_server_local->proto.udp->local_ip[1], udp_server_local->proto.udp->local_ip[2],
            udp_server_local->proto.udp->local_ip[3], udp_server_local->proto.tcp->remote_port\
);
}
uint8 hextosingle(uint8 temp)
{
	uint8_t dst;
	 if (temp < 10){
	        dst = temp + '0';
	    }else{
	        dst = temp -10 +'A';
	    }
	    return dst;
}
uint8_t HexToChar(uint8 result[3],uint8_t temp)
{
    uint8_t dst;
    uint8 single=temp%10;
    uint8 ten=(temp/10)%10;
    uint8 handler=temp/100;
    switch(handler)
    {
    case 0:
    	switch(ten)
    	{
    	case 0:
    		result[0]=hextosingle(0);
    		result[1]=hextosingle(0);
    		result[2]=hextosingle(single);
    		return 1;
    		break;
    	default:
			result[0]=hextosingle(0);
			result[1]=hextosingle(ten);
			result[2]=hextosingle(single);
			return 2;
			break;
    	}
    	break;
    case 1:
    case 2:
    	result[0]=hextosingle(1);
    	result[1]=hextosingle(ten);
    	result[2]=hextosingle(single);
    	return 3;
    	break;
    }

    return 0;
}
void t1Callback(void* arg)
{
	os_printf("t1 callback\n");
	uint8 i=0;
	uint8 ipresult[4][3];
	uint8 ipsize0=HexToChar(ipresult[0],udp_client.proto.udp->local_ip[0]);
	uint8 ipsize1=HexToChar(ipresult[1],udp_client.proto.udp->local_ip[1]);
	uint8 ipsize2=HexToChar(ipresult[2],udp_client.proto.udp->local_ip[2]);
	uint8 ipsize3=HexToChar(ipresult[3],udp_client.proto.udp->local_ip[3]);
	uint8 tolsize=ipsize0+ipsize1+ipsize2+ipsize3+9;
	uint8 data[tolsize];
	uint8 count=0;
	for ( i = 0; i < ipsize0; i++) {
		data[i] = ipresult[0][i+3-ipsize0];
	}
	count = ipsize0;
	data[count] = 0x2e;
	count++;
	for ( i = 0; i < ipsize1; i++) {
		data[count + i] = ipresult[1][i+3-ipsize1];
	}
	count = count + ipsize1;
	data[count] = 0x2e;
	count++;
	for ( i = 0; i < ipsize2; i++) {
		data[count + i] = ipresult[2][i+3-ipsize2];
	}
	count = count + ipsize2;
	data[count] = 0x2e;
	count++;
	for ( i = 0; i < ipsize3; i++) {
		data[count + i] = ipresult[3][i+3-ipsize3];
	}
	count = count + ipsize3;
	data[count] = 0x3a;
	data[count + 1] = '1';
	data[count + 2] = '2';
	data[count + 3] = '5';
	data[count + 4] = '0';
	data[count + 5] = '0';

    espconn_send(&udp_client, data, sizeof(data));

}

void udpClient(void*arg)
{
	uint8 udp_server_ip[4] = { 255, 255, 255, 255 };//广播
	uint8 udp_local_ip[4];//本地ip
    static esp_udp udp;
    struct ip_info info;
    udp_client.type = ESPCONN_UDP;
    udp_client.proto.udp = &udp;
    udp.remote_port = UDP_SERVER_LOCAL_PORT;//远程端口号
    udp.local_port = 12500;//本地发送端口号
    memcpy(udp.remote_ip, udp_server_ip, sizeof(udp_server_ip));
    wifi_get_ip_info(STATION_IF,&info);//获取本地ip地址;
    udp_local_ip[0]=info.ip.addr&0x000000ff;
    udp_local_ip[1]=(info.ip.addr&0x0000ff00)>>8;
    udp_local_ip[2]=(info.ip.addr&0x00ff0000)>>16;
    udp_local_ip[3]=(info.ip.addr&0xff000000)>>24;
    memcpy(udp.local_ip, udp_local_ip, sizeof(udp_local_ip));
    uint8 i = 0;
    os_printf("\n remote ip\n");
    for (i = 0; i <= 3; i++) {
        os_printf("%u.", udp.remote_ip[i]);
    }
    os_printf("\n");
    espconn_regist_recvcb(&udp_client, UdpRecvCb);//接收回调
    espconn_regist_sentcb(&udp_client, UdpSendCb);//发送回调
    int8 res = 0;
    res = espconn_create(&udp_client);

    if (res != 0) {
        DBG_PRINT("UDP CLIENT CREAT ERR ret:%d\n", res);
    }

    os_timer_disarm(&time1);
    os_timer_setfn(&time1, t1Callback, NULL);
    os_timer_arm(&time1, 3000, 1);

    vTaskDelete(NULL);

}



