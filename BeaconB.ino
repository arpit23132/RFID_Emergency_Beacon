#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
// #include <Arduino.h>
// #include <ESP8266HTTPClient.h>

#define RST_PIN  5     // Configurable, see typical pin layout above
#define SS_PIN   21     // Configurable, see typical pin layout above
// #define BUZZER   2    // Configurable, see typical pin layout above
#define EEPROM_SIZE 512 // EEPROM storage size


MFRC522 mfrc522(SS_PIN, RST_PIN);  // Instance of the class
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;  


#include <WiFi.h>
#include <HTTPClient.h>
const char* ssid = "Abhi";
const char* password = "abhi1234";
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbyddwl_SbxtoVd371wMFYGsRv69y0BL7et9V7t-u3hmDLBS88ir5XqyUGyAQTyw4aLt/exec";


int MAX_TAPS = 3;  // Limit of RFID taps

// Function to generate EEPROM address for each UID
int getEEPROMAddress(String uid) {
    int hash = 0;
    for (char c : uid) {
        hash += c;
    }
    return (hash % (EEPROM_SIZE - 4));  // Avoid exceeding EEPROM size
}

// Function to read stored tap count from EEPROM
int getTapCount(String uid) {
    int address = getEEPROMAddress(uid);
    int count;
    EEPROM.get(address, count);
    if (count > MAX_TAPS || count < 0) {
        count = 0; // Reset invalid counts
    }
    return count;
}

// Function to update tap count in EEPROM
void updateTapCount(String uid) {
    int address = getEEPROMAddress(uid);
    int count = getTapCount(uid) + 1;
    EEPROM.put(address, count);
    EEPROM.commit();
}

// Function to check if tap limit is exceeded
bool isLimitExceeded(String uid) {
    return getTapCount(uid) >= MAX_TAPS;
}

/* Be aware of Sector Trailer Blocks */
int block_name = 2;
int block_phone = 4;
int block_address = 6;
int block_gender = 8;
int block_disable = 10;
int block_id = 12;

/* Create another array to read data from Block */
/* Legthn of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 18;
byte readBlockname[18];
String data2;

byte readBlockphone[18];
String data4;

byte readBlockaddress[18];
String data6;

byte readBlockgender[18];
String data8;

byte readBlockdisable[18];
String data10;

byte readBlockid[18];
String data12;

void clearEEPROM() {
    Serial.println("Clearing EEPROM...");
    for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0xFF);  // Writing 0xFF resets EEPROM values
    }
    EEPROM.commit();
    Serial.println("EEPROM cleared successfully!");
}

void setup() 
{
  /* Initialize serial communications with the PC */
  Serial.begin(9600);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  /* Set BUZZER as OUTPUT */
  // pinMode(BUZZER, OUTPUT);



  /* Initialize SPI bus */


  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  EEPROM.begin(EEPROM_SIZE);

  // Clear EEPROM after each upload
  clearEEPROM();

  mfrc522.PCD_Init();
  SPI.begin();
}

void loop()
{
  /* Initialize MFRC522 Module */
  
  /* Look for new cards */
  /* Reset the loop if no new card is present on RC522 Reader */
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
     return;
  }
  /* Select one of the cards */
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }

  // Get RFID UID as a string
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
      uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.print("RFID UID: ");
  Serial.println(uid);

  // Check tap limit
  if (isLimitExceeded(uid)) {
      Serial.println("❌ Tap limit exceeded! Data will NOT be sent.");
      return;
  }

  /* Read data from the same block */
  Serial.println();
  Serial.println("Reading Data...");
  ReadDataFromBlock(block_name, readBlockname);
  ReadDataFromBlock(block_phone, readBlockphone);
  ReadDataFromBlock(block_address, readBlockaddress);
  ReadDataFromBlock(block_gender, readBlockgender);
  ReadDataFromBlock(block_disable, readBlockdisable);
  ReadDataFromBlock(block_id, readBlockid);
  /* If you want to print the full memory dump, uncomment the next line */
  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  
  /* Print the data read from block */
  Serial.println();
  // Serial.print(F("Last data in RFID:"));
  // Serial.print(blockNum);
  // Serial.print(F(" --> "));
  // for (int j=0 ; j<16 ; j++)
  // {
  //   Serial.write(readBlockData[j]);
  // }
  // Serial.println();
 


  data2 = String((char*)readBlockname);
  data2.trim();
  Serial.println(data2);

  data4 = String((char*)readBlockphone);
  data2.trim();
  Serial.println(data4);
  
  data6 = String((char*)readBlockaddress);
  data2.trim();
  Serial.println(data6);

  data8 = String((char*)readBlockgender);
  data2.trim();
  Serial.println(data8);

  data10 = String((char*)readBlockdisable);
  data2.trim();
  Serial.println(data10);

  data12 = String((char*)readBlockid);
  data2.trim();
  Serial.println(data12);


  sendDataToGoogleSheets(data2, data4, data6,data8,data10, data12);

  // Update tap count
  updateTapCount(uid);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

}

void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{ 
  /* Prepare the ksy for authentication */
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  /* Authenticating the desired data block for Read access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK)
  {
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  else
  {
    Serial.println("Authentication success");
  }

  /* Reading data from the Block */
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    Serial.println("Block was read successfully");  
  }
}


void sendDataToGoogleSheets(String d2, String d4, String d6, String d8, String d10, String d12) {
  String beacon_id="Beacon_B";
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(googleScriptURL);
        http.addHeader("Content-Type", "application/json");

        // Properly formatted JSON string
        String jsonPayload = "{\"d2\": \"" + d2 + "\", \"d4\": \"" + d4 + "\", \"d6\": \"" + d6 + "\", \"d8\": \"" + d8 + "\", \"d10\": \"" + d10 + "\", \"d12\": \"" + d12 + "\",\"beacon_id\": \"" + beacon_id + "\"}";

        int httpResponseCode = http.POST(jsonPayload);
        
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Response: " + response);
        } else {
            Serial.print("Error in sending data. HTTP Response code: ");
            Serial.println(httpResponseCode);
        }
        
        http.end();
    } else {
        Serial.println("WiFi not connected!");
    }
}