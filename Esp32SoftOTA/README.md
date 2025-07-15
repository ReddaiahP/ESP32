This code demonstrates a basic OTA (Over-The-Air) update setup for ESP32 via HTTP.
Note: This is a simple example for OTA updates.

Issue: When flashing new firmware, it overwrites the current OTA logic and replaces it with the new firmware. As a result, the ESP32 may no longer accept future OTA updates unless the new firmware also includes OTA functionality.

To address this, make sure your updated firmware preserves the OTA update logic. You can explore more robust solutions and updated code in my repositories.
