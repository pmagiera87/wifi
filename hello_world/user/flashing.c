#include <c_types.h>
#include <ip_addr.h>
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>
#include <espconn.h>
#include "flashing.h"
#include "user_interface.h"
#include "upgrade.h"
static struct espconn mainConn;
static esp_tcp mainTcp;

static void ICACHE_FLASH_ATTR serverSentCb(void *arg) {

}
static void ICACHE_FLASH_ATTR serverDisconCb(void *arg) {
	//The esp sdk passes through wrong arg here, namely the one of the *listening* socket.
	//That is why we can't use (HttpdConnData)arg->sock here.
	//Just look at all the sockets and kill the slot if needed.

}
static void ICACHE_FLASH_ATTR serverReconCb(void *arg, sint8 err) {

}

uint32_t flashoffset;
uint32_t flashaddress;
int flashState;
static struct station_config stconf;
static void ICACHE_FLASH_ATTR serverRecvCb(void *arg, char *data, unsigned short len)
{
	struct espconn *conn=arg;
	static char moduloBuff[4];
	uint32 buffer32[410];//1024bajt
	char buff[128];
	if(flashState == 2)
	{
		if(flashoffset==0)
		{

		}
		int ret = spi_flash_write(flashaddress, (uint32 *)data, len);
		os_sprintf(buff, "%d;%d,",ret,flashoffset);
		espconn_send(conn, buff, strlen(buff));
		flashaddress+=len;
		flashoffset+=len;
		return;
	}
	flashState = strtol(data,0,10);
	if(flashState == 0)
	{
		os_sprintf(buff, "flashState=%d",flashState);
		espconn_send(conn, buff, strlen(buff));
	}
	else if(flashState == 1)
	{
		int sector;
		for(sector=flashaddress; sector<(flashaddress+0x5B000);sector+=SPI_FLASH_SEC_SIZE)
			spi_flash_erase_sector(sector/SPI_FLASH_SEC_SIZE);
		espconn_send(conn, "Erase ok", strlen("Erase ok"));
	}
	else if(flashState == 3)
	{
		espconn_send(conn, "Reboot", strlen("Reboot"));
		system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
		system_upgrade_reboot();
	}
	else if(flashState == 4)
	{
		memset((char*)stconf.ssid, 0,32);
		memcpy((char*)stconf.ssid, &data[1], len-1);
		espconn_send(conn, "Set ssid", strlen("Set ssid"));
	}
	else if(flashState == 5)
	{
		memset((char*)stconf.password, 0,64);
		memcpy((char*)stconf.password, &data[1], len-1);
		wifi_station_set_config(&stconf);
		wifi_station_connect();
		espconn_send(conn, "set passwd", strlen("set passwd"));
	}
}

static void ICACHE_FLASH_ATTR serverConnectCb(void *arg)
{
	struct espconn *conn=arg;
	flashoffset = 0;
	flashState = 0;
	espconn_regist_recvcb(conn, serverRecvCb);
	espconn_regist_reconcb(conn, serverReconCb);
	espconn_regist_disconcb(conn, serverDisconCb);
	espconn_regist_sentcb(conn, serverSentCb);
	int id=system_upgrade_userbin_check();
	if (id==0)
	{
		flashaddress = 0x81000;
		espconn_send(conn, "user2.bin", strlen("user2.bin"));
	}
	else
	{
		flashaddress = 0x01000;
		espconn_send(conn, "user1.bin", strlen("user1.bin"));
	}
}
void ICACHE_FLASH_ATTR flashServerInit(void)
{

	mainConn.proto.tcp=&mainTcp;
	mainConn.type=ESPCONN_TCP;
	mainConn.state=ESPCONN_NONE;
	mainConn.proto.tcp->local_port = 8889;
	espconn_regist_connectcb(&mainConn, serverConnectCb);
	espconn_accept(&mainConn);
}
