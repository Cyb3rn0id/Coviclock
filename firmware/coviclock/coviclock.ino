/* 
 * Coviclock
 * (c)2020 Giovanni Bernardo (https://www.settorezero.com)
 *  
 * This software uses open data uploaded on Github by
 * "Presidenza del Consiglio dei Ministri - Dipartimento protezione civile"
 * at following url: https://github.com/pcm-dpc/COVID-19
 * Open data are distributed under the CC-BY-SA 4.0 License
 * 
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
 
#include <string.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h> // used by NTP Client
#include <WiFiClientSecure.h> // use this for https
//#include <WiFiClient.h> // use this for http
#include <EasyNTPClient.h> // https://github.com/aharshac/EasyNTPClient - distributed under MIT license
#include <TimeLib.h> // https://github.com/PaulStoffregen/Time - no license

#define BUZZER  D8
#define S1      D1
#define S2      D6

// WiFi SETTINGS
const char* ssid="[YOUR SSID]";
const char* password="[YOUR SSID PASSWORD]";
#define RETRIES_WIFI 100  // number of WiFi re-connection retries after a no-connection, at 500mS reconnection rate
#define USE_STATIC_IP // comment for using DHCP or if you have errors on NTP server connection

#ifdef USE_STATIC_IP
IPAddress deviceIP(192, 168, 1, 166); // ESP8266 static address
IPAddress gateway(192, 168, 1, 1); // router address
IPAddress subnet(255, 255, 255, 0); // subnet mask
IPAddress dns1 (8, 8, 8, 8); // first DNS, required for easyntp with static ip (https://github.com/aharshac/EasyNTPClient/issues/4)
IPAddress dns2 (8, 8, 4, 4); // second DNS, required for easyntp with static ip
#endif

// CSV SETTINGS
// URL of CSV used:
// https://raw.githubusercontent.com/pcm-dpc/COVID-19/master/dati-regioni/dpc-covid19-ita-regioni-latest.csv
const char* csvHost="raw.githubusercontent.com"; 
const char* csvPath="pcm-dpc/COVID-19/master/dati-regioni/dpc-covid19-ita-regioni-latest.csv";
const char* csvFind="Campania"; // change this for download data of your region, according the name as written in the CSV:
/*
Region Code, Region Name.
Cannot use region code since for region 'Trentino-Alto Adige' since is splitted in
2 autonomous regions that have same code: 04
13,Abruzzo
17,Basilicata
04,P.A. Bolzano
18,Calabria
15,Campania
08,Emilia-Romagna
06,Friuli Venezia Giulia
12,Lazio
07,Liguria
03,Lombardia
11,Marche
14,Molise
01,Piemonte
16,Puglia
20,Sardegna
19,Sicilia
09,Toscana
04,P.A. Trento
10,Umbria
02,Valle d'Aosta
05,Veneto
*/
// fingerprint is used only for a secure client over https. If you must download data using http, leave it blank
// see on Github repository how to obtain a fingerprint for different website
const char* csvHostFingerPrint="70 94 de dd e6 c4 69 48 3a 92 70 a1 48 56 78 2d 18 64 e0 b7"; // fingerprint for raw.github until 10May2022
#define USESECURECLIENT // comment if client is on http, uncomment if https
#define CSV_UPDATE_HOUR 18 // CSV is usually updated first than that time (6:00PM) - don't use AM/PM format!
#define CSV_RETRY_MINUTES 10 // if CSV is not updated at defined hour, I'll retry after those minutes
#define CSV_nFIELDS  19 // number of CSV Fields (columns)
String csvRow=""; // row containing the string found
String csvRowField[CSV_nFIELDS]; // separate fields of the row above
int csvDateCode=0; // day/month (DMM) of downloaded CSV update
bool startUpConnection=true; // true only for the first time we download the csv for indicating first connection to CSV host gone good, then false

// NTP / TIME SETTINGS
// WARNING: use DHCP (NO static IP) if you encounter problems with NTP server
// Italy follows the CET (Central European Time). NTP gives UTC time
// during the ST (Standard Time="ora solare" in italian language) CET=UTC+1
// during the DST (Daylight Saving Time="ora legale" in italian language) CET=UTC+2
// DST starts at 2:00AM of the last Sunday of March and ends at 3:00AM of the last Sunday of October
#define TIME_OFFSET 1 // local time offset during Standard Time
const char* NTPServer="it.pool.ntp.org"; //"ntp1.inrim.it"; 
bool clockIsSet=false; // flag that indicates that the time was updated the first time at startup
int8_t prevDay=-1; // used for time refresh once a day or for forcing time update
String wd[7]={"Domenica ","Lunedi   ","Martedi  ","Mercoledi","Giovedi  ","Venerdi  ","Sabato   "}; // name of weekdays, 9 chars max length in italian
String mo[12]={"GEN","FEB","MAR","APR","MAG","GIU","LUG","AGO","SET","OTT","NOV","DIC"}; // name of months
#define NTP_RETRY_MINUTES 10  // if first NTP connection gone bad, I'll retry after those minutes

// Display connections
#define TFT_CS    D2
#define TFT_RST   D3
#define TFT_DC    D0
// other 2 pins of display are connected to the SPI hardware port and cannot be
// specified in the library initialization:
// MOSI => D7
// SCK => D5
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Clients
WiFiUDP udp; // UDP client for NTP server
EasyNTPClient ntpClient(udp, NTPServer, (TIME_OFFSET*60*60));
#ifdef USESECURECLIENT
  WiFiClientSecure client; 
  #define HOSTPORT 443
#else
  WiFiClient client; 
  #define HOSTPORT 80
#endif

// routine for WiFi connecting
// if debug=true, a verbose output will be given on the display
void WiFi_connect(bool debug)
  {
  static uint16_t retr=0; //re-connection retries counter
  if (debug)
    {
    //tft.setCursor(0,0);
    tft.setTextColor(ILI9341_GREEN);  
    tft.setTextSize(1);
    tft.println("Connecting to WiFi");
    tft.print(">");
    tft.setTextColor(ILI9341_CYAN);  
    tft.println(ssid);
    }
  // workaround for issue #2186 (ESP8266 doesn't connect to WiFi with release 2.3.0)
  // https://github.com/esp8266/Arduino/issues/2186#issuecomment-228581052
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);

#ifdef USE_STATIC_IP
  WiFi.config(deviceIP, gateway, subnet, dns1, dns2);
#endif
  WiFi.begin(ssid, password);
  Serial.print(millis());
  Serial.print(" Trying to connect to WiFi. SSID: ");  
  Serial.println(ssid);
  Serial.print("[");
  while (WiFi.status()!=WL_CONNECTED) 
    {
    if (debug)
      {    
      tft.setTextColor(ILI9341_GREEN);  
      tft.print('.');
      retr++;
      }
    Serial.print(".");
    if (retr==RETRIES_WIFI)
      {
      // too many retries with same connection status: something is gone wrong
      if (debug)
        {
        tft.println();
        tft.setTextColor(ILI9341_RED);  
        tft.println("TOO MANY RETRIES!");
        tft.println("Maybe WiFi settings are wrong");
        tft.println("Or WiFi is not available");
        }
      Serial.println("]");
      Serial.print(millis());
      Serial.println(" Too many retries");
      Serial.println("Please check wifi settings");
      retr=0;
      }
    delay(500);
    } // \not connected
  if (debug)
    {
    tft.setTextColor(ILI9341_GREEN);  
    tft.println();
    tft.println("Connected!");
    tft.print("IP address: ");
    tft.println(WiFi.localIP());  
    }
  Serial.println("]");
  Serial.print(millis());
  Serial.print(" WiFi connected. ");
  Serial.print("Device IP address is: ");
  Serial.println(WiFi.localIP());
  }

// prints current date/time on the serial port
void debugDate(void)
  {
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
 
// connects to NTP server for time updating
// normally this function will connect to NTP server only if a day is passed (forced=false)
// if forced=true, time will be updated anyway
// if update does not work, I'll recheck after NTP_RETRY_MINUTES minutes
// this routine uses the checkDST function for adding an hour if we're in DST period
bool updateTime(bool forced)
  {
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
      if (checkDST) 
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
    } // prevDay!=today
  } // \updateTime

// check if actual time is Standard Time or Daylight Saving Time
// returns DST status (true if we're between the 2:00AM of the last Sunday of March
// and the 03:00AM of the last Sunday of October)
bool checkDST(void)
  {
  // In italy DST goes from last Sunday of March (we must set UTC+2 at 2:00AM)
  // until last sunday of October (we must set UTC+1 at 3:00AM)
  bool DST=false;
  
  // Month is Apr,May,Jun,Jul,Aug,Sep => for sure we're in DST
  if ((month()>3) && (month()<10))  
    {
    return (true);
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
      // it's not sunday, but sunday is passed
      else if (weekday()>1)
        {
        // If March, we're in DST, if October, DST has ended
        DST=(month()==3?true:false);
        }
      else
        // it's Sunday but are not the 2:00AM in March nor the 3:00AM in October
        {
        // If March, no DST, if October, we're still in DST
        DST=(month()==3?false:true);
        }
      }
    else
      // it's not sunday or sunday has to come
      // If March, no DST, if October, we're still in DST
      {
      DST=(month()==3?false:true);
      }
    } // this month is 3 or 10
  // in all other cases there is no DST (Month is Jan,Feb,Nov,Dec)
  return (DST);
  }
  
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
  
// connect to CSV server for downloading data
// if debug==true, a verbose output will be given on the display
int downloadCSV(bool debug)
  {
  int response=0;  
  // response:
  // 1: connection ok, string found
  // 0: connection ok but string not found (given also if you connect on https using port 403 without giving a fingerprint)
  // -1: error (404 - file not found or 301 - moved permanently)
  // -2: connection error
  
  if (debug)
    {
    tft.setTextColor(ILI9341_GREEN);  
    tft.setTextSize(1);
    tft.println("Connecting to host");
    tft.print(">");
    tft.setTextColor(ILI9341_CYAN);  
    tft.println(csvHost);
    }
  Serial.print(millis());
  Serial.print(" Connecting to CSV host (");
  Serial.print(csvHost);
  Serial.println(")");

// if you've set the secure client (https) it's mandatory send a fingerprint
// or the host will connect but will not give any response
#ifdef USESECURECLIENT
 client.setFingerprint(csvHostFingerPrint);
#endif

  if (client.connect(csvHost, HOSTPORT))
    {
    if (debug)
      {
      tft.setTextColor(ILI9341_GREEN);  
      tft.println("Connected!");
      tft.println("Searching row containing the string");
      tft.print(">");
      tft.setTextColor(ILI9341_CYAN);  
      tft.println(csvFind);
      }
    
    Serial.print(millis());
    Serial.println(" Connected to CSV host!");
    // send the GET request to the host
    client.print
      (
      String("GET /") + csvPath + " HTTP/1.1\r\n" +
      "Host: " + csvHost+ "\r\n" +
      "User-Agent: esp8266\r\n"+
      // "Accept: text/xml,application/xml,application/xhtml+xml,text/html*/*\r\n"+
      // "Accept-Language: en-us\r\n"+
      // "Accept-Charset: ISO-8859-1,utf-8\r\n"+
      "Connection: close\r\n"+
      "\r\n"
      );
    tft.setTextColor(ILI9341_GREEN);  
    Serial.print(millis());
    Serial.println( " Response from the host:");
    while (client.connected() || client.available())
      {
      startUpConnection=false; // first connection ok
      String tempRow = client.readStringUntil('\n');
      Serial.print("   > ");
      Serial.println(tempRow);

      // 404 or 301 header
      if ((tempRow.indexOf("HTTP/1.1 404")>0) || (tempRow.indexOf("HTTP/1.1 301")>0))
        {
        Serial.print(millis());
        Serial.println(" Host gave a 404 or 301 error. Connection closed");
        client.stop();
        response=-1;
        break;
        }
        
      if ((tempRow.indexOf(csvFind)>0) && (response==0)) 
        {
        csvRow=tempRow;
        client.stop();
        response=1;
        Serial.print(millis());
        Serial.println(" String found. Fetching stopped. Connection closed");
        // will print the found string below
        if (debug) tft.println("Connection closed");
        
        // parse the string
        int cc=0; // comma counter
        int k=-1; // first substring must start from index 0 and I add 1 for others
        // scanning in search of commas one char at time
        for (int i=0; i<csvRow.length(); i++) 
           {
            if (csvRow.substring(i, i+1)==",") // found a comma
              {
              csvRowField[cc]=csvRow.substring(k+1,i); // extract field from next char after previous comma until this char
              k=i; // keep in mind the position of last comma found
              cc++; // commas counter=field counter
              // added this row since on March,25th they added 2 more fields and this caused code malfunction since I used an array of 16 elements
              // and now were 18. So if in the future they add other fields, this instruction will skip the additional fields.
              if (cc==(CSV_nFIELDS-1)) break;
              } // \comma found
            } // \for
        // last element has no comma after it, so I must extract it from the last comma to the end of the string
        csvRowField[cc]=csvRow.substring(k+1,csvRow.length());
        break;
        } // string found
      } // while client connected or available

   switch(response)
      {
      case 0: // string not found
        {
        client.stop();          
        if (debug)
          {
          tft.setTextColor(ILI9341_GREEN);  
          tft.println("Connection closed");
          tft.setTextColor(ILI9341_RED);  
          tft.println("string not found");
          }
        Serial.print(millis());
        Serial.println(" String not found. Connection closed");
        }
      break;
        
      case 1: // found
        {
        if (debug)
          {
          tft.setTextColor(ILI9341_GREEN);
          tft.println("fetched row:");
          tft.println();
          tft.setTextColor(ILI9341_YELLOW);
          tft.println(csvRow);
          delay(2000);
          }
        Serial.print(millis());
        Serial.print(" Fetched row: ");
        Serial.println(csvRow);
     
        // CSV day of update
        String ud=csvRowField[0].substring(csvRowField[0].lastIndexOf('-')+1,csvRowField[0].indexOf(' '));
        // CSV month of update
        String um=csvRowField[0].substring(csvRowField[0].indexOf('-')+1,csvRowField[0].lastIndexOf('-'));
        csvDateCode=(ud.toInt()*100)+um.toInt(); // csv update time in format DMM (datecode)
        Serial.print(millis());
        Serial.print(" CSV datecode is: ");
        Serial.println(csvDateCode);
        }
      break;
        
      case -1: // error
        {
        if (debug) 
          {
          tft.setTextColor(ILI9341_RED);
          tft.println("ERROR 404 or 301");  
          }
        }
      break;
      } // \switch
    } // \client connected
  else
    {
    // connection failure
    if (debug)
      {
      tft.setTextColor(ILI9341_RED);  
      tft.println("Cannot connect");
      }
    Serial.print(millis());
    Serial.println(" Cannot connect to CSV host. Sorry");
    client.stop();
    response=-2; // connection error
    }
    
  return(response);
  } // \downloadCSV

// prints the clock on the display
void printClock(void)
  {
  // write time
  tft.setCursor(0, 216); // x,y
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);  
  tft.setTextSize(4);
  // H,m and s are in number format, so I must add a zero in front of
  if (hour()<10) tft.print("0");
  tft.print(hour());
  tft.print(":");
  if (minute()<10) tft.print("0");
  tft.print(minute());
  tft.print(":");
  if (second()<10) tft.print("0");
  tft.print(second());
  tft.print(" "); // sometimes probably a bug prints a further 0
  
  tft.println();
  tft.setTextSize(3);
  tft.print(wd[weekday()-1]);
  tft.println();
  if (day()<10) tft.print("0");
  tft.print(day());
  tft.print(" ");
  tft.print(mo[month()-1]);
  //tft.print(" ");
  //tft.print(year()); // year()-2000 for showing only the last 2 numbers
  tft.println();
  tft.setTextSize(1);
  tft.println();
  tft.setTextColor(ILI9341_GREEN);
  tft.println("(c)2020 Giovanni Bernardo");
  tft.print("https://www.settorezero.com");
  }

// refresh display with new data obtained from CSV
void updateDisplayData(void)
  {
   /* csvRowField[] array:
   * 
   * 0 data (ex.: 2020-03-25T17:00:00)
   * 1 stato (IT)
   * 2 codice_regione
   * 3 denominazione_regione
   * 4 lat
   * 5 long
   * 6 ricoverati_con_sintomi
   * 7 terapia_intensiva
   * 8 totale_ospedalizzati
   * 9 isolamento_domiciliare
   * 10 totale_positivi 
   * 11 variazione_totale_positivi (variato dal 30/03/2020 => era nuovi_attualmente_positivi)
   * 12 nuovi_positivi (aggiunto dal 30/03/2020, era il campo no.11)
   * 13 dimessi_guariti
   * 14 deceduti
   * 15 totale_casi
   * 16 tamponi
   * 17 note_it (aggiunto dal 25/03/2020)
   * 18 note_en (aggiunto dal 25/03/2020)
   */
  
  Serial.print(millis());
  Serial.println(" Updating display with new data");
  // write CSV results on the screen
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0); // x,y, I start from 7 since my display has first rows broken... :(
  tft.setTextColor(ILI9341_WHITE);  
  tft.setTextSize(3);
  tft.println("CoviClock");
  tft.println(csvFind); // print region

  tft.setTextSize(2);
  tft.println();
  tft.setTextColor(ILI9341_YELLOW);  
  tft.print("Nuovi contagi: ");
  tft.setTextColor(ILI9341_ORANGE);  
  tft.println(csvRowField[12]);
    
  tft.setTextColor(ILI9341_YELLOW);  
  tft.print("Tot. contagi: ");
  tft.setTextColor(ILI9341_PINK);  
  tft.println(csvRowField[10]);
  
  tft.setTextColor(ILI9341_YELLOW);  
  tft.print("Morti: ");
  tft.setTextColor(ILI9341_RED);  
  tft.println(csvRowField[14]);

  tft.setTextColor(ILI9341_YELLOW);  
  tft.print("In terapia: ");
  tft.setTextColor(ILI9341_MAGENTA);  
  tft.println(csvRowField[7]);

  tft.setTextColor(ILI9341_YELLOW);  
  tft.print("Isolati: ");
  tft.setTextColor(ILI9341_CYAN);  
  tft.println(csvRowField[9]);

  tft.setTextColor(ILI9341_YELLOW);  
  tft.print("Guariti: ");
  tft.setTextColor(ILI9341_GREEN);  
  tft.println(csvRowField[13]);
    
  tft.setTextSize(1);
  tft.println();
  tft.setTextColor(ILI9341_BLUE);  
  tft.print("Ultimo aggiornamento: ");
  tft.setTextColor(ILI9341_LIGHTGREY);
  // with an update from 25/03/2020 date field is YYYY-MM-DDTHH:mm:ss (they put 'T' instead of ' ' since is the ISO8601 standard)
  tft.println(csvRowField[0].substring(csvRowField[0].lastIndexOf('-')+1,csvRowField[0].indexOf('T'))+"/"+csvRowField[0].substring(csvRowField[0].indexOf('-')+1,csvRowField[0].lastIndexOf('-'))+"/"+csvRowField[0].substring(0,csvRowField[0].indexOf('-')));
  //tft.println(csvDateCode);

  Serial.print(millis());
  Serial.println(" Display update finished");

  digitalWrite(BUZZER,HIGH);
  delay(100);
  digitalWrite(BUZZER,LOW);
  delay(200);
  digitalWrite(BUZZER,HIGH);
  delay(100);
  digitalWrite(BUZZER,LOW);
  delay(200);
  digitalWrite(BUZZER,HIGH);
  delay(100);
  digitalWrite(BUZZER,LOW);
  }

// prints temperature value passed as float
// temperature will be printed with 1 decimal
// and with the degree symbol
void printTemperature(float tval)
  {
  tft.setCursor(156,272);
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_PINK,ILI9341_BLACK);
  tft.print(tval,1);
  tft.setTextSize(2);
  tft.setCursor(228,268);
  tft.print("o"); // looks like a grade symbol?! LOL
  }

// Arduino Setup function
void setup() 
  {
  pinMode(BUZZER,OUTPUT);
  digitalWrite(BUZZER,LOW); // buzzer off
  Serial.begin(115200);
  tft.begin();
  pinMode(S1,INPUT_PULLUP);
  pinMode(S2,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(S1), Switch1_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(S2), Switch2_ISR, FALLING);
  
  tft.setRotation(0);  // 0 to 3, 0 for vertical with SD pins on top
  tft.fillScreen(ILI9341_BLACK); // background color

  tft.setCursor(0,0);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setTextSize(1);
  tft.println("Coviclock");
  tft.println("by Giovanni Bernardo");
  tft.println();
 
  Serial.println();
  Serial.print(millis());
  Serial.println(" Coviclock by Giovanni Bernardo");
  WiFi_connect(true);
  clockIsSet=updateTime(false);
  } // \setup

// Arduino Loop function (main)
void loop(void) 
  {
  static int csvDownloaded=0; // status of CSV download
  static int lastCsvDateCode=-1; // datecode of CSV actually displayed (DMM)
  static int todayDateCode=0; // datecode of today (DMM)
  static bool firstCheck=true; // true when it's hour to download the new csv and is the first time we try
  static bool reCheck=false; // true when it's passed the hour for csv downloading, first try gone bad so we must retry until csv is good
  static long lastCheckTime=0; // last time we tried to download the new csv but csv was not updated
  static bool firstStart=true; // for updating data if you upload code after 00:00 and CSV has previous day date, and for debug on display only first time
  static bool fatalError=false; // if encountered an error that cannot make the clock work
  static uint8_t tempCount=0; // counter for analog reading of temperature
  static float temp=0; // temperature value
  static bool lastDst=checkDST(); // DST status, used for forcing time update since normally will occur after 00:00

  // check if clock update if needed every hour
  if (clockIsSet && !fatalError) 
    {
    bool dst=checkDST(); // check if we're in DST for forcing time update
    if (lastDst!=dst)
      {
      if(updateTime(true)) lastDst=dst;
      }
   else
      {
      updateTime(false); // normal update once a day after 00:00
      }
    } // \clockIsSet
 else // first clock set gone bad => fatal error, cannot start!
    {
    fatalError=true;
    static bool printed=false;
    if (!printed && !clockIsSet)
      {
      tft.setTextColor(ILI9341_RED);
      tft.println("FATAL ERROR : TIME NOT SET");
      Serial.print(millis());
      Serial.println(" Time not set. Please restart"); 
      }
    printed=true;
    }
    
  todayDateCode=(day()*100)+month(); // today in DMM format
   
  // if time was set at start-up and today is a different day than last csv update
  // (or we're at first run since -1!=0) => check for a CSV update
  if ((clockIsSet) && (lastCsvDateCode!=todayDateCode) && (!fatalError))
    {
    // check if is time to update
    if (!firstCheck && !reCheck) // not verified at startup since firstCheck=true and reCheck=false
       {
       if (hour()>=CSV_UPDATE_HOUR) 
          {
          firstCheck=true;
          Serial.print(millis());
          Serial.println(" It's time to update the CSV");
          }
       }
    // following line bypass thet millis() rollover
    if ((reCheck) && (millis()<lastCheckTime)) lastCheckTime=millis();
    // we must download the CSV for the first time
    // or is a recheck, then CSV_RETRY_MINUTES minutes must be passed from last search
    long retrymillis=lastCheckTime+(CSV_RETRY_MINUTES*60*1000);
    if (firstCheck || (reCheck && (millis()>retrymillis)))
      {
      if (firstCheck)
        {
        Serial.print(millis());
        Serial.println(" This is the first check of the CSV file in this day");
        }
      else
        {
        Serial.print(millis());
        Serial.println(" Re-checking the CSV file since the first check gone bad");
        }
      csvDownloaded=downloadCSV(firstStart); // debug on display only first time
      }    
    }
  else
    {
    csvDownloaded=0;
    }

  // check the CSV download status
  switch (csvDownloaded)
    {
      case 0: // string not found
        {
        if (firstStart) // only at startup is a fatal error: maybe CSV data are specified wrong
          {
          tft.setTextColor(ILI9341_RED);
          tft.setTextSize(1);
          tft.println("FATAL ERROR: STRING NOT FOUND");
          fatalError=true;
          firstStart=false;
          }
        }
      break; // \case 0
      
      case 1: // it's all ok
        {
        firstCheck=false;
        Serial.print(millis());
        Serial.print(" Today datecode is: ");
        Serial.println(todayDateCode);
        // check if csv update is today or we're at startup
        // csvDateCode is set by downloadCSV function
        if ((csvDateCode==todayDateCode) || (firstStart))
            {
            Serial.print(millis());
            if (firstStart)
                {Serial.println(" This is the first time program starts, CSV datecode doesn't care");}
            else
                {Serial.println(" CSV datecode and Today datecode match => CSV is updated!");}
            // file is updated. Do not perform further checks for today by updating lastTimeCSVUpdated
            lastCsvDateCode=csvDateCode;
            // print new data on the display
            updateDisplayData();
            reCheck=false;
            firstStart=false;
            }
        else // datecode of csv is old or is not the start-up update=> set a re-check
            {
            reCheck=true;
            lastCheckTime=millis();
            Serial.print(lastCheckTime); // now millis()
            Serial.println(" CSV still not updated. I'll try later");
            }
        // prevent further printing on display for today, eventually
        csvDownloaded=0;
        } // \case 1
      break; // \case 1
     
      case -1: // file not found
        {
        tft.setTextColor(ILI9341_RED);
        tft.setTextSize(1);
        tft.println("FATAL ERROR: FILE NOT FOUND");
        fatalError=true;
        firstStart=false;
        csvDownloaded=0; // exit from this case, preventing continuous printing on display
        } 
      break; // \case -1
      
      case -2: // connection error
        {
        // if we receive an error on CSV downloading at start-up
        // maybe host is written wrong or there are some other problems
        if (startUpConnection) // flag used only for first connection
          {
          tft.setTextColor(ILI9341_RED);
          tft.setTextSize(1);
          tft.println("FATAL ERROR : NO HOST CONNECTION");
          fatalError=true;
          }
        else // if is not the start-up, maybe better retry later
          {
          reCheck=true;
          lastCheckTime=millis();
          }
        }
      break; // \case -2
      } // \switch

 // if there are no errors, I can print clock and temperature
  if (!fatalError)
    {
    printClock(); // print the clock/calendar
    
    // read the temperature, one time for every loop iteraction
    temp+=analogRead(A0);
    tempCount++;
    if (tempCount==32)
      {
      //temp=temp/32; // average on 32 values
      //temp=(temp/1024)*1000; // mV on A0
      //temp=temp/0.3125; // Output from sensor, applied on A0 = mV first than NodeMCU voltage divider (R1=220K, R2=100K)
      temp=temp*0.09765625; // all three operations above
      temp=temp-500.0; // 500mV=0°C
      temp=temp*0.1; // 10mV=1°C (/10)
      // prints temperature value
      printTemperature(temp);
      temp=0;
      tempCount=0;
      }
    }
} // \loop
