#include "NetworkManager.hpp"


void debugDate(void)
{
	String wd[7]={"Domenica ","Lunedi   ","Martedi  ","Mercoledi","Giovedi  ","Venerdi  ","Sabato   "}; // name of weekdays, 9 chars max length in italian
	String mo[12]={"GEN","FEB","MAR","APR","MAG","GIU","LUG","AGO","SET","OTT","NOV","DIC"}; // name of months

	Serial.print(wd[weekday()-1]);
	Serial.print(" ");
	Serial.print(day());
	Serial.print(" ");
	Serial.print(mo[month()-1]);
	Serial.print(" ");
	Serial.print(year());
	Serial.print(" ");
	if (hour()<10) Serial.print('0');
	Serial.print(hour());
	Serial.print(":");
	if (minute()<10) Serial.print('0');
	Serial.print(minute());
	Serial.print(":");
	if (second()<10) Serial.print('0');
	//Serial.println(second());
	Serial.print(second());

	Serial.print(" --- ");

	Serial.print(weekday());
	Serial.print(" ");
	Serial.print(day());
	Serial.print("/");
	Serial.print(month());
	Serial.print("/");
	Serial.print(year());
	Serial.print(" ");
	Serial.print(hour());
	Serial.print(":");
	Serial.print(minute());
	Serial.print(":");
	Serial.println(second());
}

/**
 * checkDST
 * 
 * Check if actual time is Standard Time or Daylight Saving Time returns DST status 
 * (true if we're between the 2:00AM of the last Sunday of March and the 03:00AM of the last Sunday of October)
 * 
 */
bool checkDST(void)
{
	// In italy DST goes from last Sunday of March (we must set UTC+2 at 2:00AM)
	// until last sunday of October (we must set UTC+1 at 3:00AM)
	bool DST = false;

	// Month is Apr,May,Jun,Jul,Aug,Sep => for sure we're in DST
	if((month()>3) && (month()<10))  
	{
		DST = true;
	}

	// Month is March or October: we must check day
	if ((month()==3) || (month()==10))
	{
		// Last sunday can occurr only after day 24, March and October have both 31 days:
		// if 24 is Sunday, last Sunday will occurr on 31th
		if (day()<25) // if day is <25 and we're in March, cannot be DST, but if we're in October... yes!
		{
			DST=(month()==3?false:true);
		}
		// today is sunday or sunday is already passed
		// Sunday is 1 and Saturday is 7
		// the value (31-actual day number+weekday number) is a number from 1 to 7 if today is Sunday or
		// Sunday is already passed. Is a number between 8 and 13 if Sunday has to come
		if (((31-day())+weekday()) <8) // It's Sunday or Sunday is already passed
		{
			// today is Sunday and it's the 2:00AM or 2:00AM are passed if in March? 
			// or is Sunday and it's the 3:00AM or 3:00 AM are passed if in October?
			if (weekday()==1 && (((month()==3) && (hour()>1)) || ((month()==10) && (hour()>2)))) //&& hour()>1) // 
			{
				// If March, we're still in DST, if October, DST has ended
				DST=(month()==3?true:false);
			}
			else if (weekday()>1)	// it's not sunday, but sunday is passed
			{
				// If March, we're in DST, if October, DST has ended
				DST=(month()==3?true:false);
			}
			else	// it's Sunday but are not the 2:00AM in March nor the 3:00AM in October
			{
				// If March, no DST, if October, we're still in DST
				DST=(month()==3?false:true);
			}
		}
		else
		{
			// it's not sunday or sunday has to come
			// If March, no DST, if October, we're still in DST
			DST=(month()==3?false:true);
		}
	}

	// in all other cases there is no DST (Month is Jan,Feb,Nov,Dec)
	return (DST);
}


NetworkManager::NetworkManager()
{
	_useStaticIP = false;
}

NetworkManager::NetworkManager(IPAddress deviceIP, IPAddress net_gateway, IPAddress net_subnet, IPAddress net_dns1, IPAddress net_dns2)
{
	_deviceIP = deviceIP;
	_net_gateway = net_gateway;
	_net_subnet = net_subnet;
	_net_dns1 = net_dns1;
	_net_dns2 = net_dns2;
	_useStaticIP = true;
}

NetworkManager::~NetworkManager()
{

}


void NetworkManager::Connect(char* ssid, char* password)
{
	static uint16_t retr=0; //re-connection retries counter
	
	#ifdef NETWORK_DEBUG
	Serial.println("Connecting to WiFi");
	Serial.print(">");
	Serial.println(ssid);
	#endif

	// workaround for issue #2186 (ESP8266 doesn't connect to WiFi with release 2.3.0)
	// https://github.com/esp8266/Arduino/issues/2186#issuecomment-228581052
	WiFi.persistent(false);
	WiFi.mode(WIFI_OFF);
	WiFi.mode(WIFI_STA);

	if(_useStaticIP)
	{
		WiFi.config(_deviceIP, _net_gateway, _net_subnet, _net_dns1, _net_dns2);
	}

	WiFi.begin(ssid, password);

	#ifdef NETWORK_DEBUG
	Serial.print(millis());
	Serial.print(" Trying to connect to WiFi. SSID: ");  
	Serial.println(ssid);
	Serial.print("[");
	#endif

	while (WiFi.status()!=WL_CONNECTED) 
	{
		#ifdef NETWORK_DEBUG
		Serial.print(".");
		#endif

		retr++;
		
		if (retr>=RETRIES_WIFI)
		{
			// Too many retries with same connection status: something is gone wrong
			#ifdef NETWORK_DEBUG
				Serial.println();
				Serial.println("TOO MANY RETRIES!");
				Serial.println("Maybe WiFi settings are wrong");
				Serial.println("Or WiFi is not available");
			#endif
		}

		retr++;
		delay(500);
	}

	#ifdef NETWORK_DEBUG
	Serial.println("]");
	Serial.print(millis());
	Serial.print(" WiFi connected. ");
	Serial.print("Device IP address is: ");
	Serial.println(WiFi.localIP());
	#endif
}

int8_t NetworkManager::UpdateTime(int8_t lastDayUpdate)
{
	// UDP client for NTP server
	WiFiUDP udp;
	EasyNTPClient ntpClient(udp, NTP_SERVER, (TIME_OFFSET*60*60));

	static long lastChecked = 0;
	static bool reCheck = false;
	unsigned long unixTime = 0;
	int8_t updatedInDay = -1;

 	// If day changed need to update time
	if(lastDayUpdate != weekday())
	{
		#ifdef NETWORK_DEBUG
		Serial.print(millis());
		Serial.println(" Trying to update the clock for the first time today");
		#endif

		unixTime=ntpClient.getUnixTime();

 		// time is updated! yeah!
		if(unixTime > 0)
		{
			setTime(unixTime);

			#ifdef NETWORK_DEBUG
			Serial.print(millis());
			Serial.print(" Clock successfully updated. Time value=");
			Serial.println(unixTime);
			#endif

			if(checkDST()) 
			{
				#ifdef NETWORK_DEBUG
				Serial.print(millis());
				Serial.println(" We're in DST. I add an hour");
				#endif
				// add an hour if we're in DST
				adjustTime(3600);
			}

			#ifdef NETWORK_DEBUG
			Serial.print(millis());
			Serial.print(" Actual date/time: ");
			debugDate();
			#endif

			reCheck=false;
			//prevDay=weekday(); // this will prevent further updating for today!
			updatedInDay = weekday();
			//return(prevDay);
		}
		else
		{
			#ifdef NETWORK_DEBUG
			Serial.print(lastChecked);     
			Serial.println(" Clock not updated");
			#endif

			// time not updated, I'll recheck later...
			lastChecked=millis();
			reCheck=true;
			return(false);

			updatedInDay = -2;
		}
	}
	else
	{
		updatedInDay = weekday();
	}
	
	return updatedInDay;
}
