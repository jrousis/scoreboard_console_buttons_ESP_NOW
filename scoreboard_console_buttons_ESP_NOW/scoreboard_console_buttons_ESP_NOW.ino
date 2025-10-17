#include <Adafruit_NeoPixel.h>
#define LED_PIN 10
#define btn1 14
#define btn2 13
#define btn3 12
#define btn4 11
#define btn5 3
#define btn6 8
#define btn7 18
#define btn8 17

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
// Define the GPIO pin for the onboard NeoPixel.
// This is commonly GPIO 48 for ESP32-S3 boards.
#define RGB_PIN 48 
// The number of NeoPixels. Many boards have only one.
#define NUM_PIXELS 1 
// Create the NeoPixel object.
Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);
//--------------------------------------------------------------
//MAC Address: 80:64:6F:EE:70:B0
uint8_t broadcastAddress1[] = { 0x80, 0x64, 0x6F, 0xEE, 0x70, 0xB0 }; //MAC Address of the receiver (scoreboard) - change this to your receiver MAC address
esp_err_t result;

typedef struct struct_packet {
	uint16_t insrtuction;
	uint8_t id;
} struct_packet;

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
		pixel.setPixelColor(0, pixel.Color(0, 255, 0));
		pixel.show();
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
	while (!Serial);
	delay(100);
	Serial.println();
	Serial.println("Started Initializing...");
	// Initialize the NeoPixel library.
	pixel.begin();
	// Set a moderate brightness level (0-255).
	pixel.setBrightness(100);

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
	// Register peer
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

	Serial.println("Starting Testing");
	Serial.println("-------------------");

	pixel.setPixelColor(0, pixel.Color(20, 20, 20)); // Blue color
	pixel.show();

}

void loop() {
	// check the button1 is LOW
	if (digitalRead(btn1) == LOW) {
		send_wifinow_inst(INSTR_4, device_addr);
		unsigned long currentMillis = millis();
		lastDebounceTime = currentMillis;  // Update the debounce time
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
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
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
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
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
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
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
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
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
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
		pixel.setPixelColor(0, pixel.Color(255, 0, 0)); // Red color
		pixel.show();
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
		pixel.setPixelColor(0, pixel.Color(255, 255, 0)); // Yellow color
		pixel.show();
		Serial.println("Boot Button Pressed");
		if (currentMillis - lastDebounceTime > debounceDelay) {
			lastDebounceTime = currentMillis;  // Update the debounce time
		}
		while (digitalRead(GPIO_NUM_0) == LOW) {}
		delay(debounceDelay);
	}

	if (millis() - previousMillis > 400) {
		digitalWrite(LED_PIN, HIGH);
		pixel.setPixelColor(0, pixel.Color(20, 20, 20));
		pixel.show();
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
