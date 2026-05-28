#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <uri/UriBraces.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

WebServer server(80);

const int LED1 = 26;
const int LED2 = 27;

bool led1State = false;
bool led2State = false;

const int MAX_MESSAGES = 8;

String messages[MAX_MESSAGES];
int messageCount = 0;

String lastMessage = "Esperando...";

void updateOLED() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("ESP32 CHAT");

  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

  display.setCursor(0, 18);
  display.println("Ultimo mensaje:");

  display.setCursor(0, 35);
  display.println(lastMessage);

  display.display();
}

void addMessage(String msg) {

  msg.trim();

  if (msg == "") return;

  lastMessage = msg;

  updateOLED();

  if (messageCount >= MAX_MESSAGES) {

    for (int i = 0; i < MAX_MESSAGES - 1; i++) {
      messages[i] = messages[i + 1];
    }

    messages[MAX_MESSAGES - 1] = msg;

  } else {

    messages[messageCount++] = msg;
  }

  Serial.print("Mensaje: ");
  Serial.println(msg);
}

void sendHtml() {

  String chatHtml = "";

  for (int i = 0; i < messageCount; i++) {

    chatHtml += "<div class='message'>";
    chatHtml += messages[i];
    chatHtml += "</div>";
  }

  String response = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Chat</title>
<meta name="viewport"
content="width=device-width, initial-scale=1">

<style>

body{
font-family:Arial;
background:#f2f2f2;
padding:20px;
display:flex;
justify-content:center;
}

.container{
width:100%;
max-width:600px;
background:white;
padding:20px;
border-radius:14px;
box-shadow:0 2px 12px rgba(0,0,0,.2);
}

.chat-box{
height:250px;
overflow-y:auto;
border:1px solid #ccc;
padding:10px;
border-radius:10px;
margin-bottom:10px;
background:#fafafa;
}

.message{
background:#dff0ff;
padding:10px;
margin-bottom:8px;
border-radius:8px;
}

.send-area{
display:flex;
gap:10px;
}

input{
flex:1;
padding:14px;
font-size:16px;
}

button{
padding:14px;
font-size:16px;
}

.btn{
display:block;
text-align:center;
padding:15px;
border-radius:10px;
text-decoration:none;
color:white;
background:#2ecc71;
margin-bottom:10px;
}

.OFF{
background:#555;
}

</style>
</head>

<body>

<div class="container">

<h1>ESP32 Chat</h1>

<a href="/toggle/1"
class="btn LED1_CLASS">
LED1 LED1_TEXT
</a>

<a href="/toggle/2"
class="btn LED2_CLASS">
LED2 LED2_TEXT
</a>

<h2>Mensajes</h2>

<div class="chat-box">
CHAT_MESSAGES
</div>

<form action="/send" method="GET">

<div class="send-area">

<input
type="text"
name="msg"
placeholder="Escribe un mensaje">

<button type="submit">
Enviar
</button>

</div>

</form>

</div>

</body>
</html>
)rawliteral";

  response.replace(
    "CHAT_MESSAGES",
    chatHtml
  );

  response.replace(
    "LED1_TEXT",
    led1State ? "ON" : "OFF"
  );

  response.replace(
    "LED2_TEXT",
    led2State ? "ON" : "OFF"
  );

  response.replace(
    "LED1_CLASS",
    led1State ? "" : "OFF"
  );

  response.replace(
    "LED2_CLASS",
    led2State ? "" : "OFF"
  );

  server.send(
    200,
    "text/html",
    response
  );
}

void setup() {

  Serial.begin(115200);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  Wire.begin(21, 22);

  display.begin(
    SSD1306_SWITCHCAPVCC,
    0x3C
  );

  updateOLED();

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD,
    WIFI_CHANNEL
  );

  Serial.print("Conectando");

  while (WiFi.status()
         != WL_CONNECTED) {

    delay(100);
    Serial.print(".");
  }

  Serial.println("\nWiFi OK");
  Serial.println(
    WiFi.localIP()
  );

  server.on("/", sendHtml);

  server.on(
    UriBraces("/toggle/{}"),
    []() {

      int led =
      server.pathArg(0)
      .toInt();

      switch (led) {

        case 1:
          led1State =
          !led1State;

          digitalWrite(
            LED1,
            led1State
          );
          break;

        case 2:
          led2State =
          !led2State;

          digitalWrite(
            LED2,
            led2State
          );
          break;
      }

      sendHtml();
    });

  server.on("/send", []() {

    if (
      server.hasArg("msg")
    ) {

      addMessage(
        server.arg("msg")
      );
    }

    sendHtml();
  });

  server.begin();

  Serial.println(
    "Servidor iniciado"
  );
}

void loop() {
  server.handleClient();
  delay(2);
}