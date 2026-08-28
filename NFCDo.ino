#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

// =====================================================
//                  WIFI SETTINGS
// =====================================================

const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// =====================================================
//                  PN532 I2C
// note: to setup I2C on the PN532 => switch 1 : ON
//                                    switch 2 : OFF
// =====================================================

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

WebServer server(80);

// =====================================================
//               LAST DETECTED CARD
// =====================================================

uint8_t lastUID[7];
uint8_t lastUIDLength = 0;
bool cardAvailable = false;

String lastUIDString = "No card";

// Default MIFARE Key A
uint8_t keyA[6] = {
  0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF
};


// =====================================================
//                   HTML PAGE
// =====================================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>

<meta name="viewport" content="width=device-width, initial-scale=1">

<title>ESP32 NFC Station</title>

<style>

body {
  font-family: Arial, sans-serif;
  background: #111;
  color: white;
  text-align: center;
  margin: 0;
  padding: 20px;
}

.container {
  max-width: 600px;
  margin: auto;
}

.card {
  background: #1c1c1c;
  padding: 20px;
  margin-top: 20px;
  border-radius: 15px;
}

h1 {
  color: #00d084;
}

input {
  width: 90%;
  padding: 13px;
  margin: 7px;
  border-radius: 8px;
  border: none;
  font-size: 16px;
}

button {
  padding: 14px 25px;
  margin: 8px;
  border: none;
  border-radius: 10px;
  font-size: 16px;
  cursor: pointer;
}

.scan {
  background: #007bff;
  color: white;
}

.read {
  background: #00b894;
  color: white;
}

.write {
  background: #e17055;
  color: white;
}

.status {
  font-family: monospace;
  background: #080808;
  padding: 15px;
  border-radius: 10px;
  margin-top: 15px;
  word-wrap: break-word;
}

.uid {
  font-size: 22px;
  color: #00ffae;
}

.warning {
  color: #ffcc00;
  font-size: 14px;
}

</style>

</head>


<body>

<div class="container">

<h1>ESP32 NFC Station</h1>

<div class="card">

<h2>Card Scanner</h2>

<button class="scan" onclick="scanCard()">
SCAN CARD
</button>

<p>UID</p>

<div id="uid" class="uid">
No card
</div>

<div id="cardtype">
-
</div>

</div>



<div class="card">

<h2>MIFARE Classic</h2>

<label>Block number</label>

<br>

<input
  id="block"
  type="number"
  value="4"
  min="1"
  max="63">

<br>

<label>Key A</label>

<br>

<input
  id="key"
  value="FFFFFFFFFFFF"
  maxlength="12">

<br><br>

<button class="read" onclick="readBlock()">
READ BLOCK
</button>

<br>

<input
  id="data"
  placeholder="Data to write - max 16 characters">

<br>

<button class="write" onclick="writeBlock()">
WRITE BLOCK
</button>

<p class="warning">
Avoid blocks 0, 3, 7, 11, 15... unless you know what you are doing.
</p>

<div id="result" class="status">
Ready
</div>

</div>

</div>


<script>

async function scanCard() {

  document.getElementById("result").innerHTML =
  "Place card on reader...";

  try {

    let r = await fetch("/scan");

    let j = await r.json();

    if(j.success) {

      document.getElementById("uid").innerHTML = j.uid;

      document.getElementById("cardtype").innerHTML =
      j.type;

      document.getElementById("result").innerHTML =
      "Card detected";

    } else {

      document.getElementById("result").innerHTML =
      j.message;

    }

  }

  catch(e) {

    document.getElementById("result").innerHTML =
    "Connection error";

  }
}



async function readBlock() {

  let block =
  document.getElementById("block").value;

  let key =
  document.getElementById("key").value;

  document.getElementById("result").innerHTML =
  "Reading...";

  let url =
  "/read?block=" +
  encodeURIComponent(block) +
  "&key=" +
  encodeURIComponent(key);

  try {

    let r = await fetch(url);

    let j = await r.json();

    if(j.success) {

      document.getElementById("data").value =
      j.text;

      document.getElementById("result").innerHTML =
      "HEX: " + j.hex +
      "<br>TEXT: " + j.text;

    }

    else {

      document.getElementById("result").innerHTML =
      j.message;

    }

  }

  catch(e) {

    document.getElementById("result").innerHTML =
    "Read error";

  }

}



async function writeBlock() {

  let block =
  document.getElementById("block").value;

  let key =
  document.getElementById("key").value;

  let data =
  document.getElementById("data").value;

  if(!confirm(
    "Write this data to block " +
    block +
    "?"
  )) return;


  document.getElementById("result").innerHTML =
  "Writing...";

  let url =
  "/write?block=" +
  encodeURIComponent(block) +
  "&key=" +
  encodeURIComponent(key) +
  "&data=" +
  encodeURIComponent(data);

  try {

    let r = await fetch(url);

    let j = await r.json();

    document.getElementById("result").innerHTML =
    j.message;

  }

  catch(e) {

    document.getElementById("result").innerHTML =
    "Write error";

  }

}

</script>

</body>
</html>
)rawliteral";


// =====================================================
//                   FUNCTIONS
// =====================================================

String uidToString(uint8_t *uid, uint8_t length) {

  String s = "";

  for (uint8_t i = 0; i < length; i++) {

    if (uid[i] < 0x10)
      s += "0";

    s += String(uid[i], HEX);

    if (i < length - 1)
      s += ":";

  }

  s.toUpperCase();

  return s;
}


// =====================================================
//                 HEX KEY PARSER
// =====================================================

bool parseKey(String text, uint8_t *key) {

  text.replace(" ", "");
  text.replace(":", "");

  if (text.length() != 12)
    return false;


  for (int i = 0; i < 6; i++) {

    String byteString =
      text.substring(i * 2, i * 2 + 2);

    char *endptr;

    key[i] =
      strtol(byteString.c_str(),
             &endptr,
             16);

    if (*endptr != '\0')
      return false;

  }

  return true;
}


// =====================================================
//                SCAN CARD
// =====================================================

bool detectCard(uint16_t timeout = 2000) {

  uint8_t uid[7];

  uint8_t uidLength;


  bool success =
    nfc.readPassiveTargetID(
      PN532_MIFARE_ISO14443A,
      uid,
      &uidLength,
      timeout
    );


  if (!success) {

    cardAvailable = false;

    return false;

  }


  memcpy(lastUID, uid, uidLength);

  lastUIDLength = uidLength;

  cardAvailable = true;

  lastUIDString =
    uidToString(uid, uidLength);


  Serial.print("CARD UID: ");

  Serial.println(lastUIDString);


  return true;
}


// =====================================================
//                  AUTHENTICATION
// =====================================================

bool authenticateBlock(uint8_t block,
                       uint8_t *key) {

  if (!cardAvailable)
    return false;


  return nfc.mifareclassic_AuthenticateBlock(
    lastUID,
    lastUIDLength,
    block,

    // 0 = Key A
    0,

    key
  );

}


// =====================================================
//                    WEB HOME
// =====================================================

void handleRoot() {

  server.send_P(
    200,
    "text/html",
    INDEX_HTML
  );

}


// =====================================================
//                     WEB SCAN
// =====================================================

void handleScan() {

  Serial.println("Web requested card scan");


  if (!detectCard(3000)) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"No card detected\"}"
    );

    return;

  }


  String type;


  if (lastUIDLength == 4)
    type = "Probably MIFARE Classic";

  else if (lastUIDLength == 7)
    type = "Probably MIFARE Ultralight / NTAG";

  else
    type = "ISO14443A";


  String json =

    "{"

    "\"success\":true,"

    "\"uid\":\"" +
    lastUIDString +
    "\","

    "\"type\":\"" +
    type +
    "\""

    "}";


  server.send(
    200,
    "application/json",
    json
  );

}


// =====================================================
//                      READ BLOCK
// =====================================================

void handleRead() {

  if (!server.hasArg("block")) {

    server.send(
      400,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Block missing\"}"
    );

    return;

  }


  uint8_t block =
    server.arg("block").toInt();


  if (block > 63) {

    server.send(
      400,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Invalid block\"}"
    );

    return;

  }


  uint8_t key[6];


  String keyString =

    server.hasArg("key")

    ? server.arg("key")

    : "FFFFFFFFFFFF";


  if (!parseKey(keyString, key)) {

    server.send(
      400,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Key must be 12 HEX digits\"}"
    );

    return;

  }


  // Detect card again
  if (!detectCard(2000)) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Put card on reader\"}"
    );

    return;

  }


  if (lastUIDLength != 4) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"This function currently supports MIFARE Classic only\"}"
    );

    return;

  }


  if (!authenticateBlock(block, key)) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Authentication failed - wrong Key A?\"}"
    );

    return;

  }


  uint8_t data[16];


  if (!nfc.mifareclassic_ReadDataBlock(
        block,
        data
      )) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Read failed\"}"
    );

    return;

  }


  String hexString = "";

  String textString = "";


  for (int i = 0; i < 16; i++) {

    if (data[i] < 0x10)
      hexString += "0";

    hexString += String(
      data[i],
      HEX
    );

    if (i < 15)
      hexString += " ";


    if (data[i] >= 32 &&
        data[i] <= 126) {

      textString +=
        (char)data[i];

    }

    else {

      textString += ".";

    }

  }


  hexString.toUpperCase();


  String json =

    "{"

    "\"success\":true,"

    "\"hex\":\"" +
    hexString +
    "\","

    "\"text\":\"" +
    textString +
    "\""

    "}";


  server.send(
    200,
    "application/json",
    json
  );

}


// =====================================================
//                     WRITE BLOCK
// =====================================================

void handleWrite() {

  if (!server.hasArg("block") ||
      !server.hasArg("data")) {

    server.send(
      400,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Missing parameters\"}"
    );

    return;

  }


  uint8_t block =
    server.arg("block").toInt();


  // Safety
  if (block == 0) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Block 0 write disabled for safety\"}"
    );

    return;

  }


  // Sector trailers:
  // 3,7,11,15...
  if (((block + 1) % 4) == 0) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Sector trailer write disabled for safety\"}"
    );

    return;

  }


  if (block > 63) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Invalid block\"}"
    );

    return;

  }


  uint8_t key[6];


  String keyString =

    server.hasArg("key")

    ? server.arg("key")

    : "FFFFFFFFFFFF";


  if (!parseKey(keyString, key)) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Invalid Key A\"}"
    );

    return;

  }


  String text =
    server.arg("data");


  if (text.length() > 16)
    text =
      text.substring(0, 16);


  if (!detectCard(2000)) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Put card on reader\"}"
    );

    return;

  }


  if (lastUIDLength != 4) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Write currently supports MIFARE Classic only\"}"
    );

    return;

  }


  if (!authenticateBlock(
        block,
        key
      )) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"Authentication failed\"}"
    );

    return;

  }


  uint8_t data[16];


  memset(
    data,
    0,
    sizeof(data)
  );


  for (int i = 0;
       i < text.length();
       i++) {

    data[i] =
      text[i];

  }


  bool success =
    nfc.mifareclassic_WriteDataBlock(
      block,
      data
    );


  if (!success) {

    server.send(
      200,
      "application/json",
      "{\"success\":false,"
      "\"message\":\"WRITE FAILED\"}"
    );

    return;

  }


  server.send(
    200,
    "application/json",
    "{\"success\":true,"
    "\"message\":\"WRITE SUCCESSFUL\"}"
  );

}


// =====================================================
//                       SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  Serial.println();
  Serial.println("==============================");
  Serial.println(" ESP32 NFC WEB STATION");
  Serial.println("==============================");


  // -----------------------
  // I2C
  // -----------------------

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );


  // -----------------------
  // PN532
  // -----------------------

  nfc.begin();


  uint32_t versiondata =
    nfc.getFirmwareVersion();


  if (!versiondata) {

    Serial.println(
      "PN532 NOT FOUND!"
    );

    while (1) {

      delay(1000);

    }

  }


  Serial.print("PN532 detected. Firmware: ");

  Serial.print(
    (versiondata >> 16) & 0xFF
  );

  Serial.print(".");

  Serial.println(
    (versiondata >> 8) & 0xFF
  );


  nfc.SAMConfig();


  // -----------------------
  // WIFI
  // -----------------------

  Serial.println();
  Serial.print("Connecting to WiFi");


  WiFi.mode(WIFI_STA);

  WiFi.begin(
    ssid,
    password
  );


  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(".");

  }


  Serial.println();

  Serial.println(
    "WiFi connected!"
  );


  Serial.print(
    "IP address: "
  );

  Serial.println(
    WiFi.localIP()
  );


  // -----------------------
  // WEB SERVER
  // -----------------------

  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/scan",
    handleScan
  );

  server.on(
    "/read",
    handleRead
  );

  server.on(
    "/write",
    handleWrite
  );


  server.begin();


  Serial.println();
  Serial.println(
    "Web server started!"
  );

  Serial.print(
    "Open: http://"
  );

  Serial.println(
    WiFi.localIP()
  );

}


// =====================================================
//                       LOOP
// =====================================================

void loop() {

  server.handleClient();

}
