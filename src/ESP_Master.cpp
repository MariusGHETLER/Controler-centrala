/*
SENDER
*/
//#include <esp_wifi.h>
#include <esp_now.h>          // needed to receive transmit between ESP
#include <WiFi.h>             // needed for conenctin to WI-FI
#include "time.h"             // needed for get time
#include <WebServer.h>        // needed to create a simple webserver
#include <WebSocketsServer.h> // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>      // needed for JSON encapsulation (send multiple variables with one string)
#include <AT24Cxx.h>

#define i2c_address 0x50
#define I2C_SDA 21
#define I2C_SCL 22
#define Releu 13

enum wday
{
  Luni = 1,
  Marti = 2,
  Miercuri = 3,
  Joi = 4,
  Vineri = 5,
  Sambata = 6,
  Duminica = 7
} wd;

union unionFloat
{
  float f;
  byte b[4];
};
union unionInt
{
  int i;
  byte b[4];
};

enum etime
{
  hVal_2_1,
  mVal_2_1,
  hVal_3_1,
  mVal_3_1,
  hVal_2_2,
  mVal_2_2,
  hVal_3_2,
  mVal_3_2,
  hVal_2_3,
  mVal_2_3,
  hVal_3_3,
  mVal_3_3,
  hVal_2_4,
  mVal_2_4,
  hVal_3_4,
  mVal_3_4,
  hVal_2_5,
  mVal_2_5,
  hVal_3_5,
  mVal_3_5,
  hVal_2_6,
  mVal_2_6,
  hVal_3_6,
  mVal_3_6,
  hVal_2_7,
  mVal_2_7,
  hVal_3_7,
  mVal_3_7
};

enum etemp
{
  OffsetTemp,
  tVal_1_1,
  tVal_2_1,
  tVal_3_1,
  tVal_1_2,
  tVal_2_2,
  tVal_3_2,
  tVal_1_3,
  tVal_2_3,
  tVal_3_3,
  tVal_1_4,
  tVal_2_4,
  tVal_3_4,
  tVal_1_5,
  tVal_2_5,
  tVal_3_5,
  tVal_1_6,
  tVal_2_6,
  tVal_3_6,
  tVal_1_7,
  tVal_2_7,
  tVal_3_7
};

typedef enum etime etimev;
typedef enum etemp etempv;

int timeval[28];
float tempval[22];

const int timeoffs = 4;
const int tempoffs = 3;

AT24Cxx eep(i2c_address, 100);

// ssid = wifi name / password
const char *ssid = "DIGI-2.5GHz";
const char *password = "rockycheese250";

///////////////////////////////////////Global WEB Management//////////////////////////////////////////

void EEPROM_UpdateInt(uint16_t address, int value)
{
  unionInt my_data;
  my_data.i = value;
  // Serial.println("Written data b:");
  for (int i = 0; i < 4; i++)
  {
    eep.update(address, my_data.b[i]);
    address++;
    // Serial.print(my_data.b[i]);
    // Serial.print(" ");
  }
  // Serial.println();
  // Serial.println("Written data f:");
  // Serial.println(my_data.f);
}

int EEPROM_ReadInt(uint16_t address){
  unionInt my_data;
  // Serial.println("Read data b: ");
  for (int i = 0; i < 4; i++)
  {
    my_data.b[i] = eep.read(address);
    address++;
    // Serial.print(my_data.b[i]);
    // Serial.print(" ");
  }
  // Serial.println();
  // Serial.println("Read data f: ");
  // Serial.println(my_data.f);
  return (my_data.i);
}

void EEPROM_UpdateFloat(uint16_t address, float value)
{
  unionFloat my_data;
  my_data.f = value;
  // Serial.println("Written data b:");
  for (int i = 0; i < 4; i++)
  {
    eep.update(address, my_data.b[i]);
    address++;
    // Serial.print(my_data.b[i]);
    // Serial.print(" ");
  }
  // Serial.println();
  // Serial.println("Written data f:");
  // Serial.println(my_data.f);
}

float EEPROM_ReadFloat(uint16_t address)
{
  unionFloat my_data;
  // Serial.println("Read data b: ");
  for (int i = 0; i < 4; i++)
  {
    my_data.b[i] = eep.read(address);
    address++;
    // Serial.print(my_data.b[i]);
    // Serial.print(" ");
  }
  // Serial.println();
  // Serial.println("Read data f: ");
  // Serial.println(my_data.f);
  return (my_data.f);
}

// The JSON library uses static memory, so this will need to be allocated:
StaticJsonDocument<200> doc_tx; // provision memory for about 200 characters
StaticJsonDocument<2000> doc_rx;
StaticJsonDocument<2000> doc2_tx;
// We want to periodically send values to the clients, so we need to define an "interval" and remember the last time we sent data to the client (with "previousMillis")
int interval = 1000;              // send data to the client every 1000ms -> 1s
unsigned long previousMillis = 0; // we use the "millis()" command for time reference and this will output an unsigned long

// Initialization of webserver and websocket
WebServer server(80);                              // the server uses port 80 (standard port for websites
WebSocketsServer webSocket = WebSocketsServer(81); // the websocket uses port 81 (standard port for websockets
int Centrala = 0;
int StatusCentrala = 0;
//float OffsetTemp = 1.5;
float Temp, Hum;
int Program;
char datetime[40];
void WebSoketUpdate()
{
  server.handleClient();        // Needed for the webserver to handle all clients
  webSocket.loop();             // Update function for the webSockets
  unsigned long now = millis(); // read out the current "time" ("millis()" gives the time in ms since the Arduino started)
  if ((unsigned long)(now - previousMillis) > interval)
  {                                              // check if "interval" ms has passed since last time the clients were updated
    String jsonString = "";                      // create a JSON string for sending data to the client
    JsonObject object = doc_tx.to<JsonObject>(); // create a JSON Object
    object["msg_type"] = 2;
    object["temp"] = Temp;                      // write data into the JSON object -> I used "rand1" and "rand2" here, but you can use anything else
    object["hum"] = Hum;
    object["StatusCentrala"] = StatusCentrala;
    object["datetime"] = datetime;
    object["Program"] = Program;
    serializeJson(doc_tx, jsonString);  // convert JSON object to string
    //Serial.println(jsonString);         // print JSON string to console for debug purposes (you can comment this out)
    webSocket.broadcastTXT(jsonString); // send JSON string to clients
    previousMillis = now;               // reset previousMillis
  }
}

void WS_ProgramareUpdate()
{
  server.handleClient();        // Needed for the webserver to handle all clients
  webSocket.loop();             // Update function for the webSockets
  
    String jsonString = "";                      // create a JSON string for sending data to the client
    JsonObject object = doc2_tx.to<JsonObject>(); // create a JSON Object
    object["msg_type"] = 1;
    
    object["OffsetTemp"] = tempval[OffsetTemp];
    object["hVal_2_1"] = timeval[hVal_2_1];
    object["mVal_2_1"] = timeval[mVal_2_1];
    object["hVal_3_1"] = timeval[hVal_3_1];
    object["mVal_3_1"] = timeval[mVal_3_1];

    object["hVal_2_2"] = timeval[hVal_2_2];
    object["mVal_2_2"] = timeval[mVal_2_2];
    object["hVal_3_2"] = timeval[hVal_3_2];
    object["mVal_3_2"] = timeval[mVal_3_2];

    object["hVal_2_3"] = timeval[hVal_2_3];
    object["mVal_2_3"] = timeval[mVal_2_3];
    object["hVal_3_3"] = timeval[hVal_3_3];
    object["mVal_3_3"] = timeval[mVal_3_3];

    object["hVal_2_4"] = timeval[hVal_2_4];
    object["mVal_2_4"] = timeval[mVal_2_4];
    object["hVal_3_4"] = timeval[hVal_3_4];
    object["mVal_3_4"] = timeval[mVal_3_4];

    object["hVal_2_5"] = timeval[hVal_2_5];
    object["mVal_2_5"] = timeval[mVal_2_5];
    object["hVal_3_5"] = timeval[hVal_3_5];
    object["mVal_3_5"] = timeval[mVal_3_5];

    object["hVal_2_6"] = timeval[hVal_2_6];
    object["mVal_2_6"] = timeval[mVal_2_6];
    object["hVal_3_6"] = timeval[hVal_3_6];
    object["mVal_3_6"] = timeval[mVal_3_6];

    object["hVal_2_7"] = timeval[hVal_2_7];
    object["mVal_2_7"] = timeval[mVal_2_7];
    object["hVal_3_7"] = timeval[hVal_3_7];
    object["mVal_3_7"] = timeval[mVal_3_7];

    object["tVal_1_1"] = tempval[tVal_1_1];
    object["tVal_2_1"] = tempval[tVal_2_1];
    object["tVal_3_1"] = tempval[tVal_2_1];

    object["tVal_1_2"] = tempval[tVal_1_2];
    object["tVal_2_2"] = tempval[tVal_2_2];
    object["tVal_3_2"] = tempval[tVal_2_2];

    object["tVal_1_3"] = tempval[tVal_1_3];
    object["tVal_2_3"] = tempval[tVal_2_3];
    object["tVal_3_3"] = tempval[tVal_2_3];

    object["tVal_1_4"] = tempval[tVal_1_4];
    object["tVal_2_4"] = tempval[tVal_2_4];
    object["tVal_3_4"] = tempval[tVal_2_4];

    object["tVal_1_5"] = tempval[tVal_1_5];
    object["tVal_2_5"] = tempval[tVal_2_5];
    object["tVal_3_5"] = tempval[tVal_2_5];

    object["tVal_1_6"] = tempval[tVal_1_6];
    object["tVal_2_6"] = tempval[tVal_2_6];
    object["tVal_3_6"] = tempval[tVal_2_6];

    object["tVal_1_7"] = tempval[tVal_1_7];
    object["tVal_2_7"] = tempval[tVal_2_7];
    object["tVal_3_7"] = tempval[tVal_2_7];

    serializeJson(doc2_tx, jsonString);  // convert JSON object to string
    Serial.println(jsonString);         // print JSON string to console for debug purposes (you can comment this out)
    webSocket.broadcastTXT(jsonString); // send JSON string to clients
  
}

void DataStorageUpdate(uint16_t int_address, uint16_t f_address)
{
  
  for (int i = 0; i < 28; i++)
  {
    EEPROM_UpdateInt(int_address, timeval[i]);
    int_address += 4;
  }

  Serial.println("Written data int:");
  // Serial.println(final);

  for (int i = 0; i < 22; i++)
  {
    EEPROM_UpdateFloat(f_address, tempval[i]);
    f_address += 4;
  }
  Serial.println("Written data Float:");
}

void DataStorageRead(uint16_t int_address, uint16_t f_address)
{
  
  for (int i = 0; i < 28; i++)
  {
    timeval[i] = EEPROM_ReadInt(int_address);
    int_address += 4;
  }
  for (int i = 0; i < 22; i++)
  {
    tempval[i] = EEPROM_ReadFloat(f_address);
    f_address += 4;
  }


  Serial.println("Ora2 Vineri: " + String(timeval[hVal_2_5]));
  Serial.println("Minut2 Vineri: " + String(timeval[mVal_2_5]));
  Serial.println("Ora3 Vineri: " + String(timeval[hVal_3_5]));
  Serial.println("Minut3 Vineri: " + String(timeval[mVal_3_5]));

  Serial.println("Temp1 Vineri: " + String(tempval[tVal_1_5]));
  Serial.println("Temp2 Vineri: " + String(tempval[tVal_2_5]));
  Serial.println("Temp3 Vineri: " + String(tempval[tVal_3_5]));
}

void webSocketEvent(byte num, WStype_t type, uint8_t *payload, size_t length)
{ // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type)
  {                         // switch on the type of information sent
  case WStype_DISCONNECTED: // if a client is disconnected, then type == WStype_DISCONNECTED
    Serial.println("Client " + String(num) + " disconnected" + String(type));
    break;
  case WStype_CONNECTED: // if a client is connected, then type == WStype_CONNECTED
    Serial.println("Client " + String(num) + " connected " + String(type));
    WebSoketUpdate();
    // optionally you can add code here what to do when connected
    break;
  case WStype_FRAGMENT_TEXT_START:
    Serial.println("WStype_FRAGMENT_TEXT_START");
    break;
  case WStype_BIN:
    Serial.println("WStype_BIN");
    break;
  case WStype_ERROR:
    Serial.println("WStype_ERROR");
    break;
  case WStype_FRAGMENT_BIN_START:
    Serial.println("WStype_FRAGMENT_BIN_START");
    break;
  case WStype_FRAGMENT:
    Serial.println("WStype_FRAGMENT");
    break;
  case WStype_FRAGMENT_FIN:
    Serial.println("WStype_FRAGMENT_FIN");
    break;
  case WStype_PING:
    Serial.println("WStype_PING");
    break;
  case WStype_PONG:
    Serial.println("WStype_PONG");
    break;
  case WStype_TEXT: // if a client has sent data, then type == WStype_TEXT
    // try to decipher the JSON string received
    DeserializationError error = deserializeJson(doc_rx, payload);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    else
    {
      // JSON string was received correctly, so information can be retrieved:
      
      int res = doc_rx["msg_type"];
      if(res == 1)
      {

        tempval[OffsetTemp] = doc_rx["OffsetTemp"];
        tempval[tVal_1_1] = doc_rx["tVal_1_1"];
        tempval[tVal_2_1] = doc_rx["tVal_2_1"];
        tempval[tVal_3_1] = doc_rx["tVal_3_1"];
        timeval[hVal_2_1] = doc_rx["hVal_2_1"];
        timeval[mVal_2_1] = doc_rx["mVal_2_1"];
        timeval[hVal_3_1] = doc_rx["hVal_3_1"];
        timeval[mVal_3_1] = doc_rx["mVal_3_1"];

        tempval[tVal_1_2] = doc_rx["tVal_1_2"];
        tempval[tVal_2_2] = doc_rx["tVal_2_2"];
        tempval[tVal_3_2] = doc_rx["tVal_3_2"];
        timeval[hVal_2_2] = doc_rx["hVal_2_2"];
        timeval[mVal_2_2] = doc_rx["mVal_2_2"];
        timeval[hVal_3_2] = doc_rx["hVal_3_2"];
        timeval[mVal_3_2] = doc_rx["mVal_3_2"];

        tempval[tVal_1_3] = doc_rx["tVal_1_3"];
        tempval[tVal_2_3] = doc_rx["tVal_2_3"];
        tempval[tVal_3_3] = doc_rx["tVal_3_3"];
        timeval[hVal_2_3] = doc_rx["hVal_2_3"];
        timeval[mVal_2_3] = doc_rx["mVal_2_3"];
        timeval[hVal_3_3] = doc_rx["hVal_3_3"];
        timeval[mVal_3_3] = doc_rx["mVal_3_3"];

        tempval[tVal_1_4] = doc_rx["tVal_1_4"];
        tempval[tVal_2_4] = doc_rx["tVal_2_4"];
        tempval[tVal_3_4] = doc_rx["tVal_3_4"];
        timeval[hVal_2_4] = doc_rx["hVal_2_4"];
        timeval[mVal_2_4] = doc_rx["mVal_2_4"];
        timeval[hVal_3_4] = doc_rx["hVal_3_4"];
        timeval[mVal_3_4] = doc_rx["mVal_3_4"];

        tempval[tVal_1_5] = doc_rx["tVal_1_5"];
        tempval[tVal_2_5] = doc_rx["tVal_2_5"];
        tempval[tVal_3_5] = doc_rx["tVal_3_5"];
        timeval[hVal_2_5] = doc_rx["hVal_2_5"];
        timeval[mVal_2_5] = doc_rx["mVal_2_5"];
        timeval[hVal_3_5] = doc_rx["hVal_3_5"];
        timeval[mVal_3_5] = doc_rx["mVal_3_5"];

        tempval[tVal_1_6] = doc_rx["tVal_1_6"];
        tempval[tVal_2_6] = doc_rx["tVal_2_6"];
        tempval[tVal_3_6] = doc_rx["tVal_3_6"];
        timeval[hVal_2_6] = doc_rx["hVal_2_6"];
        timeval[mVal_2_6] = doc_rx["mVal_2_6"];
        timeval[hVal_3_6] = doc_rx["hVal_3_6"];
        timeval[mVal_3_6] = doc_rx["mVal_3_6"];

        tempval[tVal_1_7] = doc_rx["tVal_1_7"];
        tempval[tVal_2_7] = doc_rx["tVal_2_7"];
        tempval[tVal_3_7] = doc_rx["tVal_3_7"];
        timeval[hVal_2_7] = doc_rx["hVal_2_7"];
        timeval[mVal_2_7] = doc_rx["mVal_2_7"];
        timeval[hVal_3_7] = doc_rx["hVal_3_7"];
        timeval[mVal_3_7] = doc_rx["mVal_3_7"];

        Serial.println("Received Data from user: " + String(num));
        Serial.println("OffsetTemp: " + String(tempval[OffsetTemp]));
        Serial.println("tVal_1_1: " + String(tempval[tVal_1_1]));
        Serial.println("hVal_2_1: " + String(timeval[hVal_2_1]));
        Serial.println("mVal_2_1: " + String(timeval[mVal_2_1]));
        Serial.println("tVal_2_1: " + String(tempval[tVal_2_1]));
        Serial.println("hVal_3_1: " + String(timeval[hVal_3_1]));
        Serial.println("mVal_3_1: " + String(timeval[mVal_3_1]));
        Serial.println("tVal_3_1: " + String(tempval[tVal_3_1]));
        DataStorageUpdate(0, 130);
      }
      else
      {
        Serial.println("Citire programare...");
        WS_ProgramareUpdate();
        Serial.println("Citire programare completa");
      }

    }
    
    
    break;
  }
}

//////////////////////////////Global Time Management////////////////////////////////////////

// internet access for NTP
const char *ntpServer = "pool.ntp.org";
// timezone GTM set
const long gmtOffset_sec = 7200;
// offset for daylaight saving
const int daylightOffset_sec = 3600;
// Constant for time

char fullTimeHour[12];
char timeWeekDay[10];
char n_week_day[2] = " ";
char timeHour[3] = " ";
char timeMin[3] = " ";
// Format the time received
void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  strftime(datetime, 40, "%A, %B %d %Y %H:%M:%S", &timeinfo);
  // Serial.println("Time variables");
  strftime(fullTimeHour, 12, "%H:%M", &timeinfo);
  Serial.println(fullTimeHour);
  // strftime(timeWeekDay,10, "%A", &timeinfo);
  // Serial.println(timeWeekDay);
  strftime(n_week_day, 2, "%u", &timeinfo);
  // Serial.println(n_week_day);
  strftime(timeHour, 3, "%H", &timeinfo);
  // Serial.println(timeHour);
  strftime(timeMin, 4, "%M", &timeinfo);
  // Serial.println(timeMin);
  // Serial.println();
}

//////////////////////////////////Global ESP-NOW Settings/////////////////////////////////////////////

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {
    0x7C,
    0x9E,
    0xBD,
    0xE3,
    0xB2,
    0xCC};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message
{
  int id;
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  // Copies the sender mac address to a string

  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  // Serial.print("Bytes received: ");
  // Serial.println(len);
  // Serial.print("Sendre ID: ");
  // Serial.println(myData.id);
  Temp = myData.temp;
  // Serial.print("Temperature: ");
  // Serial.println(myData.temp);
  Hum = myData.hum;
  // Serial.print("Humidity: ");
  // Serial.println(myData.hum);
  // Serial.print("Packets Send: ");
  // Serial.println(myData.id);
  // Serial.println();
}

// Programing time interval

int ActivationCondition(const int timeStart,
                        const int timeStartM,
                        const int timeEnd,
                        const int timeEndM)
{
  bool StartFlag = false;
  bool StopFlag = false;

  if (atoi(&timeHour[0]) > timeStart)
  {
    StartFlag = true;
  }
  else if (atoi(&timeHour[0]) == timeStart)
  {
    if (atoi(&timeMin[0]) >= timeStartM)
    {
      StartFlag = true;
    }
  }
  else
  {
    StartFlag = false;
  }

  if (atoi(&timeHour[0]) < timeEnd)
  {
    StopFlag = true;
  }
  else if (atoi(&timeHour[0]) == timeEnd)
  {
    if (atoi(&timeMin[0]) <= timeEndM)
    {
      StopFlag = true;
    }
  }
  else
  {
    StopFlag = false;
  }

  if (StartFlag == true && StopFlag == true)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void espSendData()
{
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
  if (result == ESP_OK)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }
}
//////////////////////////////// Aditional Functions ///////////////////////////////////

const int timeMinus(const int time_A)
{
  if (time_A == 00)
  {
    return 00;
  }
  else
  {
    return (time_A - 1);
  }
}

const int timePlus(const int time_A)
{
  if (time_A == 59)
  {
    return 59;
  }
  else
  {
    return (time_A + 1);
  }
}

/////////////////////////////////////////////Setup/////////////////////////////////////////

void setup()
{
  pinMode(Releu, OUTPUT);
  // Init Serial Monitor
  Serial.begin(115200);

  DataStorageRead(0, 130);
  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  Serial.println("Establishing connection to WiFi with SSID: " + String(ssid)); // print SSID to the serial interface for debugging

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP()); // show IP address that the ESP32 has received from router
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  server.on("/", []() {                     // define here wat the webserver needs to do
    server.send(200, "text/html",
    
    // The String below "webpage" contains the complete HTML code that is sent to the client whenever someone connects to the webserver
    F("<!DOCTYPE html><html><head> <title>Controler Ambiental</title> <style> body { background-color: #404040; font-family: Arial, Helvetica, sans-serif; color: lightgrey; } input { border-width: 0px; } .rh { height: 21px; } .rh2 { height: 36px; } .tdc { border-style: solid; border-width: 1px; border-color: whitesmoke; } .tdcb { font-weight: bold; } .tdcc { text-align: center; } .tdt { border-top-width: 2px; } .title_r { width: 88.7667px; height: 34px; text-align: center; font-size: 20px; font-weight: bold; } .startt { color: #00FF00; } .endt { color: #FF0000; } .tbl { float: left; height: 298px; width: 700px; border-style: solid; border-collapse: collapse; border-width: 2px; border-color: whitesmoke; } </style></head><body> <h1>Controler Centrala Termica <span id='datetime'>-</span></h1> <p>Temperatura actuala: <span id='temp'>-</span> &deg;C Umiditate relativa: <span id='hum'>-</span> %</p> <p>Stare centrala: <span id='StatusCentrala'>-</span> Program: <span id='Program'>-</span></p> <p>Toleranta temperatura: &plusmn; <input id='OffsetTemp' size='2' /> &deg;C</p> <p> <button type='button' id='BTN_SEND_BACK'>Salvare programare</button> <button type='button' id='BTN_READ'>Citire programare</button> </p> <table class='tbl'> <tbody> <tr class='rh title_r'> <td colspan=2 class='tdc'>Luni</td> <td colspan=2 class='tdc'>Marti</td> <td colspan=2 class='tdc'>Miercuri</td> <td colspan=2 class='tdc'>Joi</td> <td colspan=2 class='tdc'>Vineri</td> <td colspan=2 class='tdc'>Sambata</td> <td colspan=2 class='tdc'>Duminica</td> </tr> <tr class='rh2'> <td colspan=14 class='tdc tdcb tdcc tdt'>Program 1</td> </tr> <tr class='tth startt'> <td colspan=2 class='tdc'>Start 00:00</td> <td colspan=2 class='tdc'>Start 00:00</td> <td colspan=2 class='tdc'>Start 00:00</td> <td colspan=2 class='tdc'>Start 00:00</td> <td colspan=2 class='tdc'>Start 00:00</td> <td colspan=2 class='tdc'>Start 00:00</td> <td colspan=2 class='tdc'>Start 00:00</td> </tr> <tr class='rh'> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> </tr> <tr class='rh'> <td colspan=2 class='tdc'><input id='tVal_1_1' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_1_2' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_1_3' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_1_4' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_1_5' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_1_6' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_1_7' size=2 type='text' /> &deg;C</td> </tr> <tr class='rh2'> <td colspan=14 class='tdc tdcb tdcc tdt'>Program 2</td> </tr> <tr class='rh'> <td class='tdc'><input id='hVal_2_1' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_2_1' size=2 type='text' /></td> <td class='tdc'><input id='hVal_2_2' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_2_2' size=2 type='text' /></td> <td class='tdc'><input id='hVal_2_3' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_2_3' size=2 type='text' /></td> <td class='tdc'><input id='hVal_2_4' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_2_4' size=2 type='text' /></td> <td class='tdc'><input id='hVal_2_5' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_2_5' size=2 type='text' /></td> <td class='tdc'><input id='hVal_2_6' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_2_6' size=2 type='text' /></td> <td class='tdc'><input id='hVal_2_7' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_2_7' size=2 type='text' /></td> </tr> <tr class='rh'> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> </tr> <tr class='rh'> <td colspan=2 class='tdc'><input id='tVal_2_1' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_2_2' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_2_3' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_2_4' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_2_5' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_2_6' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_2_7' size=2 type='text' /> &deg;C</td> </tr> <tr class='rh2'> <td colspan=14 class='tdc tdcb tdcc tdt'>Program 3</td> </tr> <tr class='rh'> <td class='tdc'><input id='hVal_3_1' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_3_1' size=2 type='text' /></td> <td class='tdc'><input id='hVal_3_2' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_3_2' size=2 type='text' /></td> <td class='tdc'><input id='hVal_3_3' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_3_3' size=2 type='text' /></td> <td class='tdc'><input id='hVal_3_4' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_3_4' size=2 type='text' /></td> <td class='tdc'><input id='hVal_3_5' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_3_5' size=2 type='text' /></td> <td class='tdc'><input id='hVal_3_6' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_3_6' size=2 type='text' /></td> <td class='tdc'><input id='hVal_3_7' size=2 width='20' type='text' /></td> <td class='tdc'><input id='mVal_3_7' size=2 type='text' /></td> </tr> <tr class='rh'> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> <td colspan=2 class='tdc'>Temperatura</td> </tr> <tr class='rh'> <td colspan=2 class='tdc'><input id='tVal_3_1' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_3_2' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_3_3' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_3_4' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_3_5' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_3_6' size=2 type='text' /> &deg;C</td> <td colspan=2 class='tdc'><input id='tVal_3_7' size=2 type='text' /> &deg;C</td> </tr> <tr class='rh endt'> <td colspan=2 class='tdc'>Stop 23:59</td> <td colspan=2 class='tdc'>Stop 23:59</td> <td colspan=2 class='tdc'>Stop 23:59</td> <td colspan=2 class='tdc'>Stop 23:59</td> <td colspan=2 class='tdc'>Stop 23:59</td> <td colspan=2 class='tdc'>Stop 23:59</td> <td colspan=2 class='tdc'>Stop 23:59</td> </tr> </tbody> </table></body><script> var Socket; document.getElementById('BTN_SEND_BACK').addEventListener('click', button_send_back); document.getElementById('BTN_READ').addEventListener('click', button_receive); function init() { Socket = new WebSocket('ws://192.168.100.20:81/'); Socket.onmessage = function(event) { processCommand(event); }; } function button_receive(){ var msg = { msg_type: 2 }; Socket.send(JSON.stringify(msg)); } function button_send_back() { var msg = { msg_type : 1, OffsetTemp : document.getElementById('OffsetTemp').value, tVal_1_1 : document.getElementById('tVal_1_1').value, hVal_2_1 : isHour(document.getElementById('hVal_2_1').value,document.getElementById('hVal_3_1').value), mVal_2_1 : isMin(document.getElementById('mVal_2_1').value), tVal_2_1 : document.getElementById('tVal_2_1').value, hVal_3_1 : isHour(document.getElementById('hVal_3_1').value,24), mVal_3_1 : isMin(document.getElementById('mVal_3_1').value), tVal_3_1 : document.getElementById('tVal_3_1').value, tVal_1_2 : document.getElementById('tVal_1_2').value, hVal_2_2 : isHour(document.getElementById('hVal_2_2').value,document.getElementById('hVal_3_2').value), mVal_2_2 : isMin(document.getElementById('mVal_2_2').value), tVal_2_2 : document.getElementById('tVal_2_2').value, hVal_3_2 : isHour(document.getElementById('hVal_3_2').value,24), mVal_3_2 : isMin(document.getElementById('mVal_3_2').value), tVal_3_2 : document.getElementById('tVal_3_2').value, tVal_1_3 : document.getElementById('tVal_1_3').value, hVal_2_3 : isHour(document.getElementById('hVal_2_3').value,document.getElementById('hVal_3_3').value), mVal_2_3 : isMin(document.getElementById('mVal_2_3').value), tVal_2_3 : document.getElementById('tVal_2_3').value, hVal_3_3 : isHour(document.getElementById('hVal_3_3').value,24), mVal_3_3 : isMin(document.getElementById('mVal_3_3').value), tVal_3_3 : document.getElementById('tVal_3_3').value, tVal_1_4 : document.getElementById('tVal_1_4').value, hVal_2_4 : isHour(document.getElementById('hVal_2_4').value,document.getElementById('hVal_3_4').value), mVal_2_4 : isMin(document.getElementById('mVal_2_4').value), tVal_2_4 : document.getElementById('tVal_2_4').value, hVal_3_4 : isHour(document.getElementById('hVal_3_4').value,24), mVal_3_4 : isMin(document.getElementById('mVal_3_4').value), tVal_3_4 : document.getElementById('tVal_3_4').value, tVal_1_5 : document.getElementById('tVal_1_5').value, hVal_2_5 : isHour(document.getElementById('hVal_2_5').value,document.getElementById('hVal_3_5').value), mVal_2_5 : isMin(document.getElementById('mVal_2_5').value), tVal_2_5 : document.getElementById('tVal_2_5').value, hVal_3_5 : isHour(document.getElementById('hVal_3_5').value,24), mVal_3_5 : isMin(document.getElementById('mVal_3_5').value), tVal_3_5 : document.getElementById('tVal_3_5').value, tVal_1_6 : document.getElementById('tVal_1_6').value, hVal_2_6 : isHour(document.getElementById('hVal_2_6').value,document.getElementById('hVal_3_6').value), mVal_2_6 : isMin(document.getElementById('mVal_2_6').value), tVal_2_6 : document.getElementById('tVal_2_6').value, hVal_3_6 : isHour(document.getElementById('hVal_3_6').value,24), mVal_3_6 : isMin(document.getElementById('mVal_3_6').value), tVal_3_6 : document.getElementById('tVal_3_6').value, tVal_1_7 : document.getElementById('tVal_1_7').value, hVal_2_7 : isHour(document.getElementById('hVal_2_7').value,document.getElementById('hVal_3_7').value), mVal_2_7 : isMin(document.getElementById('mVal_2_7').value), tVal_2_7 : document.getElementById('tVal_2_7').value, hVal_3_7 : isHour(document.getElementById('hVal_3_7').value,24), mVal_3_7 : isMin(document.getElementById('mVal_3_7').value), tVal_3_7 : document.getElementById('tVal_3_7').value }; Socket.send(JSON.stringify(msg)); } function processCommand(event) { var obj = JSON.parse(event.data); if(obj.msg_type == '1') { document.getElementById('OffsetTemp').value = obj.OffsetTemp.toFixed(1); document.getElementById('tVal_1_1').value = obj.tVal_1_1.toFixed(1);document.getElementById('hVal_2_1').value = obj.hVal_2_1; document.getElementById('mVal_2_1').value = obj.mVal_2_1; document.getElementById('tVal_2_1').value = obj.tVal_2_1.toFixed(1); document.getElementById('hVal_3_1').value = obj.hVal_3_1; document.getElementById('mVal_3_1').value = obj.mVal_3_1; document.getElementById('tVal_3_1').value = obj.tVal_3_1.toFixed(1); document.getElementById('tVal_1_2').value = obj.tVal_1_2.toFixed(1); document.getElementById('hVal_2_2').value = obj.hVal_2_2; document.getElementById('mVal_2_2').value = obj.mVal_2_2; document.getElementById('tVal_2_2').value = obj.tVal_2_2.toFixed(1); document.getElementById('hVal_3_2').value = obj.hVal_3_2; document.getElementById('mVal_3_2').value = obj.mVal_3_2; document.getElementById('tVal_3_2').value = obj.tVal_3_2.toFixed(1); document.getElementById('tVal_1_3').value = obj.tVal_1_3.toFixed(1); document.getElementById('hVal_2_3').value = obj.hVal_2_3; document.getElementById('mVal_2_3').value = obj.mVal_2_3; document.getElementById('tVal_2_3').value = obj.tVal_2_3.toFixed(1); document.getElementById('hVal_3_3').value = obj.hVal_3_3; document.getElementById('mVal_3_3').value = obj.mVal_3_3; document.getElementById('tVal_3_3').value = obj.tVal_3_3.toFixed(1); document.getElementById('tVal_1_4').value = obj.tVal_1_4.toFixed(1); document.getElementById('hVal_2_4').value = obj.hVal_2_4; document.getElementById('mVal_2_4').value = obj.mVal_2_4; document.getElementById('tVal_2_4').value = obj.tVal_2_4.toFixed(1); document.getElementById('hVal_3_4').value = obj.hVal_3_4; document.getElementById('mVal_3_4').value = obj.mVal_3_4; document.getElementById('tVal_3_4').value = obj.tVal_3_4.toFixed(1); document.getElementById('tVal_1_5').value = obj.tVal_1_5.toFixed(1); document.getElementById('hVal_2_5').value = obj.hVal_2_5; document.getElementById('mVal_2_5').value = obj.mVal_2_5; document.getElementById('tVal_2_5').value = obj.tVal_2_5.toFixed(1); document.getElementById('hVal_3_5').value = obj.hVal_3_5; document.getElementById('mVal_3_5').value = obj.mVal_3_5; document.getElementById('tVal_3_5').value = obj.tVal_3_5.toFixed(1); document.getElementById('tVal_1_6').value = obj.tVal_1_6.toFixed(1); document.getElementById('hVal_2_6').value = obj.hVal_2_6; document.getElementById('mVal_2_6').value = obj.mVal_2_6; document.getElementById('tVal_2_6').value = obj.tVal_2_6.toFixed(1); document.getElementById('hVal_3_6').value = obj.hVal_3_6; document.getElementById('mVal_3_6').value = obj.mVal_3_6; document.getElementById('tVal_3_6').value = obj.tVal_3_6.toFixed(1); document.getElementById('tVal_1_7').value = obj.tVal_1_7.toFixed(1); document.getElementById('hVal_2_7').value = obj.hVal_2_7; document.getElementById('mVal_2_7').value = obj.mVal_2_7; document.getElementById('tVal_2_7').value = obj.tVal_2_7.toFixed(1); document.getElementById('hVal_3_7').value = obj.hVal_3_7; document.getElementById('mVal_3_7').value = obj.mVal_3_7; document.getElementById('tVal_3_7').value = obj.tVal_3_7.toFixed(1); } else { document.getElementById('temp').innerHTML = obj.temp.toFixed(2); document.getElementById('hum').innerHTML = obj.hum.toFixed(2); document.getElementById('StatusCentrala').innerHTML = obj.StatusCentrala; document.getElementById('Program').innerHTML = obj.Program; document.getElementById('datetime').innerHTML = obj.datetime; } } window.onload = function(event) { init(); }; function isHour(vHour,nextHour) { if (Number.isInteger(parseInt(vHour))) { if(vHour < 24) { if(vHour < parseInt(nextHour)){ return vHour; } else{ var resoult = parseInt(nextHour)-1; return resoult; } }else{ var resoult = parseInt(nextHour)-1; return resoult; } } else { var resoult = parseInt(nextHour)-1; return resoult; } }; function isMin(vMin) { var retNum = 59; if (Number.isInteger(parseInt(vMin))) { if(vMin < 60) { return vMin; } else{ return parseInt(retNum); } } else { return parseInt(retNum); } };</script></html>")
  );});
  server.begin(); // start server

  webSocket.begin();                 // start websocket
  webSocket.onEvent(webSocketEvent); // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"

  WiFi.mode(WIFI_AP_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  // esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }

  tempval[OffsetTemp] = 1.5;
  
}

void loop()
{
  printLocalTime();
  // espSendData();

  int nwd = atoi(&n_week_day[0]); //number week day (1-Luni, 2-Marti...)

  if (ActivationCondition(00, 00, timeval[(nwd - 1) * timeoffs], timeval[(nwd - 1) * timeoffs + 1]) == 1) // Dimineata
  {
    if(StatusCentrala == 1)
    {
      if(Temp >= tempval[(nwd - 1) * tempoffs + 1] + tempval[OffsetTemp])
      {
        StatusCentrala = 0;
      }
    }
    else
    {
      if(Temp < tempval[(nwd - 1) * tempoffs + 1] - tempval[OffsetTemp])
      {
        StatusCentrala = 1;
      }
    }

    Serial.println("Dimineata");
    Program = 1;
  }
  else if (ActivationCondition(timeval[(nwd - 1) * timeoffs], timePlus(timeval[(nwd - 1) * timeoffs + 1]), timeval[(nwd - 1) * timeoffs + 2], timeval[(nwd - 1) * timeoffs + 3]) == 1) // Dupamasa
  {
    if(StatusCentrala == 1)
    {
      if(Temp >= tempval[(nwd - 1) * tempoffs + 2] + tempval[OffsetTemp])
      {
        StatusCentrala = 0;
      }
    }
    else
    {
      if (Temp < tempval[(nwd - 1) * tempoffs + 2] - tempval[OffsetTemp])
      {
        StatusCentrala = 1;
      }
    }

    Serial.println("Dupamasa");
    Program = 2;
  }
  else if (ActivationCondition(timeval[(nwd - 1) * timeoffs + 2], timePlus(timeval[(nwd - 1) * timeoffs + 3]), 23, 59) == 1)
  {

    if(StatusCentrala == 1)
    {
      if(Temp >= tempval[(nwd - 1) * tempoffs + 3] + tempval[OffsetTemp])
      {
        StatusCentrala = 0;
      }
    }
    else
    {
      if (Temp < tempval[(nwd - 1) * tempoffs + 3] - tempval[OffsetTemp])
      {
        StatusCentrala = 1;
      }  
    }
    
    Serial.println("Seara");
    Program = 3;
  }
  else
  {
    Serial.println("Valoare nedefinita in <> if");
  }
  
  if(StatusCentrala == 1){
    digitalWrite(Releu,HIGH);
  }else{
    digitalWrite(Releu,LOW);
  };


  WebSoketUpdate();
  delay(1000);
}