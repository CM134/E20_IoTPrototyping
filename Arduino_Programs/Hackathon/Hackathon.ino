/*
   Lora to Lora (P2P) using RN2484 and ESP8266
   This sketch will transmit a lora message with payload "20" every 2 seconds.
   An LED will blink every time a package is sent. Payload and package ID is printed on serial monitor.

   Code based on JP Meijers code from 30 September 2016
   MN Petersen, Aug 2020

   CONNECTIONS:
   RN2483 - ESP8266:
   TX     - D5-PIN
   RX     - D6-PIN
   3V3    - 3V3
   GND    - GND

   D7 on ESP8266 is connected to anode (long leg) on LED. Insert resistor between cathode and GND on ESP8266.

*/

#include <SoftwareSerial.h>
SoftwareSerial loraSerial(14, 12, false); // (rxPin (D5), txPin (D6), inverse_logic, buffer size);

String str;
String frequency = "869100000";
String sf = "sf12";
//String message = "48656c6C6F";
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

  Serial.println("starting loop");


}

//=============================================================
// LOOP
//=============================================================
void loop() {


  Serial.println("=====================");
  Serial.println();
  Serial.println("=====================");
  receivedMessage = receiveMessage();

  if (receivedMessage != "") {
    Serial.print("Received Message (unprocessed): ");
    Serial.println(receivedMessage);

    //Work decode Received message and print individual

    String BoxMessageInit = "radio_rx  52585F5244595F3432\r";
    
    if (receivedMessage == BoxMessageInit) {
      // Its our turn. We want to transmit
      Serial.println("------Its our turn now :-)");
      String ourUserID = "47344B3245";
      sendMessage(ourUserID);
      Jump:
      receivedMessage = receiveMessage();
      int len = receivedMessage.length();
      Serial.println(len);
      
      if (receivedMessage.length() >= 30){
      
      String decodedMes;
      decodedMes = hacking(receivedMessage);
      
      decodedMes = hacking(receivedMessage);

      Serial.println(decodedMes);
      Decoded:
      receivedMessage = receiveMessage();
      
      if (receivedMessage == BoxMessageInit) {
      sendMessage(decodedMes);
      }
      else{
        Serial.println("not id yet");
        goto Decoded;
        }

      }
      else{
        
        goto Jump;
        
      }
       
    }
    else {
      Serial.println("Not right message ID");
    }
  }
  else {
    Serial.println("No message received");
  }
}


//=============================================================
// Function declarations
//=============================================================

void lora_autobaud()
{
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

 

  loraSerial.println("radio set wdt 6000000"); //disable for continuous reception
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

  loraSerial.println("radio set sync 42");
  str = loraSerial.readStringUntil('\n');
  Serial.println(str);

}

void sendMessage(String message) {
  //led_on(red);
  Serial.print("Sending Message: ");
  Serial.println(message);
  //delay(500);
  Serial.println("TX: radio tx");
  loraSerial.print("radio tx ");
  loraSerial.println(message);
  str = loraSerial.readStringUntil('\n');
  Serial.print("RX: ");
  Serial.println(str);
  str = loraSerial.readStringUntil('\n');
  Serial.print("RX: ");
  Serial.println(str);
  //delay(1000);
  //led_off(red);

}



String receiveMessage() {
  //led_on(green);
  // intialise emtpy message
  String message = "";
  Serial.println("Receving Message");
  //delay(1000);
  Serial.println("TX: radio rx 0");
  loraSerial.println("radio rx 0"); //wait for 60 seconds to receive
  str = loraSerial.readStringUntil('\n');
  Serial.print("RX: ");
  Serial.println(str);
  //delay(200);

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
    }
    else
    {
      Serial.println("Received nothing");
    }
  }
  else if (str.indexOf("busy") == 0 ) {
    Serial.println("radio not going into receive mode");
    Serial.println("  Stop the radio and mac pause");
    //LED_blinker(red);
    //delay(500);
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
    //LED_blinker(red);
    //delay(500);
  }
  //led_off(green);

  return message;
}



int scheduler(int ID, int Prio) {
  // device ID and max number of devices
  // For scheduler:
  int SlotTime = 120000;  //2*60*1000
  int SleepTime = Prio * SlotTime;
  return SleepTime;
}




String hacking(String message) {
  int  len = message.length();

  message = message.substring(10, 30);

  String firstChar = message.substring(2, 4);
  String secondChar = message.substring(6, 8);
  String thirdChar = message.substring(10, 12);
  String fourthChar = message.substring(14, 16);
  String fifthChar = message.substring(18, 20);
  String outmessage;


  outmessage = "47" + thirdChar + "34" + firstChar + "4B" + fifthChar + "32" + secondChar + "45" + fourthChar + "\r\n";




  return outmessage;
}
