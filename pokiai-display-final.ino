#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <Stepper.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Stepper motor setup
#define STEPS 315 // Adjust this to your stepper's specific step count
Stepper myStepper(STEPS, D2, D3, D1, D0); // Initialize stepper with pins for ESP32

// Define target positions for the stepper motor (adjust these as necessary)
const int positionA = STEPS / 3; // Example position A
const int positionB = STEPS / 3 * 2; // Example position B
const int positionC = STEPS; // Example position C

int currentPosition = 0; // Current position of the stepper

const char* ssid     = "UW MPSK";
const char* password = "ajT@>x(*34";

String previousPayload = ""; // Keep track of the previous payload for comparison
int startY = 0; // Start Y position for scrolling text
int scrollSpeed = 1; // Change this to scroll faster or slower

int countOccurrences(String str, String toFind) {
    int count = 0;
    int pos = str.indexOf(toFind);
    while (pos != -1) {
        count++;
        pos = str.indexOf(toFind, pos + 1);
    }
    return count;
}

void setup() {
  Serial.begin(115200);
  myStepper.setSpeed(100);
  
  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Check the I2C address of your display, it might be 0x3D
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);

  // Display IP address
  display.setCursor(0,0);
  display.println("WiFi connected.");
  display.println("IP address: ");
  display.println(WiFi.localIP());
  display.display(); // Show initial message
  delay(2000); // Display for 5 seconds before clearing
}

void loop() {
    static unsigned long lastTime = 0;
    unsigned long currentTime = millis();
    int position = 0;

    // Poll the server every 10 seconds
    if (currentTime - lastTime > 10000) {
        lastTime = currentTime;
        HTTPClient http;
        http.begin("http://10.19.210.123:8888/transcription");
        int httpCode = http.GET();

        if (httpCode > 0) {
            String payload = http.getString();
            Serial.println(payload);

            if (payload != previousPayload) {
                // If the payload has changed, reset scrolling
                startY = 0;
                previousPayload = payload;
                displayText(payload);
            
            // Count occurrences of "um" in the payload
            int umCount = countOccurrences(payload, "um");

            if (umCount > 0) {
                // "um" is detected, move to Position A
                moveStepperToPositionA();
            } else {
                // "um" is not detected, move to Position B
                moveStepperToPositionB();
            }
            delay(1000);
            }
        } else {
            Serial.println("Error on HTTP request");
        }




        http.end();
    } else if (previousPayload.length() > 0) {
        // Scroll the existing text if there is any
        displayText(previousPayload);
    }
}

void displayText(String text) {
    display.clearDisplay();
    display.setCursor(0, startY);
    display.println(text);
    display.display();

    // Update startY for scrolling effect, reset if it goes beyond certain limit
    startY -= scrollSpeed;
    if (startY < -SCREEN_HEIGHT) startY = 0;
    delay(100); // Small delay to control scroll speed
}

void moveStepperToPositionA() {
    int stepsToMove = 200; // Assuming 100 steps forward
    myStepper.step(stepsToMove);
    Serial.println("Moved to Position A (forward 100 steps)");
}

void moveStepperToPositionB() {
    int stepsToMove = -200; // Assuming 100 steps backward
    myStepper.step(stepsToMove);
    Serial.println("Moved to Position B (backward -100 steps)");
}
