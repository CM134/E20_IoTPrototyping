/*
   Lora to Lora (P2P) using RN2484 and ESP8266
   This sketch will transmit a lora message with payload "20" every 2 seconds.
   An LED will blink every time a package is sent. Payload and package ID is printed on serial monitor.
   Code based on JP Meijers code from 30 September 2016
   MN Petersen, Aug 2020
   CONNECTIONS:
   RN2483 - ESP8266:
   TX     - D5
   RX     - D6
   3V3    - 3V3
   GND    - GND
   D7 on ESP8266 is connected to anode (long leg) on LED. Insert resistor between cathode and GND on ESP8266.
   Works without the delimiter message: message frame |ID|SEN1|SEN2|SEN3| = |2|5|5|5|
    CB message frame: |ID|SLEEP| = |00|max16|
*/

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h> 
SoftwareSerial loraSerial(14, 12, false); // (rxPin (D5), txPin (D6), inverse_logic, buffer size);

String str;
String frequency = "869100000";
String sf = "sf12";
String message = "48656c6C6F";
String receivedMessage;
String messageArray_Sensor[4];
String ID_sensor;
float Sensor_values[3];

int currentMillis;
int startingMillis;
int deltaTime;



#define red 0     // D3
#define yellow 5   // D1
#define green 4    // D2

String ID = "00";
int SleepTime = 10000;

// Replace with your network details
const char* ssid = "Chachou";
const char* password = "1122334455";

WiFiClient client;

unsigned long channelID = 1199888; //your channel
const char * myWriteAPIKey = "VZLKTACP3KLS4ZSU"; // your WRITE API key
const char* server = "api.thingspeak.com";

const int postingInterval = 20 * 1000; // post data every 20 seconds



void setup() {
  //output LED pin
  pinMode(13, OUTPUT);  // D7 on ESP8266

  pinMode(red, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);

  // Open serial communications and wait for port to open:
  Serial.begin(57600);  // Serial communication to PC

  loraSerial.begin(9600);  // Serial communication to RN2483
  loraSerial.setTimeout(1000);
  lora_autobaud();


  InitialisingLora(frequency, sf);

  // LEDs
  LED_CentralBox();

  // Connecting to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Serial.println("starting loop");


}

//=============================================================
// LOOP
//=============================================================
void loop() {

  receivedMessage = receiveMessage();

  if (receivedMessage != "") {
    Serial.print("Received Message (unprocessed): ");
    Serial.println(receivedMessage);

    //Work decode Received message and print individual

    // remove radio rx
    String Senmessage = receivedMessage.substring(11);
    Serial.print("cut message: ");
    Serial.println(Senmessage);
    // get ID
    String ID_receive = Senmessage.substring(0, 2);
    Serial.print("ID:");
    Serial.println(ID_receive); 


    // Slice sensor message
    String sen1 = Senmessage.substring(2, 7);
    Sensor_values[0] = decodeMessage(sen1);
    Serial.print("Value 1: ");
    Serial.print(sen1);
    Serial.print(" -> ");
    Serial.println(Sensor_values[0]);
    String sen2 = Senmessage.substring(7, 12);
    Sensor_values[1] = decodeMessage(sen2);
    Serial.print("Value 2: ");
    Serial.print(sen2);
    Serial.print(" -> ");
    Serial.println(Sensor_values[1]);
    String sen3 = Senmessage.substring(12, 17);
    Sensor_values[2] = decodeMessage(sen3);
    Serial.print("Value 3: ");
    Serial.print(sen3);
    Serial.print(" -> "); 
    Serial.println(Sensor_values[2]);

    // Wait until sensor is done sending: defined time is 10000
    led_on(yellow),
           delay(15000);
    led_off(yellow);

    // Send message:
    message = makeMessage_CB(ID, SleepTime);
    Serial.println("Sending AKK+SleepTime");
    sendMessage(message);
    delay(100);

    ThingSpeak.begin(client);
    ThingSpeak.setField(1,Sensor_values[0]);
    ThingSpeak.setField(2,Sensor_values[1]);
    ThingSpeak.setField(3,Sensor_values[2]);
    ThingSpeak.writeFields(channelID, myWriteAPIKey);
    client.stop();
    delay(postingInterval);
    
  }
  else {
    Serial.println("No message received");
  }
}


//=============================================================
// Function declarations
//=============================================================

void lora_autobaud() {
  String response = "";
  while (response == "")
  {
    delay(1000);
    loraSerial.write((byte)0x00);
    loraSerial.write(0x55);
    loraSerial.println();
    loraSerial.println("sys get ver");
    response = loraSerial.readStringUntil('\n');
  }
}


int wait_for_ok() {
  /*
    This function blocks until the word "ok\n" is received on the UART,
    or until a timeout of 3*5 seconds.
  */
  str = loraSerial.readStringUntil('\n');
  if ( str.indexOf("ok") == 0 ) {
    return 1;
  }
  else return 0;
}


void led_on(int LED)
{
  digitalWrite(LED, 1);
}

void led_off(int LED)
{
  digitalWrite(LED, 0);
}



void InitialisingLora(String frequency, String sf) {
  Serial.println("Initing LoRa");

  loraSerial.println("radio rxstop"); // causes the radio to exit Continuous Receive mode
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.listen();
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
  loraSerial.println("sys get ver");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("mac pause");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  //  loraSerial.println("radio set bt 0.5");  // Uncomment if we want to use FSK
  //  wait_for_ok();

  loraSerial.println("radio set mod lora"); // Comment if we want to use FSK
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set freq "  + frequency);
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set pwr 14");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set sf " + sf);
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set afcbw 41.7");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set rxbw 125");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  //  loraSerial.println("radio set bitrate 50000");
  //  wait_for_ok();

  //  loraSerial.println("radio set fdev 25000");
  //  wait_for_ok();

  loraSerial.println("radio set prlen 8");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set crc on");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set iqi off");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set cr 4/5");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set wdt 60000"); //disable for continuous reception
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set sync 12");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set bw 125");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);
}

void sendMessage(String message) {
  led_on(red);
  Serial.print("Sending Message: ");
  Serial.println(message);
  delay(500);
  Serial.println("TX: radio tx");
  loraSerial.print("radio tx ");
  loraSerial.println(message);
  str = loraSerial.readStringUntil('\n');
  Serial.print("RX: ");
  Serial.println(str);
  str = loraSerial.readStringUntil('\n');
  Serial.print("RX: ");
  Serial.println(str);
  delay(1000);
  led_off(red);

}

String receiveMessage() {
  led_on(green);
  // intialise emtpy message
  String message = "";
  Serial.println("Receving Message");
  delay(1000);
  Serial.println("TX: radio rx 0");
  loraSerial.println("radio rx 0"); //wait for 60 seconds to receive
  str = loraSerial.readStringUntil('\n');
  Serial.print("RX: ");
  Serial.println(str);
  delay(200);

  if ( str.indexOf("ok") == 0 )
  {
    str = String("");
    while (str == "")
    {
      str = loraSerial.readStringUntil('\n');
    }
    if ( str.indexOf("radio_rx") == 0 )  //checking if data was reeived (equals radio_rx = <data>). indexOf returns position of "radio_rx"
    {
      Serial.println(str); //printing received data
      message = str;
      LED_blinker(green);
    }
    else
    {
      Serial.println("Received nothing");
      LED_blinker(red);
    }
  }
  else if (str.indexOf("busy") == 0 ) {
    Serial.println("radio not going into receive mode");
    Serial.println("  Stop the radio and mac pause");
    LED_blinker(red);
    delay(500);
    // dont know if mac pause is needed, but it is required to be in init
    loraSerial.println("mac pause");
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);

    loraSerial.println("radio rxstop"); // causes the radio to exit Continuous Receive mode
    str = loraSerial.readStringUntil('\n');
    Serial.println(str);

  }
  else
  {
    Serial.println("radio not going into receive mode");
    LED_blinker(red);
    delay(500);
  }
  led_off(green);

  return message;
}

void LED_CentralBox() {
  // Central Box LED show
  led_off(green);
  led_off(yellow);
  led_off(red);
  delay(500);
  led_on(red);
  delay(500);
  led_off(red);
  led_on(yellow);
  delay(500);
  led_off(yellow);
  led_on(green);
  delay(500);
  led_off(green);
  delay(250);
}

void LED_blinker(int LED) {
  led_off(LED);
  delay(100);
  led_on(LED);
  delay(100);
  led_off(LED);
  delay(100);
  led_on(LED);
  delay(100);
  led_off(LED);
  delay(100);
  led_on(LED);
  delay(100);
  led_off(LED);
}


int scheduler(int ID, int Prio) {
  // device ID and max number of devices
  // For scheduler:
  int SlotTime = 120000;  //2*60*1000
  int SleepTime = Prio * SlotTime;
  return SleepTime;
}

// message encoder------------------------------------------------
String encodeMessage(String message) {
  message.replace(".", "A");
  return message;
}

float decodeMessage(String Sensor) {
  Sensor.replace("A", ".");
  float Sensor_float = Sensor.toFloat();
  return Sensor_float;
}

String makeMessage_CB(String ID, int SleepTime) {
  String message = ID + String(SleepTime);
  message = encodeMessage(message);
  return message;
}



String splitMessage(String message) {
  int  len = message.length();
  int StartIndex = 0;
  char delimiter = 'B';
  int counter = 0;
  String messageArray[4] = {"", "", "", ""};

  for (int i = 0; i <= len; i++) {
    char currentChar;
    currentChar = message[i];

    if (currentChar == delimiter) {
      // get the chars and write them in one string}
      for (int j = StartIndex; j <= i - 1; j++) {
        messageArray[counter] = messageArray[counter] + message[j];
      }

      StartIndex = i + 1;
      Serial.print(counter);
      Serial.print(" ");
      Serial.println(messageArray[counter]);
      counter = counter + 1;
    }
  }
  //gets last element after last delimiter
  for (int j = StartIndex; j <= len; j++) {
    messageArray[counter] = messageArray[counter] + message[j];
  }
  Serial.print(counter);
  Serial.print(" ");
  Serial.println(messageArray[counter]);
  return messageArray[4];
}

String getID(String messageArray) {
  // int IDindex = 0;
  // Serial.println(messageArray);
  String ID = messageArray;
  return ID;
}

float getSensors(String messageArray[4]) {
  float s[3];
  s[0] = decodeMessage(messageArray[1]);
  s[1] = decodeMessage(messageArray[2]);
  s[2] = decodeMessage(messageArray[3]);
  return s[3];
}
