
// array of different xiaomi flora MAC addresses and corresponding pins
char const* FLORA_DEVICES[] = {
    "5C:85:7E:B0:12:77", 
   "5C:85:7E:B0:12:75"
};
const int water_pins[] = {1,2};

// how often should the battery be read - in run count
#define BATTERY_INTERVAL 6
// how often should a device be retried in a run when something fails
#define RETRY 3

#define wet 75
#define dry 25
#define water_duration 5000   // 5sec
#define water_break 30000  //30sec for water to soak into soil
#define dry_break 60000*30  // 30 min for next check


//end of config

// boot count used to check if battery status should be read
uint8_t bootCount = 0;
// device count


// the remote service we wish to connect to
static BLEUUID serviceUUID("00001204-0000-1000-8000-00805f9b34fb");

// the characteristic of the remote service we are interested in
static BLEUUID uuid_version_battery("00001a02-0000-1000-8000-00805f9b34fb");
static BLEUUID uuid_sensor_data("00001a01-0000-1000-8000-00805f9b34fb");
static BLEUUID uuid_write_mode("00001a00-0000-1000-8000-00805f9b34fb");

bool readBattery;
TaskHandle_t hibernateTaskHandle = NULL;
BLEClient* floraClient = nullptr;
static int deviceCount = sizeof FLORA_DEVICES / sizeof FLORA_DEVICES[0];
int watering[] = {false, false};
int moisture_[20];

int wait_handler()
{
  int a=0;
  for (int i = 0; i < deviceCount; i++)
  {
    if (watering[i])
    {
      a++;
    }
    
  }
  if (a>0)
    return water_break;
  else
    return dry_break;
   
}

int device_identifier(char const *device_mac_adress_)
{

    for (int i = 0; i < deviceCount; i++){
        if(FLORA_DEVICES[i]== device_mac_adress_){
            return i;
        }
    }
    
}

void water_handler()
{ 
  for (int i = 0; i < deviceCount; i++)
  {
      if(moisture_[i] < dry && watering[i]==false) {
      watering[i] = true;
    }

    else if(moisture_[i] > wet && watering[i]==true) {
      watering[i] = false;
    }
  }
  Serial.println("watering is set to:");
  Serial.print(watering[0]);
  Serial.print("    ");
  Serial.println(watering[1]);
  
}

void water_do(){
  for (int i = 0; i < deviceCount; i++)
  {
    if (watering[i]){
      Serial.println("start watering");
      digitalWrite(water_pins[i],HIGH);
      delay(water_duration);
      Serial.println("stop watering");
      digitalWrite(water_pins[i], LOW);
    }
  }
  
}

void createClient()
{
  floraClient = BLEDevice::createClient(); 
}


BLEClient* getFloraClient(BLEAddress floraAddress) {
  

  if (!floraClient->connect(floraAddress)) {
    Serial.println("- Connection failed, skipping");
    return nullptr;
  }

  Serial.println("- Connection successful");
  return floraClient;
}

BLERemoteService* getFloraService(BLEClient* floraClient) {
  BLERemoteService* floraService = nullptr;

  try {
    floraService = floraClient->getService(serviceUUID);
  }
  catch (...) {
    // something went wrong
  }
  if (floraService == nullptr) {
    Serial.println("- Failed to find data service");
  }
  else {
    Serial.println("- Found data service");
  }

  return floraService;
}

bool forceFloraServiceDataMode(BLERemoteService* floraService) {
  BLERemoteCharacteristic* floraCharacteristic;
  
  // get device mode characteristic, needs to be changed to read data
  Serial.println("- Force device in data mode");
  floraCharacteristic = nullptr;
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_write_mode);
  }
  catch (...) {
    // something went wrong
  }
  if (floraCharacteristic == nullptr) {
    Serial.println("-- Failed, skipping device");
    return false;
  }

  // write the magic data
  uint8_t buf[2] = {0xA0, 0x1F};
  floraCharacteristic->writeValue(buf, 2, true);

  delay(500);
  return true;
}

bool readFloraDataCharacteristic(BLERemoteService* floraService, String baseTopic, char const *device_mac_address) {
  BLERemoteCharacteristic* floraCharacteristic = nullptr;

  // get the main device data characteristic
  Serial.println("- Access characteristic from device");
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_sensor_data);
  }
  catch (...) {
    // something went wrong
   
  }
  if (floraCharacteristic == nullptr) {
    Serial.println("-- Failed, skipping device");
    return false;
  }

  // read characteristic value
  Serial.println("- Read value from characteristic");
  std::string value;
  try{
    value = floraCharacteristic->readValue();
  }
  catch (...) {
    // something went wrong
    Serial.println("-- Failed, skipping device");
    return false;
  }
  const char *val = value.c_str();

  Serial.print("Hex: ");
  for (int i = 0; i < 16; i++) {
    Serial.print((int)val[i], HEX);
    Serial.print(" ");
  }
  Serial.println(" ");

  int16_t* temp_raw = (int16_t*)val;
  float temperature = (*temp_raw) / ((float)10.0);
  Serial.print("-- Temperature: ");
  Serial.println(temperature);

  int moisture = val[7];
  Serial.print("-- Moisture: ");
  Serial.println(moisture);

  int light = val[3] + val[4] * 256;
  Serial.print("-- Light: ");
  Serial.println(light);
 
  int conductivity = val[8] + val[9] * 256;
  Serial.print("-- Conductivity: ");
  Serial.println(conductivity);

  moisture_[device_identifier(device_mac_address)] = moisture;

  if (temperature > 200) {
    Serial.println("-- Unreasonable values received, skip publish");
    return false;
  }
return true;
 
}

bool readFloraBatteryCharacteristic(BLERemoteService* floraService, String baseTopic) {
  BLERemoteCharacteristic* floraCharacteristic = nullptr;

  // get the device battery characteristic
  Serial.println("- Access battery characteristic from device");
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_version_battery);
  }
  catch (...) {
    // something went wrong
  }
  if (floraCharacteristic == nullptr) {
    Serial.println("-- Failed, skipping battery level");
    return false;
  }

  // read characteristic value
  Serial.println("- Read value from characteristic");
  std::string value;
  try{
    value = floraCharacteristic->readValue();
  }
  catch (...) {
    // something went wrong
    Serial.println("-- Failed, skipping battery level");
    return false;
  }
  const char *val2 = value.c_str();
  int battery = val2[0];

  char buffer[64];

  Serial.print("-- Battery: ");
  Serial.println(battery);

  return true;
}

bool processFloraService(BLERemoteService* floraService, char const* deviceMacAddress, bool readBattery) {
  // set device in data mode
  if (!forceFloraServiceDataMode(floraService)) {
    return false;
  }

  String baseTopic = "a";
  bool dataSuccess = readFloraDataCharacteristic(floraService, baseTopic, deviceMacAddress);

  bool batterySuccess = true;
  if (readBattery) {
    batterySuccess = readFloraBatteryCharacteristic(floraService, baseTopic);
  }
  return true;
}

bool processFloraDevice(BLEAddress floraAddress, char const* deviceMacAddress, bool getBattery, int tryCount) {
  Serial.print("Processing Flora device at ");
  Serial.print(floraAddress.toString().c_str());
  Serial.print(" (try ");
  Serial.print(tryCount);
  Serial.println(")");



  // connect to flora ble server
  BLEClient* floraClient = getFloraClient(floraAddress); // pointer to established connection class
  if (floraClient == nullptr) {
    return false;
  }

  // connect data service
  BLERemoteService* floraService = getFloraService(floraClient);
  if (floraService == nullptr) {
    floraClient->disconnect();
    return false;
  }

   // process devices data
  bool success = processFloraService(floraService, deviceMacAddress, getBattery);

  // disconnect from device
  floraClient->disconnect();
  floraClient = nullptr;
  return 1;
}