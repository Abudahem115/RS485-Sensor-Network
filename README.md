# RS485-Sensor-Network
A complete IoT and industrial integration project using ET-ESP32 RS485 V3 (Master), ESP8266 (Slave ID=10), RS485 Modbus RTU, Blynk IoT, LCD Display 20x4.

This project builds a sensor network over RS485 where:
- ET-ESP32 RS485 V3 acts as the Modbus Master → reads sensors, displays values on a 20x4 I²C LCD, and uploads data to Blynk IoT.
- ESP8266 acts as a Modbus RTU Slave (ID=10) → provides 12 Holding Registers with environmental values.

--------------------------------------------------------------------------------------------------------------------------------------------------------------------

# Features
  - ET-ESP32 RS485 V3 (Master)
    - Integrated RS485 transceiver (no MAX485 needed).
    - Polls Modbus RTU slaves (sensors, ESP8266).
    - Displays sensor values on I²C LCD (20x4).
    - Pushes data to Blynk Cloud.
    - Threshold control logic for soil & air temp/humidity.

  - ESP8266 (Slave, ID=10)
    - Configured as Modbus RTU Slave.
    - Provides 12 holding registers:
    - **Weather Sensor** - Air humidity, air temperature, light (lux).
    - **Soil Sensor** - Soil humidity, soil temperature, EC, pH, Nitrogen, Phosphorus, Potassium, Salinity, TDS.

--------------------------------------------------------------------------------------------------------------------------------------------------------------------

# Hardware Used
  - ET-ESP32 RS485 V3 
  - ESP8266 NodeMCU
  - MAX485 (Optional)
  - ETT ET-HUB 4 BOX
  - ET-USB USART/TTL (Optional)
  - DR-60-24
  - Sensor Weather SN-300BYH-M
  - Soil Sensor RS-NPK-N01-TR18
