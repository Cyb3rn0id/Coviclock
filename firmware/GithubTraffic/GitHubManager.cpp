#include "GitHubManager.hpp"

GitHubManager::GitHubManager() : NetworkManager()
{

}

GitHubManager::GitHubManager(IPAddress deviceIP, IPAddress net_gateway, IPAddress net_subnet, IPAddress net_dns1, IPAddress net_dns2) : NetworkManager(deviceIP, net_gateway, net_subnet, net_dns1, net_dns2)
{

}

GitHubManager::~GitHubManager()
{

}

String GitHubManager::GetGitHubTrafficData(char* gitHubApiHost, char* gitHubApiPath, char* hostFingerPrint, char* gitHubToken)
{
	int responseRowCounter = 0;
	int httpStatus = -1;
	String jsonResponse = "";

	WiFiClientSecure _wifiClient;
	_wifiClient.setFingerprint(hostFingerPrint);

	//TODO: Serve controllare se è già connesso???

	if(_wifiClient.connect(gitHubApiHost, 443))
    {
		#ifdef GITHUB_DEBUG
		Serial.print(millis());
		Serial.println(" Connected to host!");
		#endif

		_wifiClient.print(String("GET ") + gitHubApiPath + " HTTP/1.1\r\n" +
							"Host: " + gitHubApiHost + "\r\n" +
							"Authorization: " + gitHubToken + "\r\n" +
							"User-Agent: esp8266\r\n"+
							"Cache-Control: no-cache\r\n\r\n");

		#ifdef GITHUB_DEBUG
		Serial.print(millis());
		Serial.println(" wait response from the host:");
		#endif

		// Loop on the response row information
		while (_wifiClient.connected() || _wifiClient.available())
		{
			String tempRow = _wifiClient.readStringUntil('\n');

			#ifdef GITHUB_DEBUG
			Serial.print(String(responseRowCounter) + "[" + httpStatus + "] > ");
			Serial.println(tempRow);
			#endif

			// Check if response status is 200
			if(responseRowCounter==0 && tempRow.indexOf("200 OK")>0)
			{
				httpStatus = 200;
			}

			// Check if current row contain the json response
			if(httpStatus == 200 && tempRow.indexOf("{")>=0)// && tempRow.endsWith("}\r\n"))
			{
				jsonResponse = tempRow;

				#ifdef GITHUB_DEBUG
				Serial.print("FOUND DATA > ");
				Serial.println(jsonResponse);
				#endif
			}

			// Increase response row counter
			responseRowCounter++;
		}
	}
	else
	{
		#ifdef GITHUB_DEBUG
		Serial.print(millis());
		Serial.println(" Cannot connect to host. Sorry");
		#endif
	}

	_wifiClient.stop();

	return jsonResponse;
}
