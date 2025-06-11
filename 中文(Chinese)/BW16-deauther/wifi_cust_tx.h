#ifndef WIFI_CUST_TX
#define WIFI_CUST_TX

#include <Arduino.h>

// 定义解除认证帧的结构体
typedef struct {
  uint16_t frame_control = 0xC0;      // 帧控制字段，设置为解除认证类型
  uint16_t duration = 0xFFFF;         // 持续时间字段
  uint8_t destination[6];             // 目标MAC地址
  uint8_t source[6];                  // 源MAC地址
  uint8_t access_point[6];            // 接入点MAC地址
  const uint16_t sequence_number = 0;  // 序列号
  uint16_t reason = 0x06;             // 解除认证原因码
} DeauthFrame;

// 定义信标帧的结构体
typedef struct {
  uint16_t frame_control = 0x80;      // 帧控制字段，设置为信标类型
  uint16_t duration = 0;              // 持续时间字段
  uint8_t destination[6];             // 目标MAC地址
  uint8_t source[6];                  // 源MAC地址
  uint8_t access_point[6];            // 接入点MAC地址
  const uint16_t sequence_number = 0;  // 序列号
  const uint64_t timestamp = 0;       // 时间戳
  uint16_t beacon_interval = 0x64;    // 信标间隔
  uint16_t ap_capabilities = 0x21;    // 接入点能力信息
  const uint8_t ssid_tag = 0;         // SSID标签
  uint8_t ssid_length = 0;            // SSID长度
  uint8_t ssid[255];                  // SSID内容
} BeaconFrame;

// 从闭源库导入所需的C函数
// 注意：函数定义可能不是100%准确，因为在编译过程中类型信息会丢失
extern uint8_t* rltk_wlan_info;
extern "C" void* alloc_mgtxmitframe(void* ptr);
extern "C" void update_mgntframe_attrib(void* ptr, void* frame_control);
extern "C" int dump_mgntframe(void* ptr, void* frame_control);

// 函数声明
void wifi_tx_raw_frame(void* frame, size_t length);
void wifi_tx_deauth_frame(void* src_mac, void* dst_mac, uint16_t reason = 0x06);
void wifi_tx_beacon_frame(void* src_mac, void* dst_mac, const char *ssid);

#endif
