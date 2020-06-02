#include <SoftwareSerial.h> //Used for transmitting to the device

SoftwareSerial softSerial(2, 3); //RX, TX

#include "SparkFun_UHF_RFID_Reader.h" //Library for controlling the M6E Nano module
RFID nano; //Create instance
byte UIDarray[10][8];
byte numberOfTags = 0;
String cart = "";

void setup()
{
  Serial.begin(115200);

  while (!Serial);
  Serial.println();
  Serial.println("Initializing...");

  if (setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    Serial.println("Module failed to respond. Please check wiring.");
    while (1); //Freeze!
  }

  nano.setRegion(REGION_NORTHAMERICA); //Set to North America

  nano.setReadPower(500); //5.00 dBm. Higher values may cause USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling
}

void loop()
{
  String productNumber = "";
  
  Serial.println(F("Press a key to read user data"));
  while (!Serial.available()); //Wait for user to send a character
  Serial.read(); //Throw away the user's character

  //Read the data from the tag
  byte responseType;
  byte myData[14];
  byte myDataLength = sizeof(myData); //Tell readUserData to read up to 64 bytes
  byte fault[8] = {0,0,0,0,0,0,0,0};
  boolean duplicate = false;
  byte faultCount = 0;
  
  responseType = nano.readUserData(myData, myDataLength); //readUserData will modify myDataLength to the actual # of bytes read

  if (responseType == RESPONSE_SUCCESS)
  {
    //Print User Data
    Serial.print(F("Size: ["));
    Serial.print(myDataLength);
    Serial.print(F("] User data: ["));
    for (byte x = 0 ; x < myDataLength ; x++)
    {
      if (myData[x] < 0x10) Serial.print(F("0"));
      Serial.print(myData[x], HEX);
      Serial.print(F(" "));
      if (myData[x] == 0x30)
        productNumber.concat(0);
      else if (myData[x] == 0x31)
        productNumber.concat(1);
      else if (myData[x] == 0x32)
        productNumber.concat(2);
      else if (myData[x] == 0x33)
        productNumber.concat(3);
      else if (myData[x] == 0x34)
        productNumber.concat(4);
      else if (myData[x] == 0x35)
        productNumber.concat(5);
      else if (myData[x] == 0x36)
        productNumber.concat(6);
      else if (myData[x] == 0x37)
        productNumber.concat(7);
      else if (myData[x] == 0x38)
        productNumber.concat(8);
      else if (myData[x] == 0x39)
        productNumber.concat(9);
    }
    Serial.println(F("]"));
    Serial.println(productNumber); 
  }
  else
    Serial.println(F("Error reading tag data"));

//---------------------duplicate detection-------------------------------------
for(byte x = 0; x < 10; x++){
  for(byte y = 0; y < 8; y++){
    if(myData[y] == UIDarray[x][y])
      fault[y]++;
  }
  
Serial.print(F("Faults: "));
for(byte x = 0; x < 8; x++){
  Serial.print(fault[x]);
}     
}
Serial.println();

for (byte x = 0; x < 8; x++)
  if(fault[x] == 1)
    faultCount++;

if(faultCount == 8){
    Serial.print(F("Duplicate tag detected! Tag not added to array!"));
    duplicate = true;
}

if(duplicate == false){
  for(byte y = 0; y < 8; y++)
    UIDarray[numberOfTags][y] = myData[y];
  Serial.print(F("Tag added to array!"));
  cart += productNumber;
  numberOfTags++;
}//if duplicate statement

Serial.print(F("All Skus in cart: "));
Serial.println(cart);

for(byte x = 0; x < 10; x++){
    Serial.println();
    for(byte y = 0; y < 8; y++){
      Serial.print(UIDarray[x][y]);
      Serial.print(F(" "));
  }
}
Serial.println();



}//end void loop

//Gracefully handles a reader that is already configured and already reading continuously
//Because Stream does not have a .begin() we have to do this outside the library
boolean setupNano(long baudRate)
{
  nano.begin(softSerial); //Tell the library to communicate over software serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  softSerial.begin(baudRate); //For this test, assume module is already at our desired baud rate
  while (!softSerial); //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while (softSerial.available()) softSerial.read();

  nano.getVersion();

  if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    //This happens if the baud rate is correct but the module is doing a ccontinuous read
    nano.stopReading();

    Serial.println(F("Module continuously reading. Asking it to stop..."));

    delay(1500);
  }
  else
  {
    //The module did not respond so assume it's just been powered on and communicating at 115200bps
    softSerial.begin(115200); //Start software serial at 115200

    nano.setBaud(baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

    softSerial.begin(baudRate); //Start the software serial port, this time at user's chosen baud rate
  }

  //Test the connection
  nano.getVersion();
  if (nano.msg[0] != ALL_GOOD) return (false); //Something is not right

  //The M6E has these settings no matter what
  nano.setTagProtocol(); //Set protocol to GEN2

  nano.setAntennaPort(); //Set TX/RX antenna ports to 1

  return (true); //We are ready to rock
}
