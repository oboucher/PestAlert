#include <bluefruit.h>
#include <dht.h>
#include <SD.h>
#include <SPI.h>

// FOR HARDWARE SERIAL 
#include <Adafruit_VC0706.h>

//FOR SOFTWARE SERIAL

// BLE Service
BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery


//CS pin 4
#define chipSelect 2
//using Hardware Serial1 for camera
#define cameraconnection Serial1

#define BUFSIZE 4   // Size of the read buffer for incoming data

//Initialize Camera object cam
Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);


char imageFile[13];
int IMGNUM;
char sendImg[13];


void loop() {
  //for some reason need to redefine motiondetect to be true
  cam.setMotionDetect(true);
  
  while ( bleuart.available() ) {
   int c = bleuart.read();

    Serial.print((char)c);
    
    
    if ((char)c == 'x') {
      // turn LED on:
      Serial.println("High");
      saveImage();
    }
    
    
    
    if ((char)c == 'p'){
      //transmitImageData();
      strcpy(sendImg, "IMAGE00.JPG");
      sendImageFile();
    }
    
    
    if ((char)c == 'r'){
      saveImage();
      bleuart.write('c');
      TXimageName();
      sendImageFile();
      
      //DHT.read11(dht_apin);
      int humid = 7;//DHT.humidity;
      int temp = 7;//DHT.temperature;
      char textPrint[32];
      sprintf(textPrint, "%s, %d, %d", imageFile, humid, temp);
      bleuart.write(textPrint);
      Serial.println();
      Serial.println(textPrint);
      
    }
    
    
    if ((char)c == 'w'){
      bleuart.write("test");
      Serial.println("test");
    }
  }
  //if (cam.motionDetected()) {
    //Serial.println("Motion!");
    //cam.setMotionDetect(false);
    //saveImage();
  //}
  delay(500);
}

void TXimageName(){
  Serial.println(sendImg);
  bleuart.print(sendImg);
}

void saveImage() {
  Serial.println("in saveImage");
  
  if (!cam.takePicture()){
    Serial.println("Failed to snap!");
    //cameraFix();
  }
  else{
    Serial.println("Picture taken!");
    
    strcpy(imageFile, "IMAGE00.JPG");
    
    for (int i = 0; i < 100; i++) {
      imageFile[5] = '0' + i / 10;
      imageFile[6] = '0' + i % 10;
      Serial.print(imageFile[5]);
      Serial.println(imageFile[6]);
      // create if does not exist, do not open existing, write, sync after write
      if (!SD.exists(imageFile)) {
      break;
      }
    }
    
  }

  File imgFile = SD.open(imageFile, FILE_WRITE);

  uint16_t jpglen = cam.frameLength();
  //USEFUL FOR DEBUGGING
  //  Serial.print(jpglen, DEC);
  //  Serial.println(" byte image");

  Serial.print("Writing image to ");
  Serial.print(imageFile);

  while (jpglen > 0) {
    // read 32 bytes at a time;
    uint8_t * buffer;
    uint8_t bytesToRead = min((uint16_t) 32, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
    buffer = cam.readPicture(bytesToRead);
    imgFile.write(buffer, bytesToRead);
    jpglen -= bytesToRead;
  }
  imgFile.close();

  Serial.println("...Done!");
  cam.resumeVideo();
  cam.setMotionDetect(true);
  memcpy(sendImg, imageFile, sizeof(sendImg));
  return;
}

/*
void cameraFix(){
  digitalWrite(camPwr, LOW);
  delay(5000);
  digitalWrite(camPwr, HIGH);
  delay(1000);
  Serial.print("Toggled Power");
  initCAM();
  saveImage();
  return;
}
*/

void transmitImageData(){
    strcpy(sendImg, "IMAGE00.JPG");
    for (int i = 0; i < 100; i++) {
      sendImg[5] = '0' + i/10;
      sendImg[6] = '0' + i%10;
      // create if does not exist, do not open existing, write, sync after write
      if (! SD.exists(sendImg)) {
        break;
      }
      sendImageFile();
    }
}

void readWipeTextFile(){
  //create File, starting with just the text document
  File dataFile = SD.open("data.txt", FILE_READ);
  //Create n and inputs, inputs has sisze BUFFSIZE+1
  char n, inputs[BUFSIZE];
  //While there are still bits to print from the file
  while (dataFile.available()){
    //
    n = dataFile.read(inputs, BUFSIZE);
    inputs[n] = 0;
    //FOR DEBUGGING HELPS TO PRINT BYTES ON SERIAL TERMINAL
    //c = inputs[1];
    //if (c <= 0xF) Serial.print(F("0"));
    //Serial.print(c, HEX);
    //Print Charecters on BLE UART
    bleuart.print(inputs[0]);
  }
  dataFile.close();
}

void sendImageFile(){
  Serial.println();
  Serial.print("Sending Image: ");
  Serial.print(sendImg);
  Serial.println(" over BLE UART");
  File imgFile = SD.open(sendImg, FILE_READ);
  
  char n, inputs[BUFSIZE+1], c;
  //FIND DIF BETWEEN PRINT AND WRITE
  //send the name of the file being sent
  //While there are still bits to print from the file

  while (imgFile.available()){
    n = imgFile.read(inputs, BUFSIZE);
    
    inputs[n] = 0;

    
    delay(10); //Need to have an at least 7.5ms interval between transmission for BLE
    bleuart.write(inputs, BUFSIZE);
     //FOR DEBUGGING HELPS TO PRINT BYTES ON SERIAL TERMINAL
    
    for (int i = 0; i< BUFSIZE; i++){
      c = inputs[i];
      if (c <= 0xF) Serial.print(F("0"));
      Serial.print(c, HEX);
    }
  }
  imgFile.close();
}
