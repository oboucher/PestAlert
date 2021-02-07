#include <bluefruit.h>
#include <dht.h>
#include <SD.h>
#include <SPI.h>

// FOR HARDWARE SERIAL 
#include <Adafruit_VC0706.h>

//FOR SOFTWARE SERIAL

// BLE Service
BLEDfu  bledfu; // OTA DFU service
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

// Image file, IMAGENUM, and sendIMG are all needed globally, so we need them up here
char imageFile[13];
int IMGNUM;
char sendImg[13];


void loop() {
  //for some reason need to redefine motiondetect to be true
  cam.setMotionDetect(true);
  
  while ( bleuart.available() ) {
    // Get the input from the rPi, then print it out so we know its right
    int c = bleuart.read();
    Serial.print((char)c);

    //This just takes a picture
    if ((char)c == 'x') {
      // turn LED on:
      Serial.println("High");
      saveImage();
    }
    //This sends image00.jpg, useful to make sure we are getting an image, does not rely on sd card
    if ((char)c == 'p'){
      //transmitImageData();
      strcpy(sendImg, "IMAGE00.JPG");
      TXimageName();
      delay(8);
      sendImageFile();
    }
    
    //Runs the main sending method
    if ((char)c == 'r'){
      //Save a picture
      saveImage();
      //send a c so the rpi knows an image was captured and is ready to be sent so it can advance
      bleuart.write('c');
      // First send the image name
      TXimageName();
      //Then send teh image data
      sendImageFile();
      // now get the temperature and humidity
      // TODO: this doent curently work, we will need to get the new senor set up once it arrives, it uses I2C instead of an alalog value and has a differnt library
      //DHT.read11(dht_apin);
      int humid = 17;//DHT.humidity;
      int temp = 17;//DHT.temperature;
      //Generate line then fill and send it
      char textPrint[32];
      sprintf(textPrint, "%s, %d, %d", imageFile, humid, temp);
      bleuart.write(textPrint);
      //Print a blank line, followed by the text string
      Serial.println();
      Serial.println(textPrint);
      
    }
    
    // Test function to make sure data is being sent
    if ((char)c == 'w'){
      bleuart.write("test");
      Serial.println("test");
    }
  }

  //wait 500ms before getting next input
  delay(500);
}

//sends image name and prints it on teh serial monitor
void TXimageName(){
  Serial.println(sendImg);
  bleuart.print(sendImg);
}

//Function that saves the image, right now we dont need a function to fix teh camera when it locks up, but we might later after more testing
// This was basically taken from the example code and works well
// TODO: make this work with up to 999 images instead of 99. Needed for CDR
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

//this finds the image name, then sends the image
//finds name by getting the highest number
//TODO: Make this work with 999 images as well. Needed for CDR
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

//I DONT THINNNNNK this is used anywhere, so commenting it out
/*
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
*/

//SENDS IMAGE FILE
//THIS NEEDS A LOT OF WORk
//TODO: right now this no longer works, and i cant figure out why, need to test it and get it working ASAP
void sendImageFile(){
  // debug print statemnts
  Serial.println();
  Serial.print("Sending Image: ");
  Serial.print(sendImg);
  Serial.println(" over BLE UART");
  //open sendIMG as a readable file
  File imgFile = SD.open(sendImg, FILE_READ);
  
  //AGAIN, DONT KNOW WHY THESE ARE WHAT THEY ARE, so leave it or try to figure out what the point of them is
  char n, inputs[BUFSIZE+1], c;
  
  //While there are still bits to print from the file
  while (imgFile.available()){
    //Read BUFSIZE bytes from image file and store into the inputs array
    n = imgFile.read(inputs, BUFSIZE);
    //set the last bit of teh inputs array to zero, idk why
    inputs[n] = 0;

    
    delay(10); //Need to have an at least 7.5ms interval between transmission for BLE
    //write BUFSIZE bytes from inputs array on BLE uart service, 
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
