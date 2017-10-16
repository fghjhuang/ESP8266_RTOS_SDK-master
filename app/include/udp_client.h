#include "user_config.h"

//const uint8 udp_server_ip[4]={192,168,1,100};
//const uint8 udp_server_ip[4] = { 255, 255, 255, 255 };

#define UDP_SERVER_LOCAL_PORT (9050)

/*Sta Connect ap config
 #define AP_CONNECT_SSID      "come on baby"
 #define AP_CONNECT_PASSWORD  "1234567890"
 //Softap config
 #define SOFTAP_SSID "ap_test"
 #define SOFTAP_PASSWORD "123456789"
 #define SOFTAP_CHANNEL 5
 */

#define DBG_PRINT(fmt,...)	do{\
	    os_printf("[Dbg]");\
	    os_printf(fmt,##__VA_ARGS__);\
	}while(0)

#define ERR_PRINT(fmt,...) do{\
	    os_printf("[Err] Fun:%s Line:%d ",__FUNCTION__,__LINE__);\
	    os_printf(fmt,##__VA_ARGS__);\
	}while(0)
#define DBG_LINES(v) os_printf("------------------%s---------------\n",v)

