// Seeed-Studio-XIAO-ESP32-S3-Sense Make sure OPI PSRAM is active, XIAOESP32S3 is selected and SD card is FORMATTED / ERASED with MSDOS FAT32  
#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#include "camera_pins.h"

unsigned long lastCaptureTime = 0; 
int imageCount = 1;                
bool camera_sign = false;          
bool sd_sign = false;              

void writeFile(fs::FS &fs, const char * path, uint8_t * data, size_t len){
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.write(data, len) == len){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void photo_save(const char * fileName) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Failed to get camera frame buffer");
    return;
  }
  writeFile(SD, fileName, fb->buf, fb->len);
  esp_camera_fb_return(fb);
  Serial.println("Photo saved to file");
}

void findLastImageNumber() {
  File root = SD.open("/");
  if(!root || !root.isDirectory()){
    Serial.println("Unable to open or invalid directory");
    return;
  }

  int maxNumber = 0;
  File file = root.openNextFile();
  while(file){
    String fileName = file.name();
    // file names returned by SD may start with '/'
    if(fileName.startsWith("/")) {
      fileName = fileName.substring(1); // remove leading '/'
    }

    if(fileName.startsWith("image") && fileName.endsWith(".jpg")) {
      // "image" is 5 chars, digits start at index 5
      int startIndex = 5;
      int endIndex = fileName.lastIndexOf('.');
      if(endIndex > startIndex) {
        String numberPart = fileName.substring(startIndex, endIndex);
        int fileNum = numberPart.toInt();
        if(fileNum > maxNumber) {
          maxNumber = fileNum;
        }
      }
    }
    file = root.openNextFile();
  }
  root.close();

  imageCount = maxNumber + 1;
  Serial.printf("Resuming from image number: %d\n", imageCount);
}

void setup() {
  Serial.begin(115200);
  while(!Serial);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  
  camera_sign = true;

  if(!SD.begin(21)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  sd_sign = true;

  findLastImageNumber();

  Serial.println("Setup complete. Will begin taking photos every 3 seconds.");
}

void loop() {
  if(camera_sign && sd_sign){
    unsigned long now = millis();
    if ((now - lastCaptureTime) >= 3000) {
      char filename[32];
      // Include a leading slash when saving
      sprintf(filename, "/image%d.jpg", imageCount);
      photo_save(filename);
      Serial.printf("Saved picture: %s\n", filename);
      imageCount++;
      lastCaptureTime = now;
    }
  }
}
