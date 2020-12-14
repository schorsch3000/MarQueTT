#include <LEDMatrixDriver.hpp>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "local_config.h"

uint8_t text[MAX_TEXT_LENGTH];



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
  client.setBufferSize(MAX_TEXT_LENGTH);
  for (int i = 0; i < sizeof(text); i++) {
    text[i] = initialText[i];
  }
  led.setIntensity(0);
  led.setEnabled(true);
  calculate_font_index();
  client.publish("ledMatrix/status", "startup");
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
    client.publish("ledMatrix/status", "reconnect");
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

void printHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
  char tmp[16];
  for (int i = 0; i < length; i++) {
    sprintf(tmp, "0x%.2X", data[i]);
    Serial.print(tmp); Serial.print(" ");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print((String)"MQTT in: " + topic + "\t = ");
  for (int i = 0; i < length; i++)  Serial.print((char)(payload[i]));
  Serial.println();

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
    const bool pr = 0;                                // set to 1 for debug prints
    if (pr) printHex8(payload, length);
    text[0] = ' ';
    for (int i = 0 ; i < 4096; i++) {
      text[i] = 0;
    }
    char tmp[16];
    int i = 0, j = 0;
    while (i < length) {
      uint8_t b = payload[i++];
      sprintf(tmp, "0x%.2X = '%c' -> ", b, b);
      if (pr) Serial.print(tmp);
      if ((b & 0b10000000) == 0) {                    // 7-bit ASCII
        if (pr) Serial.println("ASCII");
        text[j++] = b;

      } else if ((b & 0b11100000) == 0b11000000) {    // UTF-8 2-byte sequence: starts with 0b110xxxxx
        if (pr) Serial.println("UTF-8 (2)");
        if (b == 0xc3) {                              // UTF-8 1st byte
          text[j++] = payload[i++] + 64;              // map 2nd byte to Latin-1 (simplified)
        } else if (b == 0xc2) {
          if (i < length && payload[i] == 0xA7) {         // § = section sign
            text[j++] = 0xA7;
          } else if (i < length && payload[i] == 0xB0) {  // ° = degrees sign
            text[j++] = 0xB0;
          } else if (i < length && payload[i] == 0xB5) {  // µ = mu
            text[j++] = 0xB5;
          } else {
            // unknown
            text[j++] = 0x7f;                         // add checkerboard pattern
          }
          i += 1;
        } else {
          // unknown
          text[j++] = 0x7f;                           // add checkerboard pattern
          i += 1;
        }
      } else if ((b & 0b111100000) == 0b11100000) {   // UTF-8 3-byte sequence: starts with 0b1110xxxx
        if (pr) Serial.println("UTF-8 (3)");
        if (i + 1 < length && b == 0xE2) {
          if (payload[i] == 0x82 && payload[i + 1] == 0xAC) {
            text[j++] = 0x80;                           // € = euro sign
          } else if (payload[i] == 0x80 && payload[i + 1] == 0x93) {
            text[j++] = 0x96;                           // – = en dash
          } else if (payload[i] == 0x80 && payload[i + 1] == 0x94) {
            text[j++] = 0x97;                           // — = em dash
          } else if (payload[i] == 0x80 && payload[i + 1] == 0xA6) {
            text[j++] = 0x85;                           // … = ellipsis
          } else {
            // unknown
            text[j++] = 0x7f;                         // add checkerboard pattern
          }
          i += 2;
        } else {
          // unknown, skip remaining 2 bytes
          i += 2;
          text[j++] = 0x7f;                           // add checkerboard pattern
          text[j++] = 0x7f;                           // add checkerboard pattern
        }
      } else if ((b & 0b111110000) == 0b11110000) {   // UTF-8 4-byte sequence_ starts with 0b11110xxx
        if (pr) Serial.println("UTF-8 (4)");
        // unknown, skip remaining 3 bytes
        i += 3;
        text[j++] = 0x7f;                             // add checkerboard pattern
        text[j++] = 0x7f;                             // add checkerboard pattern
        text[j++] = 0x7f;                             // add checkerboard pattern
      } else {                                        // unsupported (illegal) UTF-8 sequence
        if (pr) Serial.print("UTF-8 (n) ");
        while ((payload[i] & 0b10000000) && i < length) {
          if (pr) Serial.print("+");
          i++;                                        // skip non-ASCII characters
          text[j++] = 0x7f;                           // add checkerboard pattern
        }
        if (pr) Serial.println();
      }
    }
    if (pr) Serial.print((String)"=> Text (" + j + " bytes): ");
    if (pr) printHex8(text, j + 1);

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
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password, "ledMatrix/status", 1, true, "Offline")) {
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

static const uint8_t font[750] /*PROGMEM*/ = {
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
  3, 0b01111111, 0b01000001, 0b01000001,                         /* 091 = [ */
  5, 0b00000001, 0b00000110, 0b00001000, 0b00110000, 0b01000000, /* 092 = \ */
  3, 0b01000001, 0b01000001, 0b01111111,                         /* 093 = ] */
  5, 0b00000100, 0b00000010, 0b00000001, 0b00000010, 0b00000100, /* 094 = ^ */
  5, 0b01000000, 0b01000000, 0b01000000, 0b01000000, 0b01000000, /* 095 = _ */
  1, 0b00000011,                                                 /* 096 = ' */
  5, 0b00100000, 0b01010100, 0b01010100, 0b01010100, 0b01111000, /* 097 = a */
  5, 0b01111111, 0b00101000, 0b01000100, 0b01000100, 0b00111000, /* 098 = b */
  5, 0b00111000, 0b01000100, 0b01000100, 0b01000100, 0b00101000, /* 099 = c */
  5, 0b00111000, 0b01000100, 0b01000100, 0b00101000, 0b01111111, /* 100 = d */
  5, 0b00111000, 0b01010100, 0b01010100, 0b01010100, 0b00011000, /* 101 = e */
  5, 0b00000100, 0b01111110, 0b00000101, 0b00000001, 0b00000010, /* 102 = f */
  5, 0b00011000, 0b10100100, 0b10100100, 0b10100100, 0b01111100, /* 103 = g */
  5, 0b01111111, 0b00000100, 0b00000100, 0b00000100, 0b01111000, /* 104 = h */
  3, 0b01000100, 0b01111101, 0b01000000,                         /* 105 = i */
  4, 0b01000000, 0b10000000, 0b10000100, 0b01111101,             /* 106 = j */
  5, 0b01111111, 0b00010000, 0b00010000, 0b00101000, 0b01000100, /* 107 = k */
  3, 0b01000001, 0b01111111, 0b01000000,                         /* 108 = l */
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
  3, 0b00001000, 0b00110110, 0b01000001,                         /* 123 = { */
  1, 0b01111111,                                                 /* 124 = | */
  3, 0b01000001, 0b00110110, 0b00001000,                         /* 125 = } */
  5, 0b00011000, 0b00000100, 0b00001000, 0b00010000, 0b00001100, /* 126 = ~ */
  // extension: German Umlauts
  5, 0b01010101, 0b10101010, 0b01010101, 0b10101010, 0b01010101, /* 127 = checkerboard pattern */
  5, 0b00011100, 0b00110110, 0b01010101, 0b01010101, 0b01000001, /* 128 =  */
  0,                                                             /* 129 =  */
  0,                                                             /* 130 =  */
  0,                                                             /* 131 =  */
  0,                                                             /* 132 =  */
  5, 0b01000000, 0b00000000, 0b01000000, 0b00000000, 0b01000000, /* 133 =
*/
  0,                                                             /* 134 =  */
  0,                                                             /* 135 =  */
  0,                                                             /* 136 =  */
  0,                                                             /* 137 =  */
  0,                                                             /* 138 =  */
  0,                                                             /* 139 =  */
  0,                                                             /* 140 =  */
  0,                                                             /* 141 =  */
  0,                                                             /* 142 =  */
  0,                                                             /* 143 =  */
  0,                                                             /* 144 =  */
  0,                                                             /* 145 =  */
  0,                                                             /* 146 =  */
  0,                                                             /* 147 =  */
  0,                                                             /* 148 =  */
  0,                                                             /* 149 =  */
  4, 0b00001000, 0b00001000, 0b00001000, 0b00001000,             /* 150 =  */
  5, 0b00001000, 0b00001000, 0b00001000, 0b00001000, 0b00001000, /* 151 =  */
  0,                                                             /* 152 =  */
  0,                                                             /* 153 =  */
  0,                                                             /* 154 =  */
  0,                                                             /* 155 =  */
  0,                                                             /* 156 =  */
  0,                                                             /* 157 =  */
  0,                                                             /* 158 =  */
  0,                                                             /* 159 =  */
  0,                                                             /* 160 =   */
  0,                                                             /* 161 = ¡ */
  0,                                                             /* 162 = ¢ */
  0,                                                             /* 163 = £ */
  0,                                                             /* 164 = ¤ */
  0,                                                             /* 165 = ¥ */
  0,                                                             /* 166 = ¦ */
  5, 0b00001010, 0b01010101, 0b01010101, 0b01010101, 0b00101000, /* 167 = § */
  0,                                                             /* 168 = ¨ */
  0,                                                             /* 169 = © */
  0,                                                             /* 170 = ª */
  0,                                                             /* 171 = « */
  0,                                                             /* 172 = ¬ */
  0,                                                             /* 173 = ­ */
  0,                                                             /* 174 = ® */
  0,                                                             /* 175 = ¯ */
  4, 0b00000110, 0b00001001, 0b00001001, 0b00000110,             /* 176 = ° */
  0,                                                             /* 177 = ± */
  0,                                                             /* 178 = ² */
  0,                                                             /* 179 = ³ */
  0,                                                             /* 180 = ´ */
  5, 0b11111100, 0b00100000, 0b01000000, 0b00100000, 0b01111100, /* 181 = µ */
  0,                                                             /* 182 = ¶ */
  0,                                                             /* 183 = · */
  0,                                                             /* 184 = ¸ */
  0,                                                             /* 185 = ¹ */
  0,                                                             /* 186 = º */
  0,                                                             /* 187 = » */
  0,                                                             /* 188 = ¼ */
  0,                                                             /* 189 = ½ */
  0,                                                             /* 190 = ¾ */
  0,                                                             /* 191 = ¿ */
  0,                                                             /* 192 = À */
  0,                                                             /* 193 = Á */
  0,                                                             /* 194 = Â */
  0,                                                             /* 195 = Ã */
  5, 0b11111000, 0b00010101, 0b00010100, 0b00010101, 0b11111000, /* 196 = Ä */
  0,                                                             /* 197 = Å */
  0,                                                             /* 198 = Æ */
  0,                                                             /* 199 = Ç */
  0,                                                             /* 200 = È */
  0,                                                             /* 201 = É */
  0,                                                             /* 202 = Ê */
  0,                                                             /* 203 = Ë */
  0,                                                             /* 204 = Ì */
  0,                                                             /* 205 = Í */
  0,                                                             /* 206 = Î */
  0,                                                             /* 207 = Ï */
  0,                                                             /* 208 = Ð */
  0,                                                             /* 209 = Ñ */
  0,                                                             /* 210 = Ò */
  0,                                                             /* 211 = Ó */
  0,                                                             /* 212 = Ô */
  0,                                                             /* 213 = Õ */
  5, 0b01111000, 0b10000101, 0b10000100, 0b10000101, 0b01111000, /* 214 = Ö */
  0,                                                             /* 215 = × */
  0,                                                             /* 216 = Ø */
  0,                                                             /* 217 = Ù */
  0,                                                             /* 218 = Ú */
  0,                                                             /* 219 = Û */
  5, 0b01111100, 0b10000001, 0b10000000, 0b10000001, 0b11111100, /* 220 = Ü */
  0,                                                             /* 221 = Ý */
  0,                                                             /* 222 = Þ */
  5, 0b11111110, 0b00000001, 0b01000101, 0b01001010, 0b00110000, /* 223 = ß */
  0,                                                             /* 224 = à */
  0,                                                             /* 225 = á */
  0,                                                             /* 226 = â */
  0,                                                             /* 227 = ã */
  5, 0b00100000, 0b01010101, 0b01010100, 0b01010101, 0b01111000, /* 228 = ä */
  0,                                                             /* 229 = å */
  0,                                                             /* 230 = æ */
  0,                                                             /* 231 = ç */
  0,                                                             /* 232 = è */
  0,                                                             /* 233 = é */
  0,                                                             /* 234 = ê */
  0,                                                             /* 235 = ë */
  0,                                                             /* 236 = ì */
  0,                                                             /* 237 = í */
  0,                                                             /* 238 = î */
  0,                                                             /* 239 = ï */
  0,                                                             /* 240 = ð */
  0,                                                             /* 241 = ñ */
  0,                                                             /* 242 = ò */
  0,                                                             /* 243 = ó */
  0,                                                             /* 244 = ô */
  0,                                                             /* 245 = õ */
  5, 0b00111000, 0b01000101, 0b01000100, 0b01000101, 0b00111000, /* 246 = ö */
  0,                                                             /* 247 = ÷ */
  0,                                                             /* 248 = ø */
  0,                                                             /* 249 = ù */
  0,                                                             /* 250 = ú */
  0,                                                             /* 251 = û */
  5, 0b00111100, 0b01000001, 0b01000000, 0b00100001, 0b01111100, /* 252 = ü */
  0,                                                             /* 253 = ý */
  0,                                                             /* 254 = þ */
  0,                                                             /* 255 = ÿ*/
};

uint16_t* font_index; // will be calculated at start by calculate_font_index()
/**static const uint16_t font_index[95] PROGMEM = {
  0, 3, 6, 10, 16, 22, 28, 34, 36, 40, 44, 48, 54, 57, 63, 66, 72, 78, 82, 88, 94,
  100, 106, 112, 118, 124, 130, 133, 136, 141, 147, 152, 158, 164, 170, 176,
  182, 188, 194, 200, 206, 212, 218, 224, 230, 236, 242, 248, 254, 260, 266,
  272, 278, 284, 290, 296, 302, 308, 314, 320, 324, 330, 334, 340, 346, 348,
  354, 360, 366, 372, 378, 384, 390, 396, 400, 405, 411, 415, 421, 427, 433,
  439, 445, 451, 457, 463, 469, 475, 481, 487, 493, 499, 503, 505, 509
  };**/

void calculate_font_index()
{
  uint16 fontsize = sizeof(font);
  Serial.println((String)"font uses " + fontsize + " bytes.");
  uint8_t* fontptr = (uint8_t*)font;
  int num_chars = 0;
  // 1st sweep: count chars
  while (fontptr < font + fontsize) {
    int char_width = *fontptr;
    fontptr += char_width + 1;  // add character X size
    num_chars++;
  }
  // allocate index table
  uint16_t* index = (uint16_t*)malloc(num_chars * sizeof(uint16_t));
  // 2nd sweep: calculate index values
  fontptr = (uint8_t*)font;
  for (int i = 0; i < num_chars; i++) {
    index[i] = fontptr - font;
    int char_width = *fontptr;
    //Serial.println((String)"idx " + i + " = " + (char)(i+32) + ": " + char_width + " -> " + index[i]);
    fontptr += char_width + 1;  // add character X size
  }
  font_index = index;
}



void nextChar()
{
  if (text[++textIndex] == '\0')
  {
    textIndex = 0;
    scrollWhitespace = LEDMATRIX_WIDTH;
    client.publish("ledMatrix/status", "repeat");
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
