#ifndef NetworkManager_H
#define NetworkManager_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <EasyNTPClient.h>
#include <TimeLib.h>

#define WIFI_DEBUG
#define RETRIES_WIFI 		100	// number of WiFi re-connection retries after a no-connection, at 500mS reconnection rate
#define NTP_RETRY_MINUTES	10	// if first NTP connection gone bad, I'll retry after those minutes
#define TIME_OFFSET			1

class NetworkManager
{
	private:
		bool _useStaticIP;
		IPAddress _deviceIP;
		IPAddress _net_gateway;
		IPAddress _net_subnet;
		IPAddress _net_dns1;
		IPAddress _net_dns2;
	public:
		/**
		 * Costructor 
		 * 
		 * Use this costructor for use DHCP
		 * 
		 */
		NetworkManager();

		/**
		 * Costructor 
		 * 
		 * Use this costructor for use static IP configuration
		 * 
		 * @param deviceIP IP address object for the IP of device
		 * @param net_gateway IP address object for the network Gateway
		 * @param net_subnet IP address object for the network subnet
		 * @param net_dns1 IP address object for the primary DNS
		 * @param net_dns2 IP address object for the secondary DNS
		 */
		NetworkManager(IPAddress deviceIP, IPAddress net_gateway, IPAddress net_subnet, IPAddress net_dns1, IPAddress net_dns2);
		~NetworkManager();
		
		/**
		 * Connect to WiFi network
		 * 
		 * Connect to network with indicated SSID and password
		 * 
		 * @param ssid the SSID of your network
		 * @param password the password of your network
		 * @return void
		 */
		void Connect(char* ssid, char* password);

		/**
		 * Update system Time 
		 * 
		 * Check the NTP server to update system time
		 * 
		 * @param forced to forse the Update
		 * @return void
		 */
		bool UpdateTime(bool forced);
};

#endif
