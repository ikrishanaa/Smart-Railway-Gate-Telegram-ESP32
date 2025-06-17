#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ESP32Servo.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 

// WiFi credentials
const char* ssid = "Replace with your WiFi SSID"; // Replace with your WiFi SSID
const char* password = "Replace with your WiFi Password"; // Replace with your WiFi Password

// Telegram BOT Token and Chat ID
#define BOTtoken "Replace with your BOT token"      // Replace with your BOT token
#define CHAT_ID "Replace with your CHAT ID"        // Replace with your CHAT ID

// Telegram settings
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
unsigned long lastTimeBotRan;

// GPIO Pin Setup
#define SERVO1_PIN 13
#define SERVO2_PIN 14
#define BUZZER_PIN 12
#define RED_LED 26
#define GREEN_LED 27

// --- Ultrasonic Sensor Pins ---
#define ULTRASONIC_TRIG_PIN_1 25 // GPIO pin for Ultrasonic Sensor 1 Trigger
#define ULTRASONIC_ECHO_PIN_1 33 // GPIO pin for Ultrasonic Sensor 1 Echo
#define ULTRASONIC_TRIG_PIN_2 32 // GPIO pin for Ultrasonic Sensor 2 Trigger
#define ULTRASONIC_ECHO_PIN_2 35 // GPIO pin for Ultrasonic Sensor 2 Echo (Input only pin is fine)
#define OBSTACLE_THRESHOLD 23  // Obstacle detection threshold in cm (e.g., 30cm)

// --- Servo Movement Speed Configuration ---
#define SERVO_MOVEMENT_ADDITIONAL_DELAY_MS 500 // Additional delay spread over servo steps (ms)

Servo servo1;
Servo servo2;

bool gateOpen = false;

// --- LCD Configuration ---
// Common I2C addresses: 0x27, 0x3F. Find yours with an I2C scanner if unsure.
LiquidCrystal_I2C lcd(0x27, 16, 2); // For a 16x2 I2C LCD (address, cols, rows)

// LCD State & Scrolling Text
String baseScrollTextLine1 = "SMART RAILWAY BOT";
String baseScrollTextLine2 = "YOU CAN WRITE YOUR TEXT HARE";
String scrollTextLine1_Padded; // Padded version for scrolling logic
String scrollTextLine2_Padded; // Padded version for scrolling logic
int scrollOffset1 = 0;
int scrollOffset2 = 0;
unsigned long lastLcdScrollMillis = 0;
const unsigned long LCD_SCROLL_INTERVAL = 350; // Milliseconds between scroll steps (adjust for speed)

bool lcdCustomMessageActive = false;
unsigned long lcdCustomMessageStartTime = 0;
const unsigned long LCD_CUSTOM_MESSAGE_DURATION = 4000; // Show custom messages for 4 seconds

void setupLcdScrollText() {
    // Pad the base text with spaces equal to the display width for a smooth scrolling effect
    // The text will appear to scroll off one side and re-enter from the other.
    String padding = "";
    for(int i=0; i<16; i++) padding += " "; // 16 spaces for a 16-char display

    scrollTextLine1_Padded = baseScrollTextLine1 + padding;
    scrollTextLine2_Padded = baseScrollTextLine2 + padding;
}

void lcd_display_temporary(String line1, String line2 = "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1.substring(0, 16)); // Truncate if longer than 16
    if (line2.length() > 0) {
        lcd.setCursor(0, 1);
        lcd.print(line2.substring(0, 16)); // Truncate if longer than 16
    }
    lcdCustomMessageActive = true;
    lcdCustomMessageStartTime = millis();
    // Reset scroll timer to prevent immediate overwrite by scrolling text
    lastLcdScrollMillis = millis(); 
}

void lcd_task() {
    if (lcdCustomMessageActive) {
        if (millis() - lcdCustomMessageStartTime >= LCD_CUSTOM_MESSAGE_DURATION) {
            lcdCustomMessageActive = false;
            lcd.clear(); // Clear the custom message once its duration is over
            // Force an immediate scroll update on the next iteration
            lastLcdScrollMillis = millis() - LCD_SCROLL_INTERVAL - 1; 
        } else {
            return; // Custom message is still active, so don't scroll
        }
    }

    // If no custom message is active, or a custom message just expired, perform scrolling
    if (millis() - lastLcdScrollMillis >= LCD_SCROLL_INTERVAL) {
        lastLcdScrollMillis = millis();

        // Line 1 scrolling
        lcd.setCursor(0, 0);
        String view1 = "";
        for (int i = 0; i < 16; i++) { // Construct the 16-character view for the display
            view1 += scrollTextLine1_Padded.charAt((scrollOffset1 + i) % scrollTextLine1_Padded.length());
        }
        lcd.print(view1);

        // Line 2 scrolling
        lcd.setCursor(0, 1);
        String view2 = "";
        for (int i = 0; i < 16; i++) {
            view2 += scrollTextLine2_Padded.charAt((scrollOffset2 + i) % scrollTextLine2_Padded.length());
        }
        lcd.print(view2);

        scrollOffset1++;
        if (scrollOffset1 >= scrollTextLine1_Padded.length()) {
            scrollOffset1 = 0;
        }
        // You can have different scroll speeds by adjusting how scrollOffset2 increments
        scrollOffset2++; 
        if (scrollOffset2 >= scrollTextLine2_Padded.length()) {
            scrollOffset2 = 0;
        }
    }
}


// ---- Buzzer beeps faster as gate nears close/open (with reduced delays for speed) ----
void blinkBuzzer(int currentPos, int startPos, int endPos) {
  int total = abs(endPos - startPos);
  int progress = abs(currentPos - startPos);
  int beepDelay = (total > 0) ? map(progress, 0, total, 100, 20) : 20; 
  digitalWrite(BUZZER_PIN, HIGH);
  delay(20); 
  digitalWrite(BUZZER_PIN, LOW);
  delay(beepDelay);
}

// ---- Final long beep ----
void longBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(1600); 
  digitalWrite(BUZZER_PIN, LOW);
}

// ---- Function to move both servos together smoothly and faster ----
void smoothMoveServosTogether(Servo &s1, Servo &s2, int startPos1, int endPos1, int startPos2, int endPos2, int totalAdditionalDelayMs) {
  int stepsToTake1 = abs(endPos1 - startPos1); 
  int stepsToTake2 = abs(endPos2 - startPos2);
  int stepsToTake = max(stepsToTake1, stepsToTake2); // Use the max steps if ranges are different

  if (stepsToTake == 0) { 
    s1.write(endPos1);
    s2.write(endPos2);
    longBeep(); 
    return;
  }

  float additionalDelayPerStep = (stepsToTake > 0) ? (float)totalAdditionalDelayMs / stepsToTake : 0;
  
  int direction1 = (endPos1 > startPos1) ? 1 : -1;
  int direction2 = (endPos2 > startPos2) ? 1 : -1; 

  float currentS1Pos_f = startPos1; // Use float for precise intermediate steps
  float currentS2Pos_f = startPos2;

  float incrementS1 = (float)stepsToTake1 / stepsToTake * direction1;
  float incrementS2 = (float)stepsToTake2 / stepsToTake * direction2;


  for (int step = 0; step < stepsToTake; ++step) {
    if (stepsToTake1 > 0) currentS1Pos_f += incrementS1;
    if (stepsToTake2 > 0) currentS2Pos_f += incrementS2;
    
    // Only write if the target position hasn't been overshot (for the servo that might have fewer steps)
    if (stepsToTake1 > 0) {
        if ((direction1 > 0 && round(currentS1Pos_f) <= endPos1) || (direction1 < 0 && round(currentS1Pos_f) >= endPos1)) {
            s1.write(round(currentS1Pos_f));
        } else {
            s1.write(endPos1); // Ensure it reaches the end
        }
    }

    if (stepsToTake2 > 0) {
         if ((direction2 > 0 && round(currentS2Pos_f) <= endPos2) || (direction2 < 0 && round(currentS2Pos_f) >= endPos2)) {
            s2.write(round(currentS2Pos_f));
        } else {
            s2.write(endPos2); // Ensure it reaches the end
        }
    }
    
    // Blink buzzer based on servo1's progress (or a combined progress if desired)
    if(stepsToTake1 > 0) blinkBuzzer(round(currentS1Pos_f), startPos1, endPos1); 
    else if (stepsToTake2 > 0) blinkBuzzer(round(currentS2Pos_f), startPos2, endPos2);

    if (additionalDelayPerStep > 0) {
      delay((unsigned long)additionalDelayPerStep);
    }
  }
  s1.write(endPos1); // Ensure final positions are set
  s2.write(endPos2);
  longBeep();
}

// ---- Function to get distance from a specific ultrasonic sensor ----
float getUltrasonicDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 50000); 
  
  if (duration == 0) { 
    return 9999.0; 
  }
  float distance = (duration * 0.0343) / 2.0;
  return distance;
}

// ---- Telegram Message Handler ----
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String text = bot.messages[i].text;
    String chat_id_str = bot.messages[i].chat_id; 

    if (chat_id_str != CHAT_ID) {
      bot.sendMessage(chat_id_str, "Unauthorized access!", "");
      continue;
    }

    if (text == "/start") {
      String welcome = "Welcome to Smart Railway Gate Control\n\n";
      welcome += "/open - Open the railway gate\n";
      welcome += "/close - Close the railway gate\n";
      welcome += "/status - Check gate status";
      bot.sendMessage(chat_id_str, welcome, "");
      lcd_display_temporary("Bot Started!", "Use /open /close");
    }

    else if (text == "/open") {
      if (!gateOpen) {
        bot.sendMessage(chat_id_str, "Opening gate...", "");
        lcd_display_temporary("Opening Gate...", "Please Wait");
        digitalWrite(RED_LED, LOW);    
        digitalWrite(GREEN_LED, HIGH); 
        // Servo1: 0 (closed) to 90 (open)
        // Servo2: 0 (closed) to 90 (open) // <<< MODIFIED HERE
        smoothMoveServosTogether(servo1, servo2, 0, 90, 0, 90, SERVO_MOVEMENT_ADDITIONAL_DELAY_MS);
        gateOpen = true;
        bot.sendMessage(chat_id_str, "Gate is now OPEN.", "");
        lcd_display_temporary("Gate is OPEN", "");
      } else {
        bot.sendMessage(chat_id_str, "Gate is already open.", "");
        lcd_display_temporary("Gate is ALREADY", "OPEN");
      }
    }

    else if (text == "/close") {
      float distance1 = getUltrasonicDistance(ULTRASONIC_TRIG_PIN_1, ULTRASONIC_ECHO_PIN_1);
      float distance2 = getUltrasonicDistance(ULTRASONIC_TRIG_PIN_2, ULTRASONIC_ECHO_PIN_2);

      Serial.print("US1: "); Serial.print(distance1,1); Serial.print(" cm, US2: "); Serial.print(distance2,1); Serial.println(" cm");

      bool sensor1_error = (distance1 >= 9990.0); 
      bool sensor2_error = (distance2 >= 9990.0);

      String eventMsgTelegram = "";
      String lcdLine1 = "", lcdLine2 = "";

      if (sensor1_error && sensor2_error) {
        eventMsgTelegram = "Both US1 & US2 sensor error! Gate not closing.";
        lcdLine1 = "US1 & US2 ERR"; lcdLine2 = "Check Sensors!";
      } else if (sensor1_error) {
        eventMsgTelegram = "US1 sensor error! Gate not closing.";
        lcdLine1 = "US1 SENSOR ERR"; lcdLine2 = "Check US1!";
      } else if (sensor2_error) {
        eventMsgTelegram = "US2 sensor error! Gate not closing.";
        lcdLine1 = "US2 SENSOR ERR"; lcdLine2 = "Check US2!";
      }

      if (eventMsgTelegram == "") { 
          bool obstacle1 = (distance1 > 0.1 && distance1 < OBSTACLE_THRESHOLD);
          bool obstacle2 = (distance2 > 0.1 && distance2 < OBSTACLE_THRESHOLD);
          bool tooClose1 = (distance1 > 0.1 && distance1 <= 2.0); 
          bool tooClose2 = (distance2 > 0.1 && distance2 <= 2.0);

          if (obstacle1 && obstacle2) {
            eventMsgTelegram = "Obstacles: US1 (" + String(distance1,1) + "cm) & US2 (" + String(distance2,1) + "cm). Cannot close.";
            lcdLine1 = "OBST US1&2"; lcdLine2 = String(distance1,0) + "&" + String(distance2,0) + "cm";
          } else if (obstacle1) {
            eventMsgTelegram = "Obstacle US1: " + String(distance1,1) + "cm. Cannot close.";
            lcdLine1 = "OBSTACLE US1"; lcdLine2 = String(distance1,1) + " cm";
          } else if (obstacle2) {
            eventMsgTelegram = "Obstacle US2: " + String(distance2,1) + "cm. Cannot close.";
            lcdLine1 = "OBSTACLE US2"; lcdLine2 = String(distance2,1) + " cm";
          } else if (tooClose1 && tooClose2) { 
            eventMsgTelegram = "Too Close: US1 (" + String(distance1,1) + "cm) & US2 (" + String(distance2,1) + "cm). Check manually.";
            lcdLine1 = "CLOSE US1&2"; lcdLine2 = String(distance1,0) + "&" + String(distance2,0) + "cm CHK";
          } else if (tooClose1) {
            eventMsgTelegram = "Too Close US1: " + String(distance1,1) + "cm. Check manually.";
            lcdLine1 = "CLOSE US1"; lcdLine2 = String(distance1,1) + "cm CHK";
          } else if (tooClose2) {
            eventMsgTelegram = "Too Close US2: " + String(distance2,1) + "cm. Check manually.";
            lcdLine1 = "CLOSE US2"; lcdLine2 = String(distance2,1) + "cm CHK";
          }
      }

      if (eventMsgTelegram != "") { 
        bot.sendMessage(chat_id_str, eventMsgTelegram, "");
        lcd_display_temporary(lcdLine1, lcdLine2);
      }
      else { 
        if (gateOpen) {
          bot.sendMessage(chat_id_str, "No obstacle. Closing gate...", "");
          lcd_display_temporary("Closing Gate...", "Please Wait");
          digitalWrite(GREEN_LED, LOW); 
          digitalWrite(RED_LED, HIGH);  
          // Servo1: 90 (open) to 0 (closed)
          // Servo2: 90 (open) to 0 (closed)  // <<< MODIFIED HERE
          smoothMoveServosTogether(servo1, servo2, 90, 0, 90, 0, SERVO_MOVEMENT_ADDITIONAL_DELAY_MS);
          gateOpen = false;
          bot.sendMessage(chat_id_str, "Gate is now CLOSED.", "");
          lcd_display_temporary("Gate is CLOSED", "");
        } else {
          bot.sendMessage(chat_id_str, "Gate is already closed.", "");
          lcd_display_temporary("Gate is ALREADY", "CLOSED");
        }
      }
    }

    else if (text == "/status") {
      String statusMessage = gateOpen ? "Gate is currently OPEN." : "Gate is currently CLOSED.";
      bot.sendMessage(chat_id_str, statusMessage, "");
      if(gateOpen) lcd_display_temporary("Gate Status:", "OPEN");
      else lcd_display_temporary("Gate Status:", "CLOSED");
    }
  }
}

void setup() {
  Serial.begin(115200);

  Wire.begin(); 
  lcd.init();      
  lcd.backlight(); 
  setupLcdScrollText(); 
  lcd_display_temporary("System Booting", "Please Wait..."); 

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); 
  digitalWrite(RED_LED, HIGH);   
  digitalWrite(GREEN_LED, LOW);  

  pinMode(ULTRASONIC_TRIG_PIN_1, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN_1, INPUT);
  pinMode(ULTRASONIC_TRIG_PIN_2, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN_2, INPUT);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo1.write(0);    // Servo1 closed position
  servo2.write(0);  // Servo2 closed position (was 180) // <<< MODIFIED HERE
  gateOpen = false; 

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  lcd_display_temporary("Connecting WiFi", ssid);
  WiFi.begin(ssid, password);
  int wifi_retries = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_retries < 20) { 
    delay(500);
    Serial.print(".");
    wifi_retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    client.setInsecure(); 
    Serial.println("\nWiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    lcd_display_temporary("WiFi Connected", WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection FAILED.");
    lcd_display_temporary("WiFi FAILED", "Check Network");
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) { 
    if (millis() - lastTimeBotRan > 1000) { 
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      lastTimeBotRan = millis();
    }
  } else {
      if (millis() % 10000 == 0) { 
          Serial.println("WiFi disconnected. Attempting to reconnect...");
          lcd_display_temporary("WiFi Lost", "Reconnecting...");
          WiFi.disconnect(true); 
          delay(100); 
          WiFi.begin(ssid, password);
      }
  }
  lcd_task(); 
}