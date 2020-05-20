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
	Serial.println(second());
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
	
	#ifdef WIFI_DEBUG
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

	#ifdef WIFI_DEBUG
	Serial.print(millis());
	Serial.print(" Trying to connect to WiFi. SSID: ");  
	Serial.println(ssid);
	Serial.print("[");
	#endif

	while (WiFi.status()!=WL_CONNECTED) 
	{
		#ifdef WIFI_DEBUG
		Serial.print(".");
		#endif

		retr++;
		
		if (retr>=RETRIES_WIFI)
		{
			// Too many retries with same connection status: something is gone wrong
			#ifdef WIFI_DEBUG
				Serial.println();
				Serial.println("TOO MANY RETRIES!");
				Serial.println("Maybe WiFi settings are wrong");
				Serial.println("Or WiFi is not available");
			#endif
		}

		retr++;
		delay(500);
	}

	#ifdef WIFI_DEBUG
	Serial.println("]");
	Serial.print(millis());
	Serial.print(" WiFi connected. ");
	Serial.print("Device IP address is: ");
	Serial.println(WiFi.localIP());
	#endif
}

bool NetworkManager::UpdateTime(bool forced)
{
	//TODO: Non deve stare qui
	int8_t prevDay=-1; // used for time refresh once a day or for forcing time update

	WiFiUDP udp; // UDP client for NTP server
	//EasyNTPClient ntpClient(udp, NTPServer, (TIME_OFFSET*60*60));
	EasyNTPClient ntpClient(udp, "it.pool.ntp.org", (TIME_OFFSET*60*60));
	


	static long lastChecked=0;
	static bool reCheck=false;
	unsigned long t=0;

	if (prevDay!=weekday() || forced) // day changed or forced to update
	{
		// if is a re-check, I'll do it every NTP_RETRY_MINUTES minutes
		if (reCheck)
		{
			// millis has 'rollovered' (can say it?!)
			if (millis()<lastChecked) lastChecked=millis();
			// NTP_RETRY_MINUTES minutes passed from the last check => try to re-get time
			long retrymillis=lastChecked+(NTP_RETRY_MINUTES*60*1000);
			if (millis()>retrymillis) 
			{
				Serial.print(millis());
				Serial.println(" Trying to update the clock, again!");
				unsigned long t=ntpClient.getUnixTime(); 
			}
		}
		else // it's no a re-check: it's the first time I check
		{
			Serial.print(millis());
			Serial.println(" Trying to update the clock for the first time today");
			t=ntpClient.getUnixTime();
		}

		if (t>0) // time is updated! yeah!
		{
			setTime(t);
			Serial.print(millis());
			Serial.print(" Clock successfully updated. ");
			Serial.print("t value=");
			Serial.println(t);

			//if (checkDST) 
			if(true) //TODO: Da ripristinare il controllo
			{
				Serial.print(millis());
				Serial.println(" We're in DST. I add an hour");
				adjustTime(3600); // add an hour if we're in DST
			}

			reCheck=false;
			Serial.print(millis());
			Serial.print(" Actual date/time: ");
			debugDate();
			prevDay=weekday(); // this will prevent further updating for today!
			return(true);
		}
		else
		{
			// time not updated, I'll recheck later...
			lastChecked=millis();
			reCheck=true;
			Serial.print(lastChecked);     
			Serial.println(" Clock not updated");
			return(false);
		}
	}
}
