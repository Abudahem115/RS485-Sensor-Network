// ================= RS485 Listener (ESP8266) =================
// ----- ESP8266 RS485 listen-only -----
// This code makes the ESP8266 act as a passive listener.
// It receives CSV sensor data from the ESP32 Master via RS485
#include <SoftwareSerial.h>

// ================= Pin Definitions =================
#define RS485_RX D6   // GPIO12 -> connect to RO (Receiver Output) of MAX485
#define RS485_TX D7   // GPIO13 -> connect to DI (not used here since we only receive)
#define RS485_DE D1   // GPIO5  -> tie to GND (to keep MAX485 always in receive mode)

// Create a SoftwareSerial instance for RS485 communication
SoftwareSerial RS485Serial(RS485_RX, RS485_TX); // RX, TX

// ================= Sensor Value Variables =================
float airHumi, airTemp, lux;                   // Weather sensor values
float soilHumi, soilTemp, ec, ph, nitrogen, phosphorus, potassium, salinity, tds;            // Soil sensor values - NPK nutrients - Salinity and TDS values

// ================= setup() =================
void setup() {
  Serial.begin(9600);          // Open hardware SerSPial for debugging
  RS485Serial.begin(4800);     // Must match baudrate of ESP32 Master
  Serial.println("=== RS485 Listener (ESP8266) Started ===");
}

// ================= loop() =================
void loop() {
  // Check if any data is available from RS485
  if (RS485Serial.available()) {
    // Read incoming data until newline '\n'
    String dataLine = RS485Serial.readStringUntil('\n');
    dataLine.trim(); // Remove any trailing spaces or \r\n

    // Ensure the line looks like CSV (contains at least one comma)
    if (dataLine.length() > 0 && dataLine.indexOf(',') > 0) {
      // Parse CSV string into floats (expecting 12 values)
      int parsed = sscanf(dataLine.c_str(),
                          "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
                          &airHumi, &airTemp, &lux,
                          &soilHumi, &soilTemp, &ec, &ph,
                          &nitrogen, &phosphorus, &potassium, &salinity, &tds);

      // If all 12 values were parsed successfully
      if (parsed == 12) {
        delay(500);        // Small delay to avoid flooding the Serial Monitor
        printFormatted();  // Print values in a nice formatted block
      }
    }
  }
}

// ================= printFormatted() =================
// Prints all sensor values in a structured, readable format
void printFormatted() {
  Serial.println("----Weather Sensor-----");
  Serial.printf("Air Humidity   : %.1f %%\n", airHumi);
  Serial.printf("Air Temperature: %.1f C\n", airTemp);
  Serial.printf("Light          : %.1f lux\n", lux);

  Serial.println("------Soil Sensor------");
  Serial.printf("Soil Humidity  : %.1f %%\n", soilHumi);
  Serial.printf("Soil Temp      : %.1f C\n", soilTemp);
  Serial.printf("EC             : %.0f uS/cm\n", ec);
  Serial.printf("pH             : %.1f\n", ph);
  Serial.printf("Nitrogen (N)   : %.0f mg/kg\n", nitrogen);
  Serial.printf("Phosphorus (P) : %.0f mg/kg\n", phosphorus);
  Serial.printf("Potassium (K)  : %.0f mg/kg\n", potassium);
  Serial.printf("Salinity       : %.0f mg/L\n", salinity);
  Serial.printf("TDS            : %.0f mg/L\n", tds);
  Serial.println("----------END -----------\n");
}
