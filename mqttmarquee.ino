#include <LEDMatrixDriver.hpp>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const int LEDMATRIX_SEGMENTS = 4;
const uint8_t LEDMATRIX_CS_PIN = D4;
char text[4096];
const char* initialText = "Ready, waiting for text via MQTT";

uint16_t scrollDelay = 25;
LEDMatrixDriver led(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN, 0);
uint16_t textIndex = 0;
uint8_t colIndex = 0;
uint16_t scrollWhitespace = 0;
uint64_t marqueeDelayTimestamp = 0;
uint64_t marqueeBlinkTimestamp;
uint16_t blinkDelay = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  for (int i = 0; i < sizeof(text); i++) {
    text[i] = initialText[i];
  }
  led.setIntensity(0);
  led.setEnabled(true);
}

void loop()
{
  if (blinkDelay) {
    if (marqueeBlinkTimestamp > millis()) {
      marqueeBlinkTimestamp > millis();
    }
    if (marqueeBlinkTimestamp + blinkDelay < millis()) {
      led.setEnabled(false);
      delay(1);
      marqueeBlinkTimestamp = millis();
    } else if (marqueeBlinkTimestamp + blinkDelay / 2 < millis()) {
      led.setEnabled(true);
      delay(1);
    }

  }
  marquee();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}







void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (!strcmp(topic, "ledMatrix/intensity")) {
    int intensity = 0;
    for (int i = 0 ; i < length;  i++) {
      intensity *= 10;
      intensity += payload[i] - '0';
    }
    intensity = max(0, min(15, intensity));
    led.setIntensity(intensity);
    return;
  }

  if (!strcmp(topic, "ledMatrix/delay")) {
    scrollDelay = 0;
    for (int i = 0 ; i < length;  i++) {
      scrollDelay *= 10;
      scrollDelay += payload[i] - '0';
    }
    if (scrollDelay < 10) {
      scrollDelay = 10;
    }
    if (scrollDelay > 1000) {
      scrollDelay = 1000;
    }

    return;
  }

  if (!strcmp(topic, "ledMatrix/blink")) {
    blinkDelay = 0;
    for (int i = 0 ; i < length;  i++) {
      blinkDelay *= 10;
      blinkDelay += payload[i] - '0';
    }
    if (blinkDelay < 0) {
      blinkDelay = 0;
    }
    if (blinkDelay > 10000) {
      blinkDelay = 10000;
    }
    if (!blinkDelay) {
      led.setEnabled(true);
    } else {
      marqueeBlinkTimestamp = millis();
    }

    return;
  }


  if (!strcmp(topic, "ledMatrix/enable")) {
    led.setEnabled(payload[0] == '1');
  }


  if (!strcmp(topic, "ledMatrix/text")) {
    text[0] = ' ';
    for (int i = 0 ; i < 4096; i++) {
      text[i] = 0;
    }
    bool isSpecial = false;
    for (int i = 0 ; i < length; i++) {
      if (payload[i] == 195) {
        isSpecial = true;
        Serial.print("SP");
        text[i] = 'a';

        continue;
      }
      if (isSpecial) {
        isSpecial = false;
        if (payload[i] == 188) {
          Serial.print("SP188");
          text[i] = 127;
        }
      } else {
        text[i] = payload[i];
      }

    }
    textIndex = 0;
    colIndex = 0;
    marqueeDelayTimestamp = 0;
    scrollWhitespace = 0;
    led.clear();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("ledMatrix/enable");
      client.subscribe("ledMatrix/intensity");
      client.subscribe("ledMatrix/delay");
      client.subscribe("ledMatrix/text");
      client.subscribe("ledMatrix/blink");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

const uint16_t LEDMATRIX_WIDTH    = LEDMATRIX_SEGMENTS * 8;

static const uint8_t font[515] PROGMEM = {
  2, 0b00000000, 0b00000000,                                /* 032 =   */
  2, 0b01101111, 0b01101111,                                /* 033 = ! */
  3, 0b00000011, 0b00000000, 0b00000011,                    /* 034 = " */
  5, 0b00010100, 0b01111111, 0b00010100, 0b01111111, 0b00010100, /* 035 = # */
  5, 0b00100100, 0b00101010, 0b01111111, 0b00101010, 0b00010010, /* 036 = $ */
  5, 0b00100011, 0b00010011, 0b00001000, 0b01100100, 0b01100010, /* 037 = % */
  5, 0b00110100, 0b01001010, 0b01001010, 0b00110100, 0b01010000, /* 038 = & */
  1, 0b00000011,                                            /* 039 = ' */
  3, 0b00011100, 0b00100010, 0b01000001,                    /* 040 = ( */
  3, 0b01000001, 0b00100010, 0b00011100,                    /* 041 = ) */
  3, 0b00000101, 0b00000010, 0b00000101,                    /* 042 = * */
  5, 0b00001000, 0b00001000, 0b00111110, 0b00001000, 0b00001000, /* 043 = + */
  2, 0b11100000, 0b01100000,                                /* 044 = , */
  5, 0b00001000, 0b00001000, 0b00001000, 0b00001000, 0b00001000, /* 045 = - */
  2, 0b01100000, 0b01100000,                                /* 046 = . */
  5, 0b01000000, 0b00110000, 0b00001000, 0b00000110, 0b00000001, /* 047 = / */
  5, 0b00111110, 0b01010001, 0b01001001, 0b01000101, 0b00111110, /* 048 = 0 */
  3, 0b01000010, 0b01111111, 0b01000000,                    /* 049 = 1 */
  5, 0b01000010, 0b01100001, 0b01010001, 0b01001001, 0b01000110, /* 050 = 2 */
  5, 0b00100001, 0b01000001, 0b01000101, 0b01001011, 0b00110001, /* 051 = 3 */
  5, 0b00011000, 0b00010100, 0b00010010, 0b01111111, 0b00010000, /* 052 = 4 */
  5, 0b00100111, 0b01000101, 0b01000101, 0b01000101, 0b00111001, /* 053 = 5 */
  5, 0b00111100, 0b01001010, 0b01001001, 0b01001001, 0b00110000, /* 054 = 6 */
  5, 0b00000001, 0b00000001, 0b01111001, 0b00000101, 0b00000011, /* 055 = 7 */
  5, 0b00110110, 0b01001001, 0b01001001, 0b01001001, 0b00110110, /* 056 = 8 */
  5, 0b00000110, 0b01001001, 0b01001001, 0b00101001, 0b00011110, /* 057 = 9 */
  2, 0b00110110, 0b00110110,                                /* 058 = : */
  2, 0b01110110, 0b00110110,                                /* 059 = ; */
  4, 0b00001000, 0b00010100, 0b00100010, 0b01000001,        /* 060 = < */
  5, 0b00010100, 0b00010100, 0b00010100, 0b00010100, 0b00010100, /* 061 = = */
  4, 0b01000001, 0b00100010, 0b00010100, 0b00001000,        /* 062 = > */
  5, 0b00000010, 0b00000001, 0b01010001, 0b00001001, 0b00000110, /* 063 = ? */
  5, 0b00111110, 0b01000001, 0b01011101, 0b01010101, 0b01011110, /* 064 = @ */
  5, 0b01111110, 0b00001001, 0b00001001, 0b00001001, 0b01111110, /* 065 = A */
  5, 0b01111111, 0b01001001, 0b01001001, 0b01001001, 0b00110110, /* 066 = B */
  5, 0b00111110, 0b01000001, 0b01000001, 0b01000001, 0b00100010, /* 067 = C */
  5, 0b01111111, 0b01000001, 0b01000001, 0b01000001, 0b00111110, /* 068 = D */
  5, 0b01111111, 0b01001001, 0b01001001, 0b01001001, 0b01000001, /* 069 = E */
  5, 0b01111111, 0b00001001, 0b00001001, 0b00001001, 0b00000001, /* 070 = F */
  5, 0b00111110, 0b01000001, 0b01001001, 0b01001001, 0b01111010, /* 071 = G */
  5, 0b01111111, 0b00001000, 0b00001000, 0b00001000, 0b01111111, /* 072 = H */
  5, 0b01000001, 0b01000001, 0b01111111, 0b01000001, 0b01000001, /* 073 = I */
  5, 0b00100000, 0b01000001, 0b01000001, 0b00111111, 0b00000001, /* 074 = J */
  5, 0b01111111, 0b00001000, 0b00010100, 0b00100010, 0b01000001, /* 075 = K */
  5, 0b01111111, 0b01000000, 0b01000000, 0b01000000, 0b01000000, /* 076 = L */
  5, 0b01111111, 0b00000010, 0b00000100, 0b00000010, 0b01111111, /* 077 = M */
  5, 0b01111111, 0b00000110, 0b00001000, 0b00110000, 0b01111111, /* 078 = N */
  5, 0b00111110, 0b01000001, 0b01000001, 0b01000001, 0b00111110, /* 079 = O */
  5, 0b01111111, 0b00010001, 0b00010001, 0b00010001, 0b00001110, /* 080 = P */
  5, 0b00111110, 0b01000001, 0b01010001, 0b00100001, 0b01011110, /* 081 = Q */
  5, 0b01111111, 0b00001001, 0b00011001, 0b00101001, 0b01000110, /* 082 = R */
  5, 0b00100110, 0b01001001, 0b01001001, 0b01001001, 0b00110010, /* 083 = S */
  5, 0b00000001, 0b00000001, 0b01111111, 0b00000001, 0b00000001, /* 084 = T */
  5, 0b00111111, 0b01000000, 0b01000000, 0b01000000, 0b00111111, /* 085 = U */
  5, 0b00011111, 0b00100000, 0b01000000, 0b00100000, 0b00011111, /* 086 = V */
  5, 0b00111111, 0b01000000, 0b00110000, 0b01000000, 0b00111111, /* 087 = W */
  5, 0b01100011, 0b00010100, 0b00001000, 0b00010100, 0b01100011, /* 088 = X */
  5, 0b00000111, 0b00001000, 0b01110000, 0b00001000, 0b00000111, /* 089 = Y */
  5, 0b01100001, 0b01010001, 0b01001001, 0b01000101, 0b01000011, /* 090 = Z */
  3, 0b01111111, 0b01000001, 0b01000001,                    /* 091 = [ */
  5, 0b00000001, 0b00000110, 0b00001000, 0b00110000, 0b01000000, /* 092 = \ */
  3, 0b01000001, 0b01000001, 0b01111111,                    /* 093 = ] */
  5, 0b00000100, 0b00000010, 0b00000001, 0b00000010, 0b00000100, /* 094 = ^ */
  5, 0b01000000, 0b01000000, 0b01000000, 0b01000000, 0b01000000, /* 095 = _ */
  1, 0b00000011,                                            /* 096 = ' */
  5, 0b00100000, 0b01010100, 0b01010100, 0b01010100, 0b01111000, /* 097 = a */
  5, 0b01111111, 0b00101000, 0b01000100, 0b01000100, 0b00111000, /* 098 = b */
  5, 0b00111000, 0b01000100, 0b01000100, 0b01000100, 0b00101000, /* 099 = c */
  5, 0b00111000, 0b01000100, 0b01000100, 0b00101000, 0b01111111, /* 100 = d */
  5, 0b00111000, 0b01010100, 0b01010100, 0b01010100, 0b00011000, /* 101 = e */
  5, 0b00000100, 0b01111110, 0b00000101, 0b00000001, 0b00000010, /* 102 = f */
  5, 0b00011000, 0b10100100, 0b10100100, 0b10100100, 0b01111100, /* 103 = g */
  5, 0b01111111, 0b00000100, 0b00000100, 0b00000100, 0b01111000, /* 104 = h */
  3, 0b01000100, 0b01111101, 0b01000000,                    /* 105 = i */
  4, 0b01000000, 0b10000000, 0b10000100, 0b01111101,        /* 106 = j */
  5, 0b01111111, 0b00010000, 0b00010000, 0b00101000, 0b01000100, /* 107 = k */
  3, 0b01000001, 0b01111111, 0b01000000,                    /* 108 = l */
  5, 0b01111100, 0b00000100, 0b01111100, 0b00000100, 0b01111000, /* 109 = m */
  5, 0b01111100, 0b00001000, 0b00000100, 0b00000100, 0b01111000, /* 110 = n */
  5, 0b00111000, 0b01000100, 0b01000100, 0b01000100, 0b00111000, /* 111 = o */
  5, 0b11111100, 0b00100100, 0b00100100, 0b00100100, 0b00011000, /* 112 = p */
  5, 0b00011000, 0b00100100, 0b00100100, 0b00100100, 0b11111100, /* 113 = q */
  5, 0b01111100, 0b00001000, 0b00000100, 0b00000100, 0b00001000, /* 114 = r */
  5, 0b01001000, 0b01010100, 0b01010100, 0b01010100, 0b00100000, /* 115 = s */
  5, 0b00000100, 0b00111110, 0b01000100, 0b01000000, 0b00100000, /* 116 = t */
  5, 0b00111100, 0b01000000, 0b01000000, 0b00100000, 0b01111100, /* 117 = u */
  5, 0b00011100, 0b00100000, 0b01000000, 0b00100000, 0b00011100, /* 118 = v */
  5, 0b00111100, 0b01000000, 0b00110000, 0b01000000, 0b00111100, /* 119 = w */
  5, 0b01000100, 0b00101000, 0b00010000, 0b00101000, 0b01000100, /* 120 = x */
  5, 0b00000100, 0b01001000, 0b00110000, 0b00001000, 0b00000100, /* 121 = y */
  5, 0b01000100, 0b01100100, 0b01010100, 0b01001100, 0b01000100, /* 122 = z */
  3, 0b00001000, 0b00110110, 0b01000001,                    /* 123 = { */
  1, 0b01111111,                                            /* 124 = | */
  3, 0b01000001, 0b00110110, 0b00001000,                    /* 125 = } */
  5, 0b00011000, 0b00000100, 0b00001000, 0b00010000, 0b00001100 /* 126 = ~ */



};

static const uint16_t font_index[95] PROGMEM = {
  0, 3, 6, 10, 16, 22, 28, 34, 36, 40, 44, 48, 54, 57, 63, 66, 72, 78, 82, 88, 94,
  100, 106, 112, 118, 124, 130, 133, 136, 141, 147, 152, 158, 164, 170, 176,
  182, 188, 194, 200, 206, 212, 218, 224, 230, 236, 242, 248, 254, 260, 266,
  272, 278, 284, 290, 296, 302, 308, 314, 320, 324, 330, 334, 340, 346, 348,
  354, 360, 366, 372, 378, 384, 390, 396, 400, 405, 411, 415, 421, 427, 433,
  439, 445, 451, 457, 463, 469, 475, 481, 487, 493, 499, 503, 505, 509
};



void nextChar()
{
  if (text[++textIndex] == '\0')
  {
    textIndex = 0;
    scrollWhitespace = LEDMATRIX_WIDTH;
  }
}

void nextCol(uint8_t w)
{
  if (++colIndex == w)
  {
    scrollWhitespace = 2;
    colIndex = 0;
    nextChar();
  }
}

void writeCol()
{
  if (scrollWhitespace > 0)
  {
    scrollWhitespace--;
    return;
  }

  uint8_t asc = text[textIndex] - 32;
  uint16_t idx = pgm_read_word(&(font_index[asc]));
  uint8_t w = pgm_read_byte(&(font[idx]));
  uint8_t col = pgm_read_byte(&(font[colIndex + idx + 1]));
  led.setColumn(LEDMATRIX_WIDTH - 1, col);
  nextCol(w);
}

void marquee()
{
  if (millis() < 1)
    marqueeDelayTimestamp = 0;
  if (millis() < marqueeDelayTimestamp)
    return;
  marqueeDelayTimestamp = millis() + scrollDelay;
  led.scroll(LEDMatrixDriver::scrollDirection::scrollLeft);
  writeCol();
  led.display();
}
