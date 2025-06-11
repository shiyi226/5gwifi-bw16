#include "wifi_cust_tx.h"

/*
 * 发送一个指定长度的原始802.11帧
 * 该帧必须是有效的，且序列号为0（将会自动设置）
 * 帧校验序列会自动添加，不需要包含在长度中
 * @param frame 指向原始帧的指针
 * @param size 帧的大小
*/
void wifi_tx_raw_frame(void* frame, size_t length) {
  void *ptr = (void *)**(uint32_t **)(rltk_wlan_info + 0x10);
  void *frame_control = alloc_mgtxmitframe(ptr + 0xae0);

  if (frame_control != 0) {
    // 更新帧属性
    update_mgntframe_attrib(ptr, frame_control + 8);
    // 清空帧控制数据区
    memset((void *)*(uint32_t *)(frame_control + 0x80), 0, 0x68);
    // 获取帧数据指针并复制数据
    uint8_t *frame_data = (uint8_t *)*(uint32_t *)(frame_control + 0x80) + 0x28;
    memcpy(frame_data, frame, length);
    // 设置帧长度
    *(uint32_t *)(frame_control + 0x14) = length;
    *(uint32_t *)(frame_control + 0x18) = length;
    // 发送帧
    dump_mgntframe(ptr, frame_control);
  }
}

/*
 * 在当前信道发送802.11解除认证帧
 * @param src_mac 包含发送者MAC地址的字节数组，必须为6字节
 * @param dst_mac 包含目标MAC地址的字节数组，或使用FF:FF:FF:FF:FF:FF进行广播
 * @param reason 符合802.11规范的原因码（可选）
*/
void wifi_tx_deauth_frame(void* src_mac, void* dst_mac, uint16_t reason) {
  DeauthFrame frame;
  // 设置源MAC地址
  memcpy(&frame.source, src_mac, 6);
  // 设置接入点MAC地址
  memcpy(&frame.access_point, src_mac, 6);
  // 设置目标MAC地址
  memcpy(&frame.destination, dst_mac, 6);
  // 设置解除认证原因
  frame.reason = reason;
  // 发送帧
  wifi_tx_raw_frame(&frame, sizeof(DeauthFrame));
}

/*
 * 在当前信道发送一个基本的802.11信标帧
 * @param src_mac 包含发送者MAC地址的字节数组，必须为6字节
 * @param dst_mac 包含目标MAC地址的字节数组，或使用FF:FF:FF:FF:FF:FF进行广播
 * @param ssid 以'\0'结尾的字符数组，表示SSID
*/
void wifi_tx_beacon_frame(void* src_mac, void* dst_mac, const char *ssid) {
  BeaconFrame frame;
  // 设置源MAC地址
  memcpy(&frame.source, src_mac, 6);
  // 设置接入点MAC地址
  memcpy(&frame.access_point, src_mac, 6);
  // 设置目标MAC地址
  memcpy(&frame.destination, dst_mac, 6);
  // 复制SSID并计算长度
  for (int i = 0; ssid[i] != '\0'; i++) {
    frame.ssid[i] = ssid[i];
    frame.ssid_length++;
  }
  // 发送帧（帧大小为基础大小38字节加上SSID长度）
  wifi_tx_raw_frame(&frame, 38 + frame.ssid_length);
}
