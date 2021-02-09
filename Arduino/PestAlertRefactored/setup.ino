void setup() {
  pinMode(chipSelect, OUTPUT);
  
  Serial.begin(115200);
  Serial1.begin(38400);
  
  initSD();
  initCAM();
  initBLE();

  delay(500);
  Serial.println(F("------------------------------------------------"));
}

//Confirms that there is an SD card and it can be communicated with
void initSD() {
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
}

void initCAM() {
  
  Serial.println("VC0706 Camera test");

  //Attempt to find Camera    
  if (cam.begin()) {
    Serial.println("Camera Found:");
  } else {
    Serial.println("No camera found?");
    return;
  }
  
  //Sets image size to be 640x480, the max the camera supports
  cam.setImageSize(VC0706_640x480);

  // enable Camera Motion Detection, doesnt actually do anything, but becomes unresponsive if this is not here
  cam.setMotionDetect(true);
}


void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}


void initBLE() {
  // Setup the BLE LED to be enabled on CONNECT
  Bluefruit.autoConnLed(true);

  // Config the peripheral connection with maximum bandwidth 
  // more SRAM required by SoftDevice
  // Note: All config***() function must be called before begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  
  //Tx rate +4dbm
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Pest Alert");
  //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // To be consistent OTA DFU should be added first if it exists
  bledfu.begin();

  // Setup BLEdis information and broadcast it
  bledis.setManufacturer("SDP Team 22");
  bledis.setModel("PestAlert");
  bledis.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Start BLE Battery Service
  blebas.begin();
  blebas.write(100);

  // Set up and start advertising
  startAdv();

  Serial.println("BLE On, Connect to Device");
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.println();
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}
