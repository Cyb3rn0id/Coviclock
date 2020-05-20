#ifndef GitHubManager_H
#define GitHubManager_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <EasyNTPClient.h>
#include <TimeLib.h>
#include "NetworkManager.hpp"

#define GITHUB_DEBUG

class GitHubManager : public NetworkManager
{
	private:

	public:
		/**
		 * Costructor 
		 * 
		 * Use this costructor for use DHCP
		 * 
		 */
		GitHubManager();

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
		GitHubManager(IPAddress deviceIP, IPAddress net_gateway, IPAddress net_subnet, IPAddress net_dns1, IPAddress net_dns2);
		~GitHubManager();


		/**
		 * Get data from GitHub
		 * 
		 * Get private data from GitHub with your personal Token, you need to 
		 * generate it from your settings pannel.
		 * More info at: 
		 * 
		 * @param gitHubApiHost The Host name of GitHub API
		 * @param gitHubApiPath The path of REST API to call
		 * @param gitHubFingerPrint 
		 * @param gitHubToken 
		 * 
		 * @return String of the JSON response
		 * 
		 */
		String GetGitHubTrafficData(char* gitHubApiHost, char* gitHubApiPath, char* gitHubFingerPrint, char* gitHubToken);
};

#endif
