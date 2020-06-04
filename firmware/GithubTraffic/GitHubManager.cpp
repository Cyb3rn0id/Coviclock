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


String GitHubManager::GetGitHubTrafficData(char* gitHubApiHost, String gitHubApiPath, char* hostFingerPrint, char* gitHubToken)
{
	bool noDataFound = true;
	int responseRowCounter = 0;
	int httpStatus = -1;
	String jsonResponse = "";

	WiFiClientSecure _wifiClient;
	_wifiClient.setFingerprint(hostFingerPrint);

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
		while ((_wifiClient.connected() || _wifiClient.available()) && noDataFound)
		{
			String tempRow = _wifiClient.readStringUntil('\n');

			#ifdef GITHUB_DEBUG
			Serial.print(String(responseRowCounter) + "[" + httpStatus + "] (" + millis() + ") > ");
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
				noDataFound = false;

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

GitHubManager::Clone GitHubManager::GetGitHubTrafficCloneData(char* gitHubUserName, char* gitHubRepository, char* hostFingerPrint, char* gitHubToken)
{
	bool noDataFound = true;
	int responseRowCounter = 0;
	int httpStatus = -1;
	String jsonResponse = "";
	String countsString = "";
	String uniquesString = "";
	int fieldPosition = 0;
	int colonPosition = 0;
	int commaPosition = 0;
	GitHubManager::Clone result = { 0, 0 };

	WiFiClientSecure _wifiClient;
	_wifiClient.setFingerprint(hostFingerPrint);

	if(_wifiClient.connect(GITHUB_API_HOST, 443))
    {
		#ifdef GITHUB_DEBUG
		Serial.print(millis());
		Serial.println(" Connected to host!");
		#endif
		//String("/repos/") + GITHUB_USERNAME + "/" + repositories[i]  + "/traffic/clones"
		_wifiClient.print(String("GET /repos/") + gitHubUserName + "/" + gitHubRepository + "/traffic/clones" + " HTTP/1.1\r\n" +
							"Host: " + GITHUB_API_HOST + "\r\n" +
							"Authorization: " + gitHubToken + "\r\n" +
							"User-Agent: esp8266\r\n"+
							"Cache-Control: no-cache\r\n\r\n");

		#ifdef GITHUB_DEBUG
		Serial.print(millis());
		Serial.println(" wait response from the host:");
		#endif

		// Loop on the response row information
		while ((_wifiClient.connected() || _wifiClient.available()) && noDataFound)
		{
			String tempRow = _wifiClient.readStringUntil('\n');

			#ifdef GITHUB_DEBUG
			Serial.print(String(responseRowCounter) + "[" + httpStatus + "] (" + millis() + ") > ");
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
				noDataFound = false;

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

	if(jsonResponse != "")
	{
		// Get count of Clones in the last two week
		fieldPosition = jsonResponse.indexOf("count");
		colonPosition = jsonResponse.indexOf(":", fieldPosition);
		commaPosition = jsonResponse.indexOf(",", colonPosition);
		countsString = jsonResponse.substring(colonPosition + 1, commaPosition);
		//clones[i][0] = countsString.toInt();
		result.total = countsString.toInt();
		// Get count of unique Clones in the last two week
		fieldPosition = jsonResponse.indexOf("uniques");
		colonPosition = jsonResponse.indexOf(":", fieldPosition);
		commaPosition = jsonResponse.indexOf(",", colonPosition);
		uniquesString = jsonResponse.substring(colonPosition + 1, commaPosition);
		//clones[i][1] = uniquesString.toInt();
		result.cloner = uniquesString.toInt();
	}

	return result;
}

GitHubManager::View GitHubManager::GetGitHubTrafficViewData(char* gitHubUserName, char* gitHubRepository, char* hostFingerPrint, char* gitHubToken)
{
	bool noDataFound = true;
	int responseRowCounter = 0;
	int httpStatus = -1;
	String jsonResponse = "";
	String countsString = "";
	String uniquesString = "";
	int fieldPosition = 0;
	int colonPosition = 0;
	int commaPosition = 0;
	GitHubManager::View result = { 0, 0 };

	WiFiClientSecure _wifiClient;
	_wifiClient.setFingerprint(hostFingerPrint);

	if(_wifiClient.connect(GITHUB_API_HOST, 443))
    {
		#ifdef GITHUB_DEBUG
		Serial.print(millis());
		Serial.println(" Connected to host!");
		#endif

		_wifiClient.print(String("GET /repos/") + gitHubUserName + "/" + gitHubRepository + "/traffic/views" + " HTTP/1.1\r\n" +
							"Host: " + GITHUB_API_HOST + "\r\n" +
							"Authorization: " + gitHubToken + "\r\n" +
							"User-Agent: esp8266\r\n"+
							"Cache-Control: no-cache\r\n\r\n");

		#ifdef GITHUB_DEBUG
		Serial.print(millis());
		Serial.println(" wait response from the host:");
		#endif

		// Loop on the response row information
		while ((_wifiClient.connected() || _wifiClient.available()) && noDataFound)
		{
			String tempRow = _wifiClient.readStringUntil('\n');

			#ifdef GITHUB_DEBUG
			Serial.print(String(responseRowCounter) + "[" + httpStatus + "] (" + millis() + ") > ");
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
				noDataFound = false;

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

	if(jsonResponse != "")
	{
		// Get count of Clones in the last two week
		fieldPosition = jsonResponse.indexOf("count");
		colonPosition = jsonResponse.indexOf(":", fieldPosition);
		commaPosition = jsonResponse.indexOf(",", colonPosition);
		countsString = jsonResponse.substring(colonPosition + 1, commaPosition);
		//clones[i][0] = countsString.toInt();
		result.total = countsString.toInt();
		// Get count of unique Clones in the last two week
		fieldPosition = jsonResponse.indexOf("uniques");
		colonPosition = jsonResponse.indexOf(":", fieldPosition);
		commaPosition = jsonResponse.indexOf(",", colonPosition);
		uniquesString = jsonResponse.substring(colonPosition + 1, commaPosition);
		//clones[i][1] = uniquesString.toInt();
		result.visitors = uniquesString.toInt();
	}

	return result;
}