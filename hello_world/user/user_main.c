/*
	The hello world demo
 */



#include <c_types.h>
#include <ip_addr.h>
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>
#include <espconn.h>
#include "driver/uart.h"
#include "user_interface.h"
#include "flashing.h"

#define DELAY 1000 /* milliseconds */

LOCAL os_timer_t servoTimer;
LOCAL os_timer_t hello_timer;
LOCAL os_timer_t start_timer;
extern int ets_uart_printf(const char *fmt, ...);


struct espconn conn;
esp_udp udp1;

unsigned short position[4];
unsigned int signalStange = 0;
LOCAL void ICACHE_FLASH_ATTR recvCB(void *arg, char *pData, unsigned short len)
{
	struct espconn *pConn = (struct espconn*) arg;
	unsigned short i = 0, j=0;
	while((j < len) && (i<4))
	{
		if(pData[j++] == ';')
		{
			position[i] = strtol(&pData[j], NULL, 10);
			ets_uart_printf("position:%d, %d\n",i,position[i]);
			if(position[i] > 100)
				position[i]  = 100;
			i++;
		}
	}
	char buffer[32];
	os_sprintf(buffer, "%d", signalStange);

	remot_info *remote = NULL;
	sint8 err;
	err = espconn_get_connection_info(pConn,&remote,0);
	ets_uart_printf("%d.%d.%d.%d:%d\n",remote->remote_ip[0],remote->remote_ip[1],remote->remote_ip[2],remote->remote_ip[3], remote->remote_port);
	//ets_uart_printf("%d.%d.%d.%d:%d\n",pConn->proto.udp->local_ip[0],pConn->proto.udp->local_ip[1],pConn->proto.udp->local_ip[2],pConn->proto.udp->local_ip[3], pConn->proto.udp->local_port);

	memcpy(pConn->proto.udp->remote_ip,remote->remote_ip,4);
	pConn->proto.udp->remote_port = remote->remote_port;
	espconn_send(pConn, buffer, strlen(buffer));
	ets_uart_printf("rcv");
}

int servoRunning = 1;
LOCAL void ICACHE_FLASH_ATTR hello_cb(void *arg)
{
	static int first = 0;
	//	ets_uart_printf("---\r\n");
	//	if(wifi_station_get_connect_status() == STATION_GOT_IP)
	//	{
	if(first == 0)
	{
		first = 1;


		ets_uart_printf("Connected\r\n");

	}
	else
	{
		//get rssi of connected ap
		signalStange = wifi_station_get_rssi();
		if(wifi_softap_get_station_num() > 0)
		{
			//			servoRunning = 0;
			ets_uart_printf("client 1\n");
		}
		else
		{
			//			servoRunning = 1;
			ets_uart_printf("client 0\n");
		}
	}
	//	}
	//	else
	//		ets_uart_printf("Waitforconnection\r\n");


}
LOCAL void ICACHE_FLASH_ATTR servo_cb(void *arg)
{
	static unsigned short servo =50;
	static int first = 0;
	if(first == 0)
	{
		first = 1;
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3);
		gpio_output_set(0, 0, BIT3 | BIT2 | BIT1|BIT0, 0);
	}
	if(first == 1)
	{
		gpio_output_set(BIT2, 0, 0, 0);
		os_delay_us(1000 + (position[0]*10));
		gpio_output_set(0, BIT2, 0, 0);
		first = 2;
	}
	else if(first == 2)
	{
		gpio_output_set(BIT3, 0, 0, 0);
		os_delay_us(1000 + (position[1]*10));
		gpio_output_set(0, BIT3, 0, 0);
		first = 3;
	}
	else if(first == 3)
	{
		gpio_output_set(BIT1, 0, 0, 0);
		os_delay_us(1000 + (position[2]*10));
		gpio_output_set(0, BIT1, 0, 0);
		first = 4;
	}
	else if(first == 4)
	{
		gpio_output_set(BIT0, 0, 0, 0);
		os_delay_us(1000 + (position[3]*10));
		gpio_output_set(0, BIT0, 0, 0);
		first = 1;
	}
}

void user_rf_pre_init(void)
{
}

LOCAL void ICACHE_FLASH_ATTR start_cb(void *arg)
{
	ets_uart_printf("HELLO\n");
	// Configure the UART
	uart_init(BIT_RATE_74880, BIT_RATE_74880);
	// Set up a timer to send the message
	// os_timer_disarm(ETSTimer *ptimer)
	os_timer_disarm(&hello_timer);
	os_timer_disarm(&servoTimer);
	// os_timer_setfn(ETSTimer *ptimer, ETSTimerFunc *pfunction, void *parg)
	os_timer_setfn(&hello_timer, (os_timer_func_t *)hello_cb, (void *)0);
	os_timer_setfn(&servoTimer, (os_timer_func_t *)servo_cb, (void *)0);
	// void os_timer_arm(ETSTimer *ptimer,uint32_t milliseconds, bool repeat_flag)
	os_timer_arm(&hello_timer, DELAY, 1);
	os_timer_arm(&servoTimer, 5, 1);
	memset(position, 0, sizeof(position));
	wifi_station_connect();
	flashServerInit();
}

void user_init(void)
{
	wifi_set_opmode(STATIONAP_MODE);
	//starting tcp server
	//wifi_softap_dhcps_start();
	struct softap_config config;
	config.authmode=AUTH_OPEN;
	config.max_connection = 4;
	config.beacon_interval = 100;
	config.ssid_hidden=0;
	os_strcpy((char*)config.password,"");
	os_strcpy((char*)config.ssid,"RcPlanettte");
	config.ssid_len = strlen("RcPlanettte");
	wifi_softap_set_config(&config);
	os_timer_setfn(&start_timer, (os_timer_func_t *)start_cb, (void *)0);
	os_timer_arm(&start_timer, 10, 0);

	conn.type = ESPCONN_UDP;
	conn.state = ESPCONN_NONE;
	udp1.local_port = 5050;
	conn.proto.udp = &udp1;
	espconn_create(&conn);
	espconn_regist_recvcb(&conn, recvCB);
}
