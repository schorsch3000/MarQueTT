const char* ssid = "";                                                      // Name of the SSID to connect to
const char* password = "";                                                  // Password used to connect to your wifi

const char* mqtt_server = "";                                               // Hostname or IP of mqtt-Server
const char* mqtt_username = "";                                             // Username used to connect to MQTT-Server, leave empty if unused
const char* mqtt_password = "";                                             // Password used to connect to MQTT-Server, leave empty if unused

const int LEDMATRIX_SEGMENTS = 4;                                           // Number of 8x8 Segments
const uint8_t LEDMATRIX_CS_PIN = D4;                                        // CableSelect pin
#define MAX_TEXT_LENGTH 4096                                                // Maximal text length, to large might use up to much ram
const char* initialText = "Ready, waiting for text via MQTT";               // Initial Text shown before the first MQTT-Message is recieved, don't leave empty, to shwo no text on startup set to " " 
uint16_t scrollDelay = 25;                                                  // Initial Scroll deplay
