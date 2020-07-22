/** 
 * GitHub Traffic v1.3.0
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

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "GitHubManager.hpp"
#include "mysettings.hpp"

#define BUZZER  D8
#define S1      D1
#define S2      D6
// Display connections
#define TFT_CS    D2
#define TFT_RST   D3
#define TFT_DC    D0
// other 2 pins of display are connected to the SPI hardware port and cannot be
// specified in the library initialization:
// MOSI => D7
// SCK => D5
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Use this instance type for network with DHCP
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

int8_t lastDayUpdateTime = -1;
int8_t lastDataUpdateHour = -1;
int8_t repoToUpdate = 0;
bool updateTrafficData = false;
GitHubManager::Clone clonesTraffic[REPOSITORY_NUMBER];
GitHubManager::View viewsTraffic[REPOSITORY_NUMBER];

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

// prints the clock on the display
void PrintClock(void)
{
	String wd[7]={"Domenica ","Lunedi   ","Martedi  ","Mercoledi","Giovedi  ","Venerdi  ","Sabato   "}; // name of weekdays, 9 chars max length in italian
	String mo[12]={"GEN","FEB","MAR","APR","MAG","GIU","LUG","AGO","SET","OTT","NOV","DIC"}; // name of months

	// write time
	//tft.setCursor(0, 243); // x,y
	tft.setCursor(0, 232); // x,y
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);  
	tft.setTextSize(4);
	// H,m and s are in number format, so I must add a zero in front of
	if (hour()<10) tft.print("0");
	tft.print(hour());
	tft.print(":");
	if (minute()<10) tft.print("0");
	tft.print(minute());
	/*tft.setTextSize(2);
	tft.print(":");
	if (second()<10) tft.print("0");
	tft.print(second());
	tft.print(" "); // sometimes probably a bug prints a further 0*/

	tft.println();
	tft.setTextSize(3);
	tft.print(wd[weekday()-1]);
	tft.println();
	if (day()<10) tft.print("0");
	tft.print(day());
	tft.print(" ");
	tft.print(mo[month()-1]);
	tft.print(" ");
	tft.print(year()); // use year()-2000 for showing only the last 2 numbers
	tft.println();
	tft.setTextSize(1);
	tft.setTextColor(ILI9341_GREEN);
	//tft.println("(c)2020 Roberto D'Amico");
	tft.print("https://bobboteck.github.io/");

	tft.setCursor(120, 246);
	tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);  
	tft.setTextSize(2);
	tft.print(":");
	if (second()<10) tft.print("0");
	tft.print(second());
	tft.print(" ");
	
}

void printTemperature(float tval)
{
	tft.setCursor(180,250);
	tft.setTextSize(2);
	tft.setTextColor(ILI9341_PINK,ILI9341_BLACK);
	tft.print(tval,1);
	tft.setTextSize(1);
	tft.setCursor(228,250);
	tft.print("o"); // looks like a grade symbol?! LOL
}

void SetDisplayColor(int newValue, int oldValue)
{
	if(newValue > oldValue)
	{
		tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
	}
	else if(newValue < oldValue)
	{
		tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
	}
	else
	{
		tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
	}
}

void PrintBootTrafficData(void)
{
	String jsonData;
	String countsString = "";
	String uniquesString = "";
	int fieldPosition = 0;
	int colonPosition = 0;
	int commaPosition = 0;

	tft.println("Updating data ...");
	Serial.println("Updating data ...");
	// Loop for get data from GitHub repositories
	for(int i=0;i<REPOSITORY_NUMBER;i++)
	{
		// Get clone traffic data for current Repository
		clonesTraffic[i] = github.GetGitHubTrafficCloneData(GITHUB_USERNAME, repositories[i], GITHUB_FINGER_PRINT, GITHUB_TOKEN);
		tft.print(".");
		// Get view traffic data for current Repository
		viewsTraffic[i] = github.GetGitHubTrafficViewData(GITHUB_USERNAME, repositories[i], GITHUB_FINGER_PRINT, GITHUB_TOKEN);
		tft.print(".");
	}

	// Write title data
	tft.fillScreen(ILI9341_BLACK);
	tft.setCursor(0,0);
	tft.setTextColor(ILI9341_WHITE);  
	tft.setTextSize(2);
	tft.println("GitHub Traffic Clock");
	tft.setTextColor(ILI9341_LIGHTGREY);
	tft.setTextSize(1);
	tft.println("SW by Roberto D'Amico [@bobboteck]");
	tft.println("HW by Giovanni Bernardo [@settorezero]");
	tft.println();
	// Loop for print traffic data on display
	for(int i=0;i<REPOSITORY_NUMBER;i++)
	{
		tft.setTextColor(ILI9341_YELLOW);
		tft.setTextSize(1);
		tft.println(repositories[i]);

		tft.setTextColor(ILI9341_LIGHTGREY);
		tft.setTextSize(1);
		tft.print("Clones: ");
		//tft.print(clones[i][0]);
		tft.print(clonesTraffic[i].total);
		tft.print(" (");
		//tft.print(clones[i][1]);
		tft.print(clonesTraffic[i].cloner);
		tft.print(") - Views: ");
		//tft.print(views[i][0]);
		tft.print(viewsTraffic[i].total);
		tft.print(" (");
		//tft.print(views[i][1]);
		tft.print(viewsTraffic[i].visitors);
		tft.println(")");
		tft.println();
	}

	//tft.drawFastHLine(0,40,200,ILI9341_MAGENTA);
	//tft.drawFastHLine(0,262,240,ILI9341_MAGENTA);
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

	tft.begin();
	tft.setRotation(0);
	tft.fillScreen(ILI9341_BLACK);
	tft.setCursor(0,0);
	tft.setTextColor(ILI9341_LIGHTGREY);
	tft.setTextSize(1);
	tft.println("GitHub Traffic 1.3.0");
	tft.println("SW by Roberto D'Amico [@bobboteck]");
	tft.println("HW by Giovanni Bernardo [@settorezero]");
	tft.println();
	tft.println("Start connection to WiFi: ");
	tft.println(WIFI_SSID);
	github.Connect(WIFI_SSID, WIFI_PWD);
	tft.println("Wifi connected!");

	tft.println("Update Time ...");
	lastDayUpdateTime = github.UpdateTime(lastDayUpdateTime);
	// Check update Time status
	if(lastDayUpdateTime >= 0)
	{
		tft.println("Time updated.");
	}
	else
	{
		tft.println("Cannot update Time");
	}
	
	tft.println("Repository to check:");
	tft.setTextColor(ILI9341_YELLOW);
	tft.setTextSize(1);
	for(int i=0;i<8;i++)
	{
		tft.println(repositories[i]);
	}

	tft.setTextColor(ILI9341_LIGHTGREY);
	tft.println();
	tft.println("Ready");

	PrintBootTrafficData();
	lastDataUpdateHour = hour();
}

void loop(void) 
{
	static uint8_t tempCount=0; // counter for analog reading of temperature
	static float temp=0; // temperature value

	PrintClock();
	delay(100);

	// read the temperature, one time for every loop iteraction
	float tempTemp = analogRead(A0);
	Serial.println(tempTemp);
	temp += tempTemp;

	tempCount++;
	if (tempCount==32)
	{
		temp=temp/32;										// average on 32 values
		float mVoltReadFromADC0 = temp * 0.9765625;			// 1V / 1024 = 0.0009765625V = 0.9765625mV
		float mVoltOnPinA0 = mVoltReadFromADC0 / 0.3125;	// 0.3125==(R2/(R1+R2)) voltage divider on NODE MCU Board
		float mVoltSensorTemp = mVoltOnPinA0 - 400;			// 400mV is the 0°C value for the sensor
		float sensorTemp = mVoltSensorTemp / 19.5;			// 19.5mV for each °C
		printTemperature(sensorTemp);

		temp=0;
		tempCount=0;
	}

	if(lastDataUpdateHour < hour())
	{
		lastDataUpdateHour = hour();
		updateTrafficData = true;
	}

	if(updateTrafficData && repoToUpdate < REPOSITORY_NUMBER)
	{
		// Print "Updating ..." next to the repo title row on display
		tft.setCursor(0, 40 + (repoToUpdate * 24));
		tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
		tft.setTextSize(1);
		tft.print(repositories[repoToUpdate]);
		tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
		tft.print("  Updating.");

		GitHubManager::Clone cloneData = github.GetGitHubTrafficCloneData(GITHUB_USERNAME, repositories[repoToUpdate], GITHUB_FINGER_PRINT, GITHUB_TOKEN);
		tft.print(".");
		GitHubManager::View viewData = github.GetGitHubTrafficViewData(GITHUB_USERNAME, repositories[repoToUpdate], GITHUB_FINGER_PRINT, GITHUB_TOKEN);
		tft.print(".");

		tft.setCursor(0, 40 + (repoToUpdate * 24));
		tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
		tft.setTextSize(1);
		tft.print(repositories[repoToUpdate]);
		// Delete "  Updating..." string from display
		tft.print("             ");
		tft.setCursor(0, 48 + (repoToUpdate * 24));
		tft.setTextSize(1);

		tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
		tft.print("Clones: ");
		SetDisplayColor(cloneData.total, clonesTraffic[repoToUpdate].total);
		tft.print(cloneData.total);
		clonesTraffic[repoToUpdate].total = cloneData.total;

		tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
		tft.print(" (");
		SetDisplayColor(cloneData.cloner, clonesTraffic[repoToUpdate].cloner);
		tft.print(cloneData.cloner);
		clonesTraffic[repoToUpdate].cloner = cloneData.cloner;

		tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
		tft.print(") - Views: ");
		SetDisplayColor(viewData.total, viewsTraffic[repoToUpdate].total);
		tft.print(viewData.total);
		viewsTraffic[repoToUpdate].total = viewData.total;

		tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
		tft.print(" (");
		SetDisplayColor(viewData.visitors, viewsTraffic[repoToUpdate].visitors);
		tft.print(viewData.visitors);
		viewsTraffic[repoToUpdate].visitors = viewData.visitors;

		tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
		tft.print(")        ");

		repoToUpdate++;
	}
	else
	{
		updateTrafficData = false;
		repoToUpdate = 0;
	}
	

	lastDayUpdateTime = github.UpdateTime(lastDayUpdateTime);
}