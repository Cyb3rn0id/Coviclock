/** 
 * GitHub Traffic 
 * This file is part of the Coviclock project (https://github.com/Cyb3rn0id/Coviclock)
 * Copyright (c) 2020 Roberto D'Amico (Bobboteck - https://bobboteck.github.io/)
 * 
 * Based on project:
 * Coviclock
 * (c)2020 Giovanni Bernardo (https://www.settorezero.com)
 * Repository Url: https://github.com/Cyb3rn0id/Coviclock
 * Full article:  https://www.settorezero.com/wordpress/coviclock-informazioni-tempo-reale-coronavirus-covid19
 * 
 * LICENSE
 * Attribution-NonCommercial-ShareAlike 4.0 International 
 * (CC BY-NC-SA 4.0)
 * 
 * This is a human-readable summary of (and not a substitute for) the license:
 * You are free to:
 * SHARE — copy and redistribute the material in any medium or format
 * ADAPT — remix, transform, and build upon the material
 * The licensor cannot revoke these freedoms as long as you follow the license terms.
 * Under the following terms:
 * ATTRIBUTION — You must give appropriate credit, provide a link to the license, 
 * and indicate if changes were made. You may do so in any reasonable manner, 
 * but not in any way that suggests the licensor endorses you or your use.
 * NON COMMERCIAL — You may not use the material for commercial purposes.
 * SHARE ALIKE — If you remix, transform, or build upon the material,
 * you must distribute your contributions under the same license as the original.
 * NO ADDITIONAL RESTRICTIONS — You may not apply legal terms or technological 
 * measures that legally restrict others from doing anything the license permits.
 * 
 * Warranties
 * The Licensor offers the Licensed Material as-is and as-available, and makes
 * no representations or warranties of any kind concerning the Licensed Material, 
 * whether express, implied, statutory, or other. This includes, without limitation, 
 * warranties of title, merchantability, fitness for a particular purpose, 
 * non-infringement, absence of latent or other defects, accuracy, or the presence
 * or absence of errors, whether or not known or discoverable. Where disclaimers 
 * of warranties are not allowed in full or in part, this disclaimer may not apply to You.
 * 
 * Please read the Full License text at the following link:
 * https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode
 */

#include "GitHubManager.hpp"

#define BUZZER  D8
#define S1      D1
#define S2      D6
#define WIFI_SSID			"YOUR_WIFI_SSID_STRING"
#define WIFI_PWD			"TOUR_WIFI_PWD_STRING"
#define GITHUB_API_HOST		"api.github.com"
#define GITHUB_FINGER_PRINT	"5974618813ca1234154d110ac17fe667076942f5"			// Scade il 16/07/2020
#define GITHUB_TOKEN		"token ..."

// Use this istruction for network with DHCP
GitHubManager github;
/*
// Use the code below for static ip configuration
IPAddress deviceIP(192, 168, 1, 166); // ESP8266 static address
IPAddress gateway(192, 168, 1, 1); // router address
IPAddress subnet(255, 255, 255, 0); // subnet mask
IPAddress dns1 (8, 8, 8, 8); // first DNS, required for easyntp with static ip (https://github.com/aharshac/EasyNTPClient/issues/4)
IPAddress dns2 (8, 8, 4, 4); // second DNS, required for easyntp with static ip
GitHubManager github = GitHubManager(deviceIP, gateway, subnet, dns1, dns2);
*/

// interrupt vector on button S1 pressing
ICACHE_RAM_ATTR void Switch1_ISR(void)
{
	Serial.println("Switch ONE");
}

// interrupt vector on button S2 pressing
ICACHE_RAM_ATTR void Switch2_ISR(void)
{
	Serial.println("Switch TWO");
}

void setup() 
{
	pinMode(BUZZER,OUTPUT);
	digitalWrite(BUZZER,LOW);	// Set the buzzer off
	pinMode(S1,INPUT_PULLUP);
	pinMode(S2,INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(S1), Switch1_ISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(S2), Switch2_ISR, FALLING);

	Serial.begin(115200);
	Serial.println();
	Serial.print(millis());
	Serial.println(" GitHub Traffic monitor software by Roberto D'Amico, based on Coviclock by Giovanni Bernardo");

	github.Connect(WIFI_SSID, WIFI_PWD);
	
	github.UpdateTime(false);

	String jsonData = github.GetGitHubTrafficData(GITHUB_API_HOST, "/repos/bobboteck/jetbot_ros_webconsole/traffic/clones", GITHUB_FINGER_PRINT, GITHUB_TOKEN);
	Serial.print(millis());
	Serial.print(" - ");
	Serial.println(jsonData);
}

void loop(void) 
{
	//String jsonData = github.GetGitHubTrafficData(GITHUB_API_HOST, "/repos/bobboteck/jetbot_ros_webconsole/traffic/clones", GITHUB_FINGER_PRINT, GITHUB_TOKEN);
}