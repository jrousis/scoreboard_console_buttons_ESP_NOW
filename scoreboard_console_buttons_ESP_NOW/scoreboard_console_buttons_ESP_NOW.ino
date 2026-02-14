

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
// Setup for ESP-NOW
#include <esp_now.h>
#include <WiFi.h>
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
//--------------------------------------------------------------
//MAC Address: 38:18:2B:3E:0C:DC
uint8_t broadcastAddress1[] = { 0x38, 0x18, 0x2B, 0x3E, 0x0C, 0xDC }; // Broadcast address to send data to
//38:18:2B:3C:AA:8C
uint8_t broadcastAddress2[] = { 0x38, 0x18, 0x2B, 0x3C, 0xAA, 0x8C }; // Broadcast address to send data to

esp_err_t result;

typedef struct shot_packet {
	char time[9];
	uint8_t alarm;
	uint8_t id;
	uint8_t buzzer_time;
	uint8_t brightness;
} shot_packet;

typedef struct struct_packet {
	uint16_t insrtuction;
	uint8_t id;
} struct_packet;

shot_packet packet_to_slave;
struct_packet display_packet;
struct_packet display_received;
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
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
	Serial.print("WiFi Packet Lengh: ");
	Serial.println(len);
	Serial.print("Device ID: ");
	Serial.println(display_received.id);
	Serial.print("Instruction WiFi received: ");
	Serial.println(display_received.insrtuction, HEX);

}

//--------------------------------------------------------------


void setup() {
	Serial.begin(115200);
	Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
	delay(100);
	while (!Serial);
	
	Serial.println();
	// print the file name and the date of compilation
	Serial.println(__FILE__); 
	Serial.println("Compiled at: " __DATE__);	
	Serial.println("Started Initializing...");
	// Broadcast Address to send data to
	Serial.print("Broadcast MAC Address to: "); 
	for (int i = 0; i < 6; i++) {
		Serial.printf("%02X", broadcastAddress1[i]);
		if (i < 5) Serial.print(":");
	}
	Serial.println();
	Serial.print("And to: ");
	for (int i = 0; i < 6; i++) {
		Serial.printf("%02X", broadcastAddress2[i]);
		if (i < 5) Serial.print(":");
	}
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

	Serial.println("Starting Testing");
	Serial.println("-------------------");
#if defined(CONFIG_IDF_TARGET_ESP32S3)
	pixel.setPixelColor(0, pixel.Color(20, 20, 20)); // Blue color
	pixel.show();
#endif
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
	if (Serial1.available() >= 2) {
		// open LED 
		digitalWrite(LED_PIN, LOW);
		uint8_t byte1 = Serial1.read();
		uint8_t byte2 = Serial1.read();
		uint16_t instruction = (byte1 << 8) | byte2;
		if (instruction >= INSTR_0 && instruction <= INSTR_8) {
			Serial.print("Serial1 data shootclock received: ");
			Serial.println(instruction, HEX);
			Send_WiFiNow_to_ShotClock(instruction);
		}
		// close LED
		digitalWrite(LED_PIN, HIGH);
	}
}

void Send_WiFiNow_to_ShotClock(uint16_t instr)
{
	if ((instr & 0xFF00) == 0xA000 && (instr & 0x00FF) <100) {
		byte seconds = instr & 0x00FF;
		char time_ascii[2];
		time_ascii[0] = (seconds / 10) + '0';
		time_ascii[1] = (seconds % 10) + '0';
		packet_to_slave.time[0] = time_ascii[0]; 
		packet_to_slave.time[1] = time_ascii[1];
		esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t*)&packet_to_slave, sizeof(packet_to_slave));
		if (result != ESP_OK) {
			Serial.println("WiFiNOW Error sending the data to broadcastAddress1");
		}
		result = esp_now_send(broadcastAddress2, (uint8_t*)&packet_to_slave, sizeof(packet_to_slave));	
		if (result != ESP_OK) {
			Serial.println("WiFiNOW Error sending the data to broadcastAddress2");
		}
	}
}

void send_wifinow_inst(uint16_t instr, uint8_t ID)
{
	display_packet.id = ID;
	display_packet.insrtuction = instr;
	esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t*)&display_packet, sizeof(display_packet));
	if (result == ESP_OK) {
		//Serial.println("WiFiNOW Sent with success");
	}
	else {
		Serial.println("WiFiNOW Error sending the data");
	}
}
