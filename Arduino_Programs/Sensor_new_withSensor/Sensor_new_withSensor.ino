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

    DHT sensor added at D4

    TODO: format sensor readings to be only max 5 digits
*/

#include <SoftwareSerial.h>
SoftwareSerial loraSerial(14, 12, false); // (rxPin (D5), txPin (D6), inverse_logic, buffer size);

#include "DHT.h"
#define DHTPIN D4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11    

DHT dht(DHTPIN, DHTTYPE);

// Temporary variables
static char celsiusTemp[7];
static char humidityTemp[7];


String str;
String frequency = "869100000";
String sf = "sf12";
String message = "48656c6C6F";
String receivedMessage;
String ID_receive;

int currentMillis;
int startingMillis;
int deltaTime;
int SleepTime;

#define red 0     // D3
#define yellow 5   // D1
#define green 4    // D2



String ID = "11";
float s1 = 12.12;
float s2 = 13.13;
float s3 = 44.44;

String ID_CB = "00";

void setup() {
  //output LED pin
  pinMode(13, OUTPUT);  // D7 on ESP8266

  pinMode(red, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);

  // Open serial communications and wait for port to open:
  Serial.begin(57600);  // Serial communication to PC
  dht.begin();

  loraSerial.begin(9600);  // Serial communication to RN2483
  loraSerial.setTimeout(1000);
  lora_autobaud();


  InitialisingLora(frequency, sf);

  //LED
  LED_sensor();

  Serial.println("starting loop");
}

//=============================================================
// LOOP
//=============================================================
void loop() {
  // send message for defined time
  startingMillis = millis();
  int sendingTime = 10000;
  currentMillis = millis();
  deltaTime = currentMillis - startingMillis;

  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  int failcounter = 0;
  readSensor:
  delay(1000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float hic = dht.computeHeatIndex(t, h, false);
  // You can delete the following Serial.print's, it's just for debugging purposes
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print("  Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(" Heat index: ");
  Serial.print(hic);
  Serial.println(" *C ");

  // Check if any reads failed and exit early (to try again).
  if ((isnan(h) || isnan(t)) && failcounter <= 3) {
    Serial.println("Failed to read from DHT sensor!");
    strcpy(celsiusTemp, "Failed");
    strcpy(humidityTemp, "Failed");

    delay(1000);
    failcounter = failcounter +1;
    Serial.print(failcounter);
    goto readSensor;  
  }


  message = makeMessage_Sensor(ID, h, t, hic);
  while (deltaTime <= sendingTime) {
    sendMessage(message);
    currentMillis = millis();
    deltaTime = currentMillis - startingMillis;
    yield(); // so watchdog doesnt reset controller
    delay(100);
  }
receiveCB: // State
  receivedMessage = receiveMessage();
  if (receivedMessage != "") {
    // remove radio rx
    String CBmessage = receivedMessage.substring(11);
    Serial.print("cut message: ");
    Serial.println(CBmessage);
    // get ID
    String ID_receive = CBmessage.substring(0, 2);
    Serial.print("ID:");
    Serial.println(ID_receive);
    if (ID_receive == ID_CB) {
      //message coming from CB
      // get sleep time
      String SleepTimeStr = CBmessage.substring(2);
      Serial.print("Sleep Time: ");
      Serial.println(SleepTimeStr);
      // convert string to int
      SleepTime = SleepTimeStr.toInt();
      Serial.print("Sleep for [ms]: ");
      Serial.println(SleepTime);
      led_on(yellow);
      delay(SleepTime);
      led_off(yellow);
    }
    else {
      //message coming not from CB
      Serial.print("Receive the wrong ID:");
      Serial.println(ID_receive);
      // jump back to state receiveCB
      goto receiveCB;

    }

  }
  else {
    // unsuccesful commuincation
    LED_blinker(red);
    Serial.println("No message received");
    Serial.println("Sleep for short time");
    led_on(yellow);
    delay(5000);
    led_off(yellow);
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


void led_on(int LED) {
  digitalWrite(LED, 1);
}

void led_off(int LED) {
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
  loraSerial.println("radio rxstop"); // causes the radio to exit Continuous Receive mode
  str = loraSerial.readStringUntil('\n');
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
  delay(200);
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
    delay(1000);
  }
  led_off(green);
  return message;
}

void LED_sensor() {
  // Sensor LED show
  led_off(green);
  led_off(yellow);
  led_off(red);
  delay(500);
  led_on(red);
  led_on(yellow);
  led_on(green);
  delay(250);
  led_off(green);
  delay(250);
  led_off(yellow);
  delay(250);
  led_off(red);
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

// message encoding and decoding
String encodeMessage(String message) {
  message.replace(".", "A");

  int len = message.length();

  if (len > 6){
  	message.substring(0,5);
  }

  return message;
}

float decodeMessage(String Sensor) {
  Sensor.replace("A", ".");
  float Sensor_float = Sensor.toFloat();
  return Sensor_float;
}

String makeMessage_Sensor(String ID, float s1, float s2, float s3) {
  String sns1 = encodeMessage(String(s1));\
  String sns2 = encodeMessage(String(s2));
  String sns3 = encodeMessage(String(s3));

  String message = ID + sns1 + sns2 + sns3;

  return message;
}



