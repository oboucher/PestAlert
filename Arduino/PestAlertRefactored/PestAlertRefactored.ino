#include <bluefruit.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
// FOR HARDWARE SERIAL 
#include <Adafruit_VC0706.h>


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
    
    //This sends image00.jpg, useful to make sure we are getting an image, does not rely on sd card
    if ((char)c == 'p'){
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
      //send image name, temp, and humidity
      //TXdata();
      //send image data
      sendImageFile();
    }
    
    // Test function to make sure data is being sent
    if ((char)c == 'w'){
      TXdata();
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

void TXdata(){
  char textPrint[32];
  unsigned int H_dat, T_dat;
  float RH, T_C;
  int humid, temp;
  getTH(&H_dat, &T_dat);
      
    
  RH = (float) H_dat * 6.10e-3;
  humid = (int) RH;
  T_C = (float) T_dat * 1.007e-2 - 40.0;
  T_C = T_C * 9/5 + 32;
  temp = (int) T_C;

  sprintf(textPrint, "%s, %d, %d", imageFile, humid, temp);
  Serial.println(textPrint);
  bleuart.write(textPrint);
}

//Function that saves the image
//This process was basically taken from the example code and works well

void saveImage() {
  
  if (!cam.takePicture()){
  }
  else{
    Serial.println("Picture taken!");
    
    strcpy(imageFile, "IMAGE000.JPG");
    
    for (int i = 0; i < 1000; i++) {
      imageFile[5] = '0' + i / 100;
      imageFile[6] = '0' + i % 100;
      imageFile[7] = '0' + i % 10;
      // create if does not exist, do not open existing, write, sync after write
      if (!SD.exists(imageFile)) {
      break;
      }
    }
    
  }
  
  File imgFile = SD.open(imageFile, FILE_WRITE);

  uint16_t jpglen = cam.frameLength();

  Serial.print("Writing image to ");
  Serial.print(imageFile);

  while (jpglen > 0) {
    // read 64 bytes of the camera's frame buffer at a time
    uint8_t * buffer;
    uint8_t bytesToRead = min((uint16_t) 64, jpglen);
    buffer = cam.readPicture(bytesToRead);
    imgFile.write(buffer, bytesToRead);
    jpglen -= bytesToRead;
  }
  imgFile.close();
  cam.resumeVideo();
  cam.setMotionDetect(true);
  memcpy(sendImg, imageFile, sizeof(sendImg));
  return;
}

//SENDS IMAGE FILE
//THIS NEEDS A LOT OF WORk
//TODO: works, needs to be much faster, ideally send 16bytes at a time, but over 4 bytes at a time causes data loss 
//      related to speed, i.e. 8bytes looses half the data
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
    
    //for (int i = 0; i< BUFSIZE; i++){
    //  c = inputs[i];
    //  if (c <= 0xF) Serial.print(F("0"));
    //  Serial.print(c, HEX);
    //}
  }
  imgFile.close();
}
void getTH(unsigned int *p_H_dat, unsigned int *p_T_dat)
{
      byte address, Hum_H, Hum_L, Temp_H, Temp_L;
      unsigned int H_dat, T_dat;
      address = 0x27;
      Wire.beginTransmission(address); 
      Wire.endTransmission();
      delay(300);
      
      Wire.requestFrom((int)address, (int) 4);
      //HIGH BITS OF HUM
      Hum_H = Wire.read();
      //LOW BITS
      Hum_L = Wire.read();
      //HIGH BITS OF TEMP
      Temp_H = Wire.read();
      //LOW BITS
      Temp_L = Wire.read();
      Wire.endTransmission();
      
      Hum_H = Hum_H & 0x3f;
      //Combine HIGH and LOW bits of both temp and humidity
      H_dat = (((unsigned int)Hum_H) << 8) | Hum_L;
      T_dat = (((unsigned int)Temp_H) << 8) | Temp_L;
      T_dat = T_dat / 4;
      *p_H_dat = H_dat;
      *p_T_dat = T_dat;
}
