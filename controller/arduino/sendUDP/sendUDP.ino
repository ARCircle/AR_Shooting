//docks: https://www.arduino.cc/reference/en/libraries/wifi/

#include <M5StickCPlus.h>
#include <WiFi.h>
#include <WiFiUDP.h>
#include <string.h>
// to config i2s
#include <driver/i2s.h>
#include <math.h>
// to call MahonyAHRSupdateIMU()
#include "utility/MahonyAHRS.h"

//WiFi config
const char* SSID = "tenk-LAN 2.4G";
const char* pass = "x24j7d68";
const char* sendto_addr = "192.168.195.76"; //may change
const int sendto_port = 10503;
WiFiUDP UDP;
#define MSG_SIZE (sizeof(char) + sizeof(float) * 6)
uint8_t message[MSG_SIZE];
#define PACKET_INTERVAL 5 // 200Hz, 5ms interval
bool under_config_mode = false;
bool btnB_pressed = false;

// IMU config
float gyro_offset[3] = {0};
float acc_offset[3] = {0};
float yaw_delta = 0;
float gyroX = 0, gyroY= 0, gyroZ= 0;
float accX= 0, accY= 0, accZ= 0;
#define OFFSET_RANGE 5000.0

//I2S config
//I2S config
#define I2S_NUM I2S_NUM_0
#define I2S_PIN_CLK     0
#define I2S_PIN_DATA    34
// #define READ_LEN    1024
#define GAIN_FACTOR 1
#define SAMPLE_RATE 10000 // Hz
#define BIT_PER_SAMPLE 16
# define RMS_RANGE_SEC 0.005 // [sec]
// calc RMS every 0.005sec , 16bit/sample -> read 1600[bit] = 800[byte]
#define READ_LEN (BIT_PER_SAMPLE * SAMPLE_RATE * RMS_RANGE_SEC / 2) //[byte]
#define RMS_THRES 4000
#define RMS_BUFF_SIZE (SAMPLE_RATE * RMS_RANGE_SEC)
uint16_t REC_BUFFER[int(READ_LEN)] = {0};
int16_t *calcBuffer = NULL;
size_t datasize; // amount of read bytes by i2s_read()
long long I2S_sum = 0;
int RMS = 0;
char fire_c = NULL;
int MIC_OFFSET = -770; //@hino circle room

//color def
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// //typedef pair
// typedef struct{
//   int RMS;
//   bool overThres;
// } RMS_pair;

// -----microphone functions-----
void mic_init(){
  //i2s config
  //参考
  // esp32 i2sのパラメータ説明
  // https://lang-ship.com/blog/work/esp32-i2s-mic/
  // https://lang-ship.com/blog/work/esp32-i2s-mic-2/
  // i2s configの例
  // https://docs.espressif.com/projects/esp-idf/en/v3.3/api-reference/peripherals/i2s.html#application-example
  // M5stickC+のマイクスケッチ例
  // https://github.com/m5stack/M5StickC/blob/master/examples/Basics/Micophone/Micophone.ino
  // マイクスケッチ例2
  // https://blog.revetronique.com/i2smic-esp32-3/
  M5.Lcd.setTextColor(MAGENTA, BLACK);
  M5.Lcd.printf("Init I2S.\n");
  M5.Lcd.setTextColor(WHITE, BLACK);
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, //records as stereo, change to monoral @i2s_set_clk
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // default interrupt priority
      .dma_buf_count = 2,
      .dma_buf_len = 512, // [byte]
  };
  i2s_pin_config_t pin_config = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num   = I2S_PIN_NO_CHANGE,
      .ws_io_num = I2S_PIN_CLK,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_PIN_DATA,
  };
  esp_err_t err;
  err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  M5.Lcd.printf("i2s_driver_install %s\n", esp_err_to_name(err));
  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  M5.Lcd.printf("i2s_set_pin %s\n", esp_err_to_name(err));
  err = i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  M5.Lcd.printf("i2s_set_clk %s\n", esp_err_to_name(err));
  err = i2s_read(
      I2S_NUM,
      (char*)REC_BUFFER, //バイナリで取得されるので，文字(0,1)で格納
      READ_LEN,
      &datasize,
      // portMAX_DELAY
      (100 / portTICK_RATE_MS)
    ); 
  M5.Lcd.printf("i2s_read %s\n", esp_err_to_name(err));
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.Lcd.printf("DONE: %d[Hz], %d[bit]\nRMS range: %.5f[sec]\n", SAMPLE_RATE, BIT_PER_SAMPLE, RMS_RANGE_SEC);
}
void calc_rms(){
  M5.Lcd.setTextColor(YELLOW, BLACK);
  for(int i = 0; i < datasize; i++){
    //I2S_sum += calcBuffer[i] + MIC_OFFSET;
    I2S_sum += pow(((calcBuffer[i] + MIC_OFFSET) * GAIN_FACTOR), 2);
  }
  //RMS = I2S_sum * 1.0 / datasize;
  RMS = sqrt(I2S_sum * 1.0/(datasize));
  I2S_sum = 0;
}
void set_fire_c(){
  // if btnB was pressed(btnB_pressed==true), send fire code: c once
  // else, send code: f
  // when no fire detection, send NULL
  if(btnB_pressed){
    under_config_mode = true;
  }
  if(RMS>RMS_THRES){
    if(under_config_mode){
      fire_c = 'c';
      under_config_mode = false; //exit config mode
    }
    else
      fire_c = 'f';
  }
  else{
    fire_c = NULL;
  }
}
void show_rms(){
  //print
  if(under_config_mode){
    M5.Lcd.setCursor(0,48);
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.printf(">>AIM CENTER OF MAIN MONITOR.<<\n");
    digitalWrite(10, LOW);
  }
  else{
    M5.Lcd.setCursor(0,48);
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.printf("                                       \n");
    digitalWrite(10, HIGH);
  }
  M5.Lcd.setCursor(0,16);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.printf("RMS %d     \n", RMS);
  if(RMS>RMS_THRES){
    M5.Lcd.setTextColor(MAGENTA, BLACK);
    digitalWrite(10, LOW);
    M5.Lcd.printf("FIRE %d     \n",RMS);
  }
  else{
    digitalWrite(10, HIGH);
  }
}
void button_fire(){
  // set fire_c bool True when buttonA is pressed.
  M5.update();
  if(M5.BtnA.isPressed()){
    fire_c = 'f';
  }
  else{
    fire_c= NULL;
  }
}
void mic_task(void* arg){
  esp_err_t err;
  M5.update();
  while(true){
    M5.update();
    btnB_pressed = M5.BtnB.wasPressed(); //config mode detection
    // M5.Lcd.fillScreen(BLACK);
    // params: https://docs.espressif.com/projects/esp-idf/en/v4.2.3/esp32/api-reference/peripherals/i2s.html
    err = i2s_read(
      I2S_NUM,
      &REC_BUFFER,
      READ_LEN,
      &datasize,
      (100 / portTICK_RATE_MS) // no timeout
    ); 
    calcBuffer = (int16_t *)REC_BUFFER; // バイナリ文字列で記録されているので，intにキャスト
    calc_rms();
    show_rms();
    set_fire_c();
    // button_fire();
    delay(1);
  }
}
// ----------


// -----WiFi functions-----
void WiFiConnect(){
  //connect to WiFi
  WiFi.begin(SSID, pass);
  M5.Lcd.setTextColor(MAGENTA);
  M5.Lcd.printf("Connecting");
  while(WiFi.status() != WL_CONNECTED){
    M5.Lcd.print('.');
    delay(1000);
  }
  const char* my_addr = WiFi.localIP().toString().c_str();
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.printf("\nWiFi connected.\n");
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.printf(
  ".SSID %s\n"
  ".my_addr %s\n"
  ".sendto %s, %d\n"
  , SSID, my_addr, sendto_addr, sendto_port);
}
void UdpSend(void* arg){
  int status = 0;

  while(true){

    status = UDP.beginPacket(sendto_addr, sendto_port);
    memcpy(&message, &fire_c, sizeof(fire_c)); // fire detection bool
    memcpy((uint8_t*)&message + sizeof(char) + sizeof(float) * 0, &gyroX, sizeof(gyroX)); // gyroX
    memcpy((uint8_t*)&message + sizeof(char) + sizeof(float) * 1, &gyroY, sizeof(gyroY)); // gyroY
    memcpy((uint8_t*)&message + sizeof(char) + sizeof(float) * 2, &gyroZ, sizeof(gyroZ)); // gyroZ
    memcpy((uint8_t*)&message + sizeof(char) + sizeof(float) * 3, &accX, sizeof(accX)); // accX
    memcpy((uint8_t*)&message + sizeof(char) + sizeof(float) * 4, &accY, sizeof(accY)); // accy
    memcpy((uint8_t*)&message + sizeof(char) + sizeof(float) * 5, &accZ, sizeof(accZ)); // accZ
    status = UDP.write(message,sizeof(message));
    status = UDP.endPacket();
    // M5.Lcd.setCursor(0,48);
    // M5.Lcd.printf("%d",status);
    delay(PACKET_INTERVAL);

  }
}
// ----------


// -----IMU functions-----
void init_IMU(){
  //docs: https://docs.m5stack.com/en/api/stickc/imu
  M5.Lcd.setTextColor(MAGENTA, BLACK);
  int status;
  status = M5.IMU.Init();
  if(status == 0){
    M5.Lcd.printf("IMU.Init OK\n");
    // getIMUoffset(gyro_offset);
  }
  else{
    M5.Lcd.printf("IMU.Init FAILED");
    delay(500);
  }
}
void getIMUoffset(float* gyro_offset){
  // print indication for user
  M5.Lcd.setTextColor(RED, BLACK);
  M5.Lcd.printf("--PUT MODULE FLAT & STABLE--\n");
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.Lcd.printf("3...    ");
  delay(1000);
  M5.Lcd.printf("2...    ");
  delay(1000);
  M5.Lcd.printf("1...    ");
  delay(1000);
  M5.Lcd.printf("0...    \n");
  delay(1000);
  // get avg offset
  float buff[6]= {0};
  double sum[6] = {0};
  for(int i=0; i < OFFSET_RANGE*2; i++){
    if(i > OFFSET_RANGE){
      M5.IMU.getGyroData(&buff[0], &buff[1], &buff[2]);
      M5.IMU.getAccelData(&buff[3], &buff[4], &buff[5]);
      for(int j = 0; j < 6; j++){
        sum[j] += buff[j];
      }
    }
  }
  for(int i=0; i < 3; i++){
    gyro_offset[i] = sum[i] / OFFSET_RANGE;
    acc_offset[i] = sum[i+3] / OFFSET_RANGE;
  }
  for(int i=0; i < 3; i++){
    if((acc_offset[i%3] > acc_offset[(i+1)%3]) && (acc_offset[i%3] > acc_offset[(i+2)%3])){
      acc_offset[i%3] -= 1.0;
    }
  }
  //print offset
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.printf("gyro offset:\n%f, %f, %f\n",
    gyro_offset[0], gyro_offset[1], gyro_offset[2]);
  M5.Lcd.printf("accel offset:\n%f, %f, %f\n",
    acc_offset[0], acc_offset[1], acc_offset[2]);
}

void getGyroAcc(void* arg){
  //参考: https://qiita.com/foka22ok/items/53d5271a21313e9ddcbd
  while(true){
    M5.IMU.getGyroData(&gyroX, &gyroY, &gyroZ);
    M5.IMU.getAccelData(&accX, &accY, &accZ);
    //offset
    // gyroX -= gyro_offset[0];
    // gyroY -= gyro_offset[1];
    // gyroZ -= gyro_offset[2];
    // M5.Lcd.print(gyroX);
  }
}
// ----------


// -----main section-----
void call_nextMode(){
  while(M5.BtnA.wasPressed()!=1){
    M5.update();
    delay(500);
  }
  M5.Lcd.setCursor(0,0);
  M5.Lcd.fillScreen(BLACK);
  M5.update();
}
void setup() {
  // hardware settings
  pinMode(10, OUTPUT); //for LED handle
  digitalWrite(10, HIGH); // off LED
  M5.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextSize(1);
  //start init settings
  //wifi
  M5.Lcd.setTextColor(RED);
  M5.Lcd.printf("--WiFi CONNECT--\n");
  WiFiConnect();
  UDP.begin(sendto_port);
  call_nextMode();
  //mic
  M5.Lcd.setTextColor(RED, BLACK);
  M5.Lcd.printf("--MIC INIT--\n");
  mic_init();
  call_nextMode();
  //imu
  M5.Lcd.setTextColor(RED, BLACK);
  M5.Lcd.printf("--MIC INIT--\n");
  init_IMU();
  call_nextMode();
  // udp_task
  M5.update();
  M5.Lcd.setCursor(0,0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(RED, BLACK);
  M5.Lcd.printf("--UDP MODE--\n");
  // explanation of xTaskCreateUniversal()
  // https://lang-ship.com/blog/work/esp32-freertos-l02-taskcreate/
  // http://www.azusa-st.com/kjm/FreeRtos/API/taskcreate.html
  // core0 is used for WiFi tasks -> xCoreID = 1;
  xTaskCreateUniversal(mic_task, "mic_task",2048, NULL, 1, NULL, 1);
  xTaskCreateUniversal(getGyroAcc, "getGyroAcc", 8192, NULL, 2, NULL, 1);
  xTaskCreateUniversal(UdpSend, "UdpSend",16384, NULL, 3, NULL, 1);
  // udp task

}

void loop() {
  // M5.Lcd.setCursor(0,0);
  // M5.Lcd.printf("A");
  // mic_record_task();
  // M5.Lcd.printf("rms");
  // M5.Lcd.setCursor(0,100); //動的描画領域
  // M5.Lcd.printf("B");
  // showI2SInfo(i2s_rms.RMS,i2s_rms.overThres);
}
// ----------