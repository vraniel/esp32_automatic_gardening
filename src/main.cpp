



#include "BLEDevice.h"
#include "Arduino.h"
#include "config.h"




void setup() 
{
  // all action is done when device is woken up
  Serial.begin(115200);
  delay(1000);
  BLEDevice::init("");
  createClient();
}

void loop() 
{

  Serial.println("Initialize BLE client...");
  bootCount++;
  
  // check if battery status should be read - based on boot count
  readBattery = ((bootCount % BATTERY_INTERVAL) == 0);

  // process devices
  for (int i=0; i<deviceCount; i++) {

    int tryCount = 0;
    char const* deviceMacAddress = FLORA_DEVICES[i];
    BLEAddress floraAddress(deviceMacAddress);
  

    while (tryCount < RETRY) {
      tryCount++;
      if (processFloraDevice(floraAddress, deviceMacAddress, readBattery, tryCount)) {
        
        break;
      }
      delay(1000);
    }
    //Serial.println(ESP.getFreeHeap());
    delay(1500);
    

  }

  Serial.println("moisture level is:");
  Serial.print(moisture_[0]);
  Serial.print("    ");
  Serial.println(moisture_[1]);
  water_handler();
  water_do();

  delay(wait_handler());
}

