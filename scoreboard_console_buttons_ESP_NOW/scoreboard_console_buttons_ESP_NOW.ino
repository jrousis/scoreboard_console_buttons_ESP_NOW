
#define SHOT_CLOCK_GLOBAL_BROADCAST true

#define INSTR_0 0xA000
#define INSTR_1 0xA151
#define INSTR_2 0xA152
#define INSTR_3 0xA153
#define INSTR_4 0xA154
#define INSTR_5 0xA155
#define INSTR_6 0xA156
#define INSTR_7 0xA157
#define INSTR_8 0xA158
//--------------------------------------------------------------
// Timestamp to debounce the button
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 75;  // Debounce delay in milliseconds
unsigned long previousMillis = 0;
volatile bool buttonPressed = false;  // Volatile variable for interrupt
uint8_t device_addr = 1;
bool OnDataSent_enabled = true;
// Setup for ESP-NOW
#include <esp_now.h>
#include "mbedtls/md.h"
#include <WiFi.h>
#include <EEPROM.h>

#define EEPROM_SIZE 128
#define EEPROM_DEFAULT 0 // 1 byte
#define EEPROM_PARAMS_START 7 // start address for params
//--------------------------------------------------------------
// check if the chip is ESP32-S3
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include <Adafruit_NeoPixel.h>
// Define the GPIO pin for the onboard NeoPixel.
// This is commonly GPIO 48 for ESP32-S3 boards.
#define RGB_PIN 48 

#define LED_PIN 10
#define btn1 14
#define btn2 13
#define btn3 12
#define btn4 11
#define btn5 3
#define btn6 8
#define btn7 18
#define btn8 17

#define RXD1 22 //19
#define TXD1 5  //18

// The number of NeoPixels. Many boards have only one.
#define NUM_PIXELS 1 
// Create the NeoPixel object.
Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);
#else
#define LED_PIN 13

#define btn1 33
#define btn2 25
#define btn3 26
#define btn4 27
#define btn5 14
#define btn6 12
#define btn7 18
#define btn8 19

#define RXD1 22 //19
#define TXD1 5  //18

#endif
//---------------------------------------------------------------------
// make params object to store the parameters
struct Params {
	uint8_t Mac_shoot_1[6];
	uint8_t Mac_shoot_2[6];
	uint8_t Mac_team_1[6];
	uint8_t Mac_team_2[6];
};
Params params;

void saveParamsToEEPROM() {
	EEPROM.put(EEPROM_PARAMS_START, params);
	EEPROM.commit();
}

void loadParamsFromEEPROM() {
	EEPROM.get(EEPROM_PARAMS_START, params);
}

//--------------------------------------------------------------
// Define the secret key for HMAC (must be 16 bytes for ESP-NOW encryption)
static const uint8_t SECRET_KEY[] = { 
	0x21, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
	0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE
};


uint8_t globalBroadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // Local broadcast address

//MAC Address: 38:18:2B:3C:D6:20
uint8_t broadcastAddress1[] = { 0x38, 0x18, 0x2B, 0x3C, 0xD6, 0x20 }; // Broadcast address to send data
//38:18:2B:3E:0F:48
uint8_t broadcastAddress2[] = { 0x38, 0x18, 0x2B, 0x3E, 0x0F, 0x48 }; // Broadcast address to send data

//38:18:2B:3E:41:BC
uint8_t broadcast_team1[] = { 0x38, 0x18, 0x2B, 0x3E, 0x41, 0xBC }; // Broadcast address to send data
//38:18:2B:3E:24:D8	
uint8_t broadcast_team2[] = { 0x38, 0x18, 0x2B, 0x3E, 0x24, 0xD8 }; // Broadcast address to send data

// MAC for console 10:20:BA:75:45:B0
uint8_t broadcast_console[] = { 0x10, 0x20, 0xBA, 0x75, 0x45, 0xB0 }; // Broadcast address to send data

bool console_online = false;
uint32_t g_seq = 0;
uint32_t g_sessionId = 0;
esp_err_t result;

typedef struct shot_packet {
	char time[9];
	uint8_t time_run = 0;
	uint8_t shot_run = 0;
	uint8_t alarm = 0;
	uint8_t brightness = 0;
	uint32_t sessionId = 0;
	uint32_t seq = 0;
	uint8_t hmac[8] = {0};
} shot_packet;

typedef struct struct_display {
	uint16_t insrtuction;
	uint8_t id;
} struct_display;

typedef struct struct_packet {
	char insrtuction[8];
	char device[8];  // CONSOLE
	uint8_t len;
	uint8_t data[32];
} struct_packet;

shot_packet packet_to_slave;
struct_display display_packet;
struct_packet display_received;
esp_now_peer_info_t peerInfo;

char ascii_buf[8] = {0};

void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
	if (!OnDataSent_enabled){ 
		OnDataSent_enabled = true;
		return; 
	} // Skip if the callback is disabled

	Serial.print("\r\nLast Packet Send Status:\t");
	//Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
	Serial.println(status == 0 ? "Delivery Success" : "Delivery Fail");
	if (status == 0) {
		//open LED to green
		digitalWrite(LED_PIN, LOW);
#if defined(CONFIG_IDF_TARGET_ESP32S3)
		pixel.setPixelColor(0, pixel.Color(0, 255, 0));
		pixel.show();
#endif
		previousMillis = millis();
	}
	else {
		//display_response = 2;
	}
}

// Callback when data is received
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
	memcpy(&display_received, incomingData, sizeof(display_received));
	Serial.println();
	Serial.print("Device: ");
	for (int i = 0; i < 7; i++) {
		Serial.print(display_received.device[i]);
	}
	Serial.print(" - Instruction: ");
	for (int i = 0; i < 8; i++) {
		Serial.print(display_received.insrtuction[i]);
	}
	Serial.print(" - Serial Received data: ");
	for (int i = 0; i < display_received.len; i++) {
		Serial.print(display_received.data[i], HEX);
		Serial.print(" ");
	}
	Serial.println();

	// Check if the instruction is "TEAM_A" or "TEAM_B"
	if (strncmp(display_received.insrtuction, "TEAM_A", 6) == 0) {
		Serial.println("Received instruction for TEAM A");
		// Handle TEAM A instruction
		result = esp_now_send(broadcast_team1, (uint8_t*)&display_received, sizeof(display_received));	
		if (result == ESP_OK) {
			Serial.println("WiFiNOW Sent to TEAM A with success");
		}
		else {
			Serial.println("Error sending to TEAM A");
		}
	}
	else if (strncmp(display_received.insrtuction, "TEAM_B", 6) == 0) {
		Serial.println("Received instruction for TEAM B");
		// Handle TEAM B instruction
		result = esp_now_send(broadcast_team2, (uint8_t*)&display_received, sizeof(display_received));
		if (result == ESP_OK) {
			Serial.println("WiFiNOW Sent to TEAM B with success");
		}
		else {
			Serial.println("Error sending to TEAM B");
		}
	}
	else {
		// send to Serial1 the display_received.insrtuction
		Serial1.println(display_received.insrtuction);
	}

	if (strncmp(display_received.device, "CONSOLE", 6) == 0 && !console_online) {
		// detected console connection
		// copy the received MAC of the sender to broadcast_console
		
		console_online = addPeer(mac);
	}

}

//function to add a new peer by MAC
bool addPeer(const uint8_t* mac) {
	memcpy(peerInfo.peer_addr, mac, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;
	Serial.println();
	if (esp_now_add_peer(&peerInfo) != ESP_OK) {		
		Serial.println("Failed to add the new peer");
		return false;
	}
	else {

		Serial.println("New Peer added successfully");
		Serial.print("Peer MAC: "); 
		for (int i = 0; i < 6; i++) {
			broadcast_console[i] = mac[i];
			Serial.printf("%02X", peerInfo.peer_addr[i]);
			if (i < 5) Serial.print(":");
		}
		Serial.println();
		return true;
	}
}

//----------------------------------------------
bool computeHmac8(const uint8_t* data, size_t len, uint8_t out8[8])
{
	uint8_t hmac_full[32];
	mbedtls_md_context_t ctx;
	mbedtls_md_init(&ctx);
	if (mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1) != 0) {
		mbedtls_md_free(&ctx);
		return false;
	}
	if (mbedtls_md_hmac_starts(&ctx, SECRET_KEY, sizeof(SECRET_KEY)) != 0 ||
		mbedtls_md_hmac_update(&ctx, data, len) != 0 ||
		mbedtls_md_hmac_finish(&ctx, hmac_full) != 0) {
		mbedtls_md_free(&ctx);
		return false;
	}
	memcpy(out8, hmac_full, 8); // Use the first 8 bytes of the HMAC
	mbedtls_md_free(&ctx);
	return true;
}

bool signPacket()
{
	return computeHmac8((const uint8_t*)&packet_to_slave, offsetof(shot_packet, hmac), packet_to_slave.hmac);
}

// make sequence add
void sequence_add() {
	if (g_seq == UINT32_MAX) {
		// Rotate session to avoid seq reuse within same session
		g_sessionId = esp_random();
		g_seq = 0;
	}

	packet_to_slave.seq = g_seq++;
	packet_to_slave.sessionId = g_sessionId;
}

//--------------------------------------------------------------

void setup() {
	Serial.begin(115200);
	Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
	EEPROM.begin(EEPROM_SIZE);
	while (!Serial);
	delay(10);
	
	loadParamsFromEEPROM();
	memcpy(broadcastAddress1, params.Mac_shoot_1, 6);
	memcpy(broadcastAddress2, params.Mac_shoot_2, 6);
	memcpy(broadcast_team1, params.Mac_team_1, 6);
	memcpy(broadcast_team2, params.Mac_team_2, 6);

	Serial.println();
	// print the file name and the date of compilation
	Serial.println(__FILE__); 
	Serial.println("Compiled at: " __DATE__);	
	Serial.println("Started Initializing...");

	Serial.println();
	// Broadcast Address to send data to
	Serial.println("Broadcast MAC Shot Clock to: "); 
	Serial.print("MAC 1 = ");
	for (int i = 0; i < 6; i++) {
		Serial.printf("%02X", broadcastAddress1[i]);
		if (i < 5) Serial.print(":");
	}
	Serial.println();
	Serial.print("MAC 2 = ");
	for (int i = 0; i < 6; i++) {
		Serial.printf("%02X", broadcastAddress2[i]);
		if (i < 5) Serial.print(":");
	}
	Serial.println();

	// Broadcast Address to send data to
	Serial.println("Broadcast MAC Team modules to: ");
	Serial.print("MAC 1 = ");
	for (int i = 0; i < 6; i++) {
		Serial.printf("%02X", broadcast_team1[i]);
		if (i < 5) Serial.print(":");
	}
	Serial.println();
	Serial.print("MAC 2 = ");
	for (int i = 0; i < 6; i++) {
		Serial.printf("%02X", broadcast_team2[i]);
		if (i < 5) Serial.print(":");
	}
	Serial.println(); Serial.println();


	// Initialize the NeoPixel library.
#if defined(CONFIG_IDF_TARGET_ESP32S3)
	pixel.begin();
	// Set a moderate brightness level (0-255).
	pixel.setBrightness(100);
#endif
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);

	pinMode(btn1, INPUT_PULLUP);
	pinMode(btn2, INPUT_PULLUP);
	pinMode(btn3, INPUT_PULLUP);
	pinMode(btn4, INPUT_PULLUP);
	pinMode(btn5, INPUT_PULLUP);
	pinMode(btn6, INPUT_PULLUP);
	pinMode(btn7, INPUT_PULLUP);
	pinMode(btn8, INPUT_PULLUP);
	// and the Boot button
	pinMode(GPIO_NUM_0, INPUT_PULLUP);

	// Set device as a Wi-Fi Station
	WiFi.mode(WIFI_STA);
	Serial.print("Machine MAC Address: ");
	Serial.println(WiFi.macAddress());
	// Init ESP-NOW
	if (esp_now_init() != ESP_OK) {
		Serial.println("Error initializing ESP-NOW");
		return;
	}
	else {
		Serial.println("initialized ESP - NOW");
	}

	esp_now_register_send_cb(OnDataSent);
	// Register for a callback function that will be called when data is received
	esp_now_register_recv_cb(OnDataRecv);
	// Register peer 1
	memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;

	// Add peer
	if (esp_now_add_peer(&peerInfo) != ESP_OK) {
		Serial.println("Failed to add peer 1");
	}
	else {
		Serial.println("Peer 1 added");
	}

	// Register peer 2
	memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;
	// Add peer
	if (esp_now_add_peer(&peerInfo) != ESP_OK) {
		Serial.println("Failed to add peer 2");
	}
	else {
		Serial.println("Peer 2 added");
	}

	// Register peer team 1
	memcpy(peerInfo.peer_addr, broadcast_team1, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;
	// Add peer
	if (esp_now_add_peer(&peerInfo) != ESP_OK) {
		Serial.println("Failed to add peer team 1");
	}
	else {
		Serial.println("Peer team 1 added");
	}

	// Register peer team 2
	memcpy(peerInfo.peer_addr, broadcast_team2, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;
	// Add peer
	if (esp_now_add_peer(&peerInfo) != ESP_OK) {
		Serial.println("Failed to add peer team 2");
	}
	else {
		Serial.println("Peer team 2 added");
	}

	//Register peer for broadcast_console
	/*memcpy(peerInfo.peer_addr, broadcast_console, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;
	if (esp_now_add_peer(&peerInfo) != ESP_OK) {
		Serial.println("Failed to add peer console");
	}
	else {
		Serial.println("Peer console added");
	}*/

	Serial.println("Starting Testing");
	Serial.println("-------------------");
#if defined(CONFIG_IDF_TARGET_ESP32S3)
	pixel.setPixelColor(0, pixel.Color(20, 20, 20)); // Blue color
	pixel.show();
#endif

	g_sessionId = esp_random();  // Initialize session ID
}

void loop() {
	// check the button1 is LOW
	if (digitalRead(btn1) == LOW) {
		send_wifinow_inst(INSTR_4, device_addr);
		unsigned long currentMillis = millis();
		lastDebounceTime = currentMillis;  // Update the debounce time
#if defined(CONFIG_IDF_TARGET_ESP32S3)
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
#endif
		Serial.println("Button 1 Pressed");
		if (currentMillis - lastDebounceTime > debounceDelay) {
			lastDebounceTime = currentMillis;  // Update the debounce time
		}
		while (digitalRead(btn1) == LOW) {}
		delay(debounceDelay);
	}
	else if (digitalRead(btn2) == LOW) {
		send_wifinow_inst(INSTR_3, device_addr);
		unsigned long currentMillis = millis();
		lastDebounceTime = currentMillis;  // Update the debounce time
#if defined(CONFIG_IDF_TARGET_ESP32S3)
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
#endif
		Serial.println("Button 2 Pressed");
		if (currentMillis - lastDebounceTime > debounceDelay) {
			lastDebounceTime = currentMillis;  // Update the debounce time
		}
		while (digitalRead(btn2) == LOW) {}
		delay(debounceDelay);
	}
	else if (digitalRead(btn3) == LOW) {
		send_wifinow_inst(INSTR_2, device_addr);
		unsigned long currentMillis = millis();
		lastDebounceTime = currentMillis;  // Update the debounce time
#if defined(CONFIG_IDF_TARGET_ESP32S3)
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
#endif
		Serial.println("Button 3 Pressed");
		if (currentMillis - lastDebounceTime > debounceDelay) {
			lastDebounceTime = currentMillis;  // Update the debounce time
		}
		while (digitalRead(btn3) == LOW) {}
		delay(debounceDelay);
	}
	else if (digitalRead(btn4) == LOW) {
		send_wifinow_inst(INSTR_1, device_addr);
		unsigned long currentMillis = millis();
		lastDebounceTime = currentMillis;  // Update the debounce time
#if defined(CONFIG_IDF_TARGET_ESP32S3)
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
#endif
		Serial.println("Button 4 Pressed");
		if (currentMillis - lastDebounceTime > debounceDelay) {
			lastDebounceTime = currentMillis;  // Update the debounce time
		}
		while (digitalRead(btn4) == LOW) {}
		delay(debounceDelay);
	}
	else if (digitalRead(btn5) == LOW) {
		send_wifinow_inst(INSTR_5, device_addr);
		unsigned long currentMillis = millis();
		lastDebounceTime = currentMillis;  // Update the debounce time
#if defined(CONFIG_IDF_TARGET_ESP32S3)
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
#endif
		Serial.println("Button 5 Pressed");
		if (currentMillis - lastDebounceTime > debounceDelay) {
			lastDebounceTime = currentMillis;  // Update the debounce time
		}
		while (digitalRead(btn5) == LOW) {}
		delay(debounceDelay);
	}
	else if (digitalRead(btn6) == LOW) {
		send_wifinow_inst(INSTR_6, device_addr);
		unsigned long currentMillis = millis();
		lastDebounceTime = currentMillis;  // Update the debounce time
#if defined(CONFIG_IDF_TARGET_ESP32S3)
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
#endif
		Serial.println("Button 6 Pressed");
		if (currentMillis - lastDebounceTime > debounceDelay) {
			lastDebounceTime = currentMillis;  // Update the debounce time
		}
		while (digitalRead(btn6) == LOW) {}
		delay(debounceDelay);
	}
	else if (digitalRead(GPIO_NUM_0) == LOW) {
		send_wifinow_inst(INSTR_1, device_addr);
		unsigned long currentMillis = millis();
		lastDebounceTime = currentMillis;  // Update the debounce time
#if defined(CONFIG_IDF_TARGET_ESP32S3)
		pixel.setPixelColor(0, pixel.Color(255, 255, 0)); // Yellow color
		pixel.show();
#endif
		Serial.println("Boot Button Pressed");
		if (currentMillis - lastDebounceTime > debounceDelay) {
			lastDebounceTime = currentMillis;  // Update the debounce time
		}
		while (digitalRead(GPIO_NUM_0) == LOW) {}
		delay(debounceDelay);
	}

	if (millis() - previousMillis > 400) {
		digitalWrite(LED_PIN, HIGH);
#if defined(CONFIG_IDF_TARGET_ESP32S3)
		pixel.setPixelColor(0, pixel.Color(20, 20, 20));
		pixel.show();
#endif

		previousMillis = millis();		
	}

	// Check for incoming serial1 data
	// expecting 2 bytes looking for instructions from 0xA151 to 0xA158
	if (Serial1.available() > 0) {
		String input;
		bool gotLine = readSerial1LineWithTimeout(input, 200);
		Serial.println();

		if (gotLine) {
			if (gotLine) {
				input.trim();
				if (input[0] == 0xA0) {
					uint8_t head_byte = input[0];
					ascii_buf[0] = input[1];
					ascii_buf[1] = input[2];
					
					Serial.print("Instruction 0xA0 Received: ");
					for (int i = 0; i < 2; i++) {
						Serial.print(ascii_buf[i], HEX);
						Serial.print(" ");
					}
					Serial.println();
					Send_WiFiNow_to_ShotClock(head_byte);		  // send WiFi now to shot clock the instruction of 0xA0

					// send WiFi now to console the data of shot clock
					struct_packet packet_to_send;
					strcpy(packet_to_send.device, "CONSOLE");
					strcpy(packet_to_send.insrtuction, "SHOTCLK");
					
					packet_to_send.len = input.length() - 1;
					for (int i = 0; i < packet_to_send.len; i++) {

						packet_to_send.data[i] = input[i + 1];
						Serial.println(packet_to_send.data[i], HEX);
					}
					Serial.println();
					result = esp_now_send(broadcast_console, (uint8_t*)&packet_to_send, sizeof(packet_to_send));  // send to console the data of shot clock
					if (result == ESP_OK) {
						Serial.println("WiFiNOW Sent to Console with success");
					}
					else {
						Serial.println("Error sending to Console");
					}

				}
				else if (input[0] == 0xC0) {
					Serial.print("Instruction 0xC0 Received data: ");
					Serial.println(input.substring(1));
					Serial.println("In HEX: ");
					// send WiFi now to console the data
					struct_packet packet_to_send;
					strcpy(packet_to_send.device, "CONSOLE");
					strcpy(packet_to_send.insrtuction, "DISPLAY");
					packet_to_send.len = input.length() - 1;
					for (int i = 0; i < packet_to_send.len; i++) {

						packet_to_send.data[i] = input[i + 1];
						Serial.print(packet_to_send.data[i], HEX); Serial.print(" ");
					}
					Serial.println();
					result = esp_now_send(broadcast_console, (uint8_t*)&packet_to_send, sizeof(packet_to_send));
					if (result == ESP_OK) {
						Serial.println("WiFiNOW Sent to Console with success");
					}
					else {
						Serial.println("Error sending to Console");
					}
				}
				else if (input[0] == 0xE0) {
					Serial.print("Instruction 0xE0 Received data: ");
					Serial.println(input.substring(1));
					Serial.println("In HEX: ");
					for (int i = 1; i < input.length(); i++) {
						Serial.print(input[i], HEX); Serial.print(" ");
					}
					Serial.println();

					// send WiFi now to console the data
					struct_packet packet_to_send;
					strcpy(packet_to_send.device, "CONSOLE");
					if (input[1] == 'A') {
						strcpy(packet_to_send.insrtuction, "TEAM_A");
						Serial.println("Team A instruction");
					}
					else if (input[1] == 'B') {
						strcpy(packet_to_send.insrtuction, "TEAM_B");
						Serial.println("Team B instruction");
					}
					packet_to_send.len = input.length() - 1;
					for (int i = 0; i < packet_to_send.len - 1; i++) {
						packet_to_send.data[i] = input[i + 2];
						Serial.print(packet_to_send.data[i], HEX); Serial.print(" ");
					}
					Serial.println();
					if (input[1] == 'A') {
						result = esp_now_send(broadcast_team1, (uint8_t*)&packet_to_send, sizeof(packet_to_send));
					}
					 else if (input[1] == 'B') {
						result = esp_now_send(broadcast_team2, (uint8_t*)&packet_to_send, sizeof(packet_to_send));
					}
					 else {
						Serial.println("Unknown team instruction");
						return;
					}
					if (result == ESP_OK) {
						Serial.println("WiFiNOW Sent to Console with success");
					}
					else {
						Serial.println("Error sending to Console");
					}
				}
				else if (input[0] == 0xAB) {
					uint8_t brightness_value = input[1];
					Serial.print("Instruction 0xAB Received for brightness: ");
					Serial.println(brightness_value);
					packet_to_slave.brightness = brightness_value;
				}
			}
		}
		//if (head_byte == 0xA0) {
		//	//Print the ascii_buf
		//	Serial.println();
		//	Serial.print("Serial Instruction 0xA0 Received data: ");
		//	for (int i = 0; i < 2; i++) {
		//		Serial.print(ascii_buf[i], HEX);
		//		Serial.print(" ");
		//	}
		//	Serial.println();

		//	Send_WiFiNow_to_ShotClock(head_byte);
		//}

		// close LED
		digitalWrite(LED_PIN, HIGH);
	}

	// check if receive data on serial the string "MACShot1:", "MACShot2:", "MACTeam1:" and "MACTeam2:" after that save the MAC address to EEPROM
	if (Serial.available()) {
		String input = Serial.readStringUntil('\n');
		if (input.startsWith("MACShot1:")) {
			String macStr = input.substring(9);
			macStr.trim();
			if (macStr.length() == 17) {
				int values[6];
				sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
				for (int i = 0; i < 6; i++) {
					params.Mac_shoot_1[i] = (uint8_t)values[i];
				}
				saveParamsToEEPROM();
				Serial.print("New MAC Address for Shot Clock 1 set: ");
				for (int i = 0; i < 6; i++) {
					Serial.printf("%02X", params.Mac_shoot_1[i]);
					if (i < 5) Serial.print(":");
				}
				Serial.println();
			}
		}
		else if (input.startsWith("MACShot2:")) {
			String macStr = input.substring(9);
			macStr.trim();
			if (macStr.length() == 17) {
				int values[6];
				sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
				for (int i = 0; i < 6; i++) {
					params.Mac_shoot_2[i] = (uint8_t)values[i];
				}
				saveParamsToEEPROM();
				Serial.print("New MAC Address for Shot Clock 2 set: ");
				for (int i = 0; i < 6; i++) {
					Serial.printf("%02X", params.Mac_shoot_2[i]);
					if (i < 5) Serial.print(":");
				}
				Serial.println();
			}
		}
		else if (input.startsWith("MACTeam1:")) {
			String macStr = input.substring(9);
			macStr.trim();
			if (macStr.length() == 17) {
				int values[6];
				sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
				for (int i = 0; i < 6; i++) {
					params.Mac_team_1[i] = (uint8_t)values[i];
				}
				saveParamsToEEPROM();
				Serial.print("New MAC Address for Team 1 set: ");
				for (int i = 0; i < 6; i++) {
					Serial.printf("%02X", params.Mac_team_1[i]);
					if (i < 5) Serial.print(":");
				}
				Serial.println();
			}
		}
		else if (input.startsWith("MACTeam2:")) {
			String macStr = input.substring(9);
			macStr.trim();
			if (macStr.length() == 17) {
				int values[6];
				sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]);
				for (int i = 0; i < 6; i++) {
					params.Mac_team_2[i] = (uint8_t)values[i];
				}
				saveParamsToEEPROM();
				Serial.print("New MAC Address for Team 2 set: ");
				for (int i = 0; i < 6; i++) {
					Serial.printf("%02X", params.Mac_team_2[i]);
					if (i < 5) Serial.print(":");
				}
				Serial.println();
			}
		}
	}	
}

// Plan (pseudocode):
// 1. Add a helper that reads from Serial1 until '\n' or timeout.
// 2. Accumulate bytes while available and track elapsed time.
// 3. Return true if a full line was received, otherwise false.
// 4. Replace readStringUntil call with the helper and handle partial data.

bool readSerial1LineWithTimeout(String& output, unsigned long timeoutMs) {
	output = "";
	unsigned long start = millis();
	while (millis() - start < timeoutMs) {
		while (Serial1.available() > 0) {
			char c = static_cast<char>(Serial1.read());
			if (c == '\r') {
				continue;
			}
			if (c == '\n') {
				return true;
			}
			output += c;
		}
		delay(1);
	}
	return false;
}

void Send_WiFiNow_to_ShotClock(uint8_t instr)
{
	if (instr == 0xA0) {
		sequence_add();
		packet_to_slave.time[0] = ascii_buf[0];
		packet_to_slave.time[1] = ascii_buf[1];
				if (!signPacket()) {
			Serial.println("Failed to sign packet");
		}
#if defined(SHOT_CLOCK_GLOBAL_BROADCAST)
		OnDataSent_enabled = false; // disable the OnDataSent callback to avoid multiple calls for each sent packet
		result = esp_now_send(globalBroadcastAddress, (uint8_t*)&packet_to_slave, sizeof(packet_to_slave));	  // send to all the devices in range
		Serial.println("WiFiNOW Sent to all shot clocks in range with success");
	}
		
#else		   		
		result = esp_now_send(broadcastAddress1, (uint8_t*)&packet_to_slave, sizeof(packet_to_slave));
		if (result != ESP_OK) {
			Serial.println("WiFiNOW Error sending the data to broadcastAddress1");
		}
		result = esp_now_send(broadcastAddress2, (uint8_t*)&packet_to_slave, sizeof(packet_to_slave));	
		if (result != ESP_OK) {
			Serial.println("WiFiNOW Error sending the data to broadcastAddress2");
		}
	}

#endif

}

void send_wifinow_inst(uint16_t instr, uint8_t ID)
{
	display_packet.id = ID;
	display_packet.insrtuction = instr;
	result = esp_now_send(broadcastAddress1, (uint8_t*)&display_packet, sizeof(display_packet));
	if (result == ESP_OK) {
		//Serial.println("WiFiNOW Sent with success");
	}
	else {
		Serial.println("WiFiNOW Error sending the data");
	}
}
