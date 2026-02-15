/********************************** (C) COPYRIGHT *******************************
 * File Name          : ble_hid_service.c
 * Author             : Custom Keyboard Library
 * Version            : V1.0
 * Date               : 2024/11/07
 * Description        : 蓝牙 HID 服务实现（支持键盘、鼠标、多媒体）
 *******************************************************************************/

#include "ble_hid_service.h"
#include "hiddev.h"
#include "battservice.h"
#include <string.h>

/* ==================== UUID 定义 ==================== */

// HID 服务 UUID
const uint8_t hidServUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HID_SERV_UUID), HI_UINT16(HID_SERV_UUID)
};

// HID Boot 键盘输入 UUID
const uint8_t hidBootKeyInputUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(BOOT_KEY_INPUT_UUID), HI_UINT16(BOOT_KEY_INPUT_UUID)
};

// HID Boot 键盘输出 UUID
const uint8_t hidBootKeyOutputUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(BOOT_KEY_OUTPUT_UUID), HI_UINT16(BOOT_KEY_OUTPUT_UUID)
};

// HID 信息 UUID
const uint8_t hidInfoUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HID_INFORMATION_UUID), HI_UINT16(HID_INFORMATION_UUID)
};

// HID 报告映射 UUID
const uint8_t hidReportMapUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(REPORT_MAP_UUID), HI_UINT16(REPORT_MAP_UUID)
};

// HID 控制点 UUID
const uint8_t hidControlPointUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HID_CTRL_PT_UUID), HI_UINT16(HID_CTRL_PT_UUID)
};

// HID 报告 UUID
const uint8_t hidReportUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(REPORT_UUID), HI_UINT16(REPORT_UUID)
};

// HID 协议模式 UUID
const uint8_t hidProtocolModeUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(PROTOCOL_MODE_UUID), HI_UINT16(PROTOCOL_MODE_UUID)
};

/* ==================== HID 信息 ==================== */

static const uint8_t hidInfo[HID_INFORMATION_LEN] = {
    LO_UINT16(0x0111), HI_UINT16(0x0111),   // bcdHID (USB HID 1.11)
    0x00,                                    // bCountryCode
    HID_KBD_FLAGS                            // Flags
};

/* ==================== HID 报告描述符 ==================== */

// 复合 HID 报告描述符：键盘 + 鼠标 + 多媒体
static const uint8_t hidReportMap[] = {
    /* ========== 键盘报告 (Report ID 0) ========== */
    0x05, 0x01,         // Usage Page (Generic Desktop)
    0x09, 0x06,         // Usage (Keyboard)
    0xA1, 0x01,         // Collection (Application)
    
    // 修饰键 (8 bits)
    0x05, 0x07,         //   Usage Page (Key Codes)
    0x19, 0xE0,         //   Usage Minimum (224) - Left Control
    0x29, 0xE7,         //   Usage Maximum (231) - Right GUI
    0x15, 0x00,         //   Logical Minimum (0)
    0x25, 0x01,         //   Logical Maximum (1)
    0x75, 0x01,         //   Report Size (1)
    0x95, 0x08,         //   Report Count (8)
    0x81, 0x02,         //   Input (Data, Variable, Absolute)
    
    // 保留字节
    0x95, 0x01,         //   Report Count (1)
    0x75, 0x08,         //   Report Size (8)
    0x81, 0x01,         //   Input (Constant)
    
    // LED 输出报告
    0x95, 0x05,         //   Report Count (5)
    0x75, 0x01,         //   Report Size (1)
    0x05, 0x08,         //   Usage Page (LEDs)
    0x19, 0x01,         //   Usage Minimum (1)
    0x29, 0x05,         //   Usage Maximum (5)
    0x91, 0x02,         //   Output (Data, Variable, Absolute)
    
    // LED 填充
    0x95, 0x01,         //   Report Count (1)
    0x75, 0x03,         //   Report Size (3)
    0x91, 0x01,         //   Output (Constant)
    
    // 按键数组 (6 bytes)
    0x95, 0x06,         //   Report Count (6)
    0x75, 0x08,         //   Report Size (8)
    0x15, 0x00,         //   Logical Minimum (0)
    0x25, 0x65,         //   Logical Maximum (101)
    0x05, 0x07,         //   Usage Page (Key Codes)
    0x19, 0x00,         //   Usage Minimum (0)
    0x29, 0x65,         //   Usage Maximum (101)
    0x81, 0x00,         //   Input (Data, Array)
    
    0xC0,               // End Collection
    
    /* ========== 鼠标报告 (Report ID 1) ========== */
    0x05, 0x01,         // Usage Page (Generic Desktop)
    0x09, 0x02,         // Usage (Mouse)
    0xA1, 0x01,         // Collection (Application)
    0x85, 0x01,         //   Report ID (1)
    0x09, 0x01,         //   Usage (Pointer)
    0xA1, 0x00,         //   Collection (Physical)
    
    // 按钮 (3 bits)
    0x05, 0x09,         //     Usage Page (Buttons)
    0x19, 0x01,         //     Usage Minimum (1)
    0x29, 0x03,         //     Usage Maximum (3)
    0x15, 0x00,         //     Logical Minimum (0)
    0x25, 0x01,         //     Logical Maximum (1)
    0x75, 0x01,         //     Report Size (1)
    0x95, 0x03,         //     Report Count (3)
    0x81, 0x02,         //     Input (Data, Variable, Absolute)
    
    // 填充 (5 bits)
    0x75, 0x05,         //     Report Size (5)
    0x95, 0x01,         //     Report Count (1)
    0x81, 0x01,         //     Input (Constant)
    
    // X, Y, Wheel
    0x05, 0x01,         //     Usage Page (Generic Desktop)
    0x09, 0x30,         //     Usage (X)
    0x09, 0x31,         //     Usage (Y)
    0x09, 0x38,         //     Usage (Wheel)
    0x15, 0x81,         //     Logical Minimum (-127)
    0x25, 0x7F,         //     Logical Maximum (127)
    0x75, 0x08,         //     Report Size (8)
    0x95, 0x03,         //     Report Count (3)
    0x81, 0x06,         //     Input (Data, Variable, Relative)
    
    0xC0,               //   End Collection (Physical)
    0xC0,               // End Collection (Application)
    
    /* ========== 多媒体报告 (Report ID 2) ========== */
    0x05, 0x0C,         // Usage Page (Consumer)
    0x09, 0x01,         // Usage (Consumer Control)
    0xA1, 0x01,         // Collection (Application)
    0x85, 0x02,         //   Report ID (2)
    
    0x15, 0x00,         //   Logical Minimum (0)
    0x26, 0xFF, 0x03,   //   Logical Maximum (1023)
    0x19, 0x00,         //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,   //   Usage Maximum (1023)
    0x75, 0x10,         //   Report Size (16)
    0x95, 0x01,         //   Report Count (1)
    0x81, 0x00,         //   Input (Data, Array)
    
    0xC0                // End Collection
};

uint16_t hidReportMapLen = sizeof(hidReportMap);

/* ==================== HID 报告映射表 ==================== */

static hidRptMap_t hidRptMap[HID_NUM_REPORTS];

/* ==================== 属性变量 ==================== */

// HID 服务属性
static const gattAttrType_t hidService = {ATT_BT_UUID_SIZE, hidServUUID};

// 包含服务（电池）
static uint16_t include = GATT_INVALID_HANDLE;

// HID 信息特性
static uint8_t hidInfoProps = GATT_PROP_READ;

// HID 报告映射特性
static uint8_t hidReportMapProps = GATT_PROP_READ;

// 外部报告引用描述符
static uint8_t hidExtReportRefDesc[ATT_BT_UUID_SIZE] = {
    LO_UINT16(BATT_LEVEL_UUID), HI_UINT16(BATT_LEVEL_UUID)
};

// HID 控制点特性
static uint8_t hidControlPointProps = GATT_PROP_WRITE_NO_RSP;
static uint8_t hidControlPoint;

// HID 协议模式特性
static uint8_t hidProtocolModeProps = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP;
uint8_t hidProtocolMode = HID_PROTOCOL_MODE_REPORT;

/* ===== 键盘输入报告 ===== */
static uint8_t hidReportKeyInProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t hidReportKeyIn;
static gattCharCfg_t hidReportKeyInClientCharCfg[GATT_MAX_NUM_CONN];
static uint8_t hidReportRefKeyIn[HID_REPORT_REF_LEN] = {
    HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT
};

/* ===== 键盘 LED 输出报告 ===== */
static uint8_t hidReportLedOutProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
static uint8_t hidReportLedOut;
static uint8_t hidReportRefLedOut[HID_REPORT_REF_LEN] = {
    HID_RPT_ID_LED_OUT, HID_REPORT_TYPE_OUTPUT
};

/* ===== 鼠标输入报告 ===== */
static uint8_t hidReportMouseInProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t hidReportMouseIn;
static gattCharCfg_t hidReportMouseInClientCharCfg[GATT_MAX_NUM_CONN];
static uint8_t hidReportRefMouseIn[HID_REPORT_REF_LEN] = {
    HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT
};

/* ===== 多媒体输入报告 ===== */
static uint8_t hidReportConsumerInProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t hidReportConsumerIn;
static gattCharCfg_t hidReportConsumerInClientCharCfg[GATT_MAX_NUM_CONN];
static uint8_t hidReportRefConsumerIn[HID_REPORT_REF_LEN] = {
    HID_RPT_ID_CONSUMER_IN, HID_REPORT_TYPE_INPUT
};

/* ===== Boot 键盘输入报告 ===== */
static uint8_t hidReportBootKeyInProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t hidReportBootKeyIn;
static gattCharCfg_t hidReportBootKeyInClientCharCfg[GATT_MAX_NUM_CONN];

/* ===== Boot 键盘输出报告 ===== */
static uint8_t hidReportBootKeyOutProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
static uint8_t hidReportBootKeyOut;

/* ==================== 属性表 ==================== */

static gattAttribute_t hidAttrTbl[] = {
    // HID 服务
    {{ATT_BT_UUID_SIZE, primaryServiceUUID}, GATT_PERMIT_READ, 0, (uint8_t *)&hidService},
    
    // 包含服务（电池）
    {{ATT_BT_UUID_SIZE, includeUUID}, GATT_PERMIT_READ, 0, (uint8_t *)&include},
    
    // HID 信息声明
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &hidInfoProps},
    // HID 信息值
    {{ATT_BT_UUID_SIZE, hidInfoUUID}, GATT_PERMIT_ENCRYPT_READ, 0, (uint8_t *)hidInfo},
    
    // HID 控制点声明
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &hidControlPointProps},
    // HID 控制点值
    {{ATT_BT_UUID_SIZE, hidControlPointUUID}, GATT_PERMIT_ENCRYPT_WRITE, 0, &hidControlPoint},
    
    // HID 协议模式声明
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &hidProtocolModeProps},
    // HID 协议模式值
    {{ATT_BT_UUID_SIZE, hidProtocolModeUUID}, GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0, &hidProtocolMode},
    
    // HID 报告映射声明
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &hidReportMapProps},
    // HID 报告映射值
    {{ATT_BT_UUID_SIZE, hidReportMapUUID}, GATT_PERMIT_ENCRYPT_READ, 0, (uint8_t *)hidReportMap},
    // 外部报告引用描述符
    {{ATT_BT_UUID_SIZE, extReportRefUUID}, GATT_PERMIT_READ, 0, hidExtReportRefDesc},
    
    /* ===== 键盘输入报告 ===== */
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &hidReportKeyInProps},
    {{ATT_BT_UUID_SIZE, hidReportUUID}, GATT_PERMIT_ENCRYPT_READ, 0, &hidReportKeyIn},
    {{ATT_BT_UUID_SIZE, clientCharCfgUUID}, GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0, (uint8_t *)&hidReportKeyInClientCharCfg},
    {{ATT_BT_UUID_SIZE, reportRefUUID}, GATT_PERMIT_READ, 0, hidReportRefKeyIn},
    
    /* ===== 键盘 LED 输出报告 ===== */
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &hidReportLedOutProps},
    {{ATT_BT_UUID_SIZE, hidReportUUID}, GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0, &hidReportLedOut},
    {{ATT_BT_UUID_SIZE, reportRefUUID}, GATT_PERMIT_READ, 0, hidReportRefLedOut},
    
    /* ===== 鼠标输入报告 ===== */
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &hidReportMouseInProps},
    {{ATT_BT_UUID_SIZE, hidReportUUID}, GATT_PERMIT_ENCRYPT_READ, 0, &hidReportMouseIn},
    {{ATT_BT_UUID_SIZE, clientCharCfgUUID}, GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0, (uint8_t *)&hidReportMouseInClientCharCfg},
    {{ATT_BT_UUID_SIZE, reportRefUUID}, GATT_PERMIT_READ, 0, hidReportRefMouseIn},
    
    /* ===== 多媒体输入报告 ===== */
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &hidReportConsumerInProps},
    {{ATT_BT_UUID_SIZE, hidReportUUID}, GATT_PERMIT_ENCRYPT_READ, 0, &hidReportConsumerIn},
    {{ATT_BT_UUID_SIZE, clientCharCfgUUID}, GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0, (uint8_t *)&hidReportConsumerInClientCharCfg},
    {{ATT_BT_UUID_SIZE, reportRefUUID}, GATT_PERMIT_READ, 0, hidReportRefConsumerIn},
    
    /* ===== Boot 键盘输入报告 ===== */
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &hidReportBootKeyInProps},
    {{ATT_BT_UUID_SIZE, hidBootKeyInputUUID}, GATT_PERMIT_ENCRYPT_READ, 0, &hidReportBootKeyIn},
    {{ATT_BT_UUID_SIZE, clientCharCfgUUID}, GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0, (uint8_t *)&hidReportBootKeyInClientCharCfg},
    
    /* ===== Boot 键盘输出报告 ===== */
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &hidReportBootKeyOutProps},
    {{ATT_BT_UUID_SIZE, hidBootKeyOutputUUID}, GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE, 0, &hidReportBootKeyOut},
    
};

/* ==================== 属性索引枚举 ==================== */

enum {
    HID_SERVICE_IDX = 0,
    HID_INCLUDED_SERVICE_IDX,
    HID_INFO_DECL_IDX,
    HID_INFO_IDX,
    HID_CONTROL_POINT_DECL_IDX,
    HID_CONTROL_POINT_IDX,
    HID_PROTOCOL_MODE_DECL_IDX,
    HID_PROTOCOL_MODE_IDX,
    HID_REPORT_MAP_DECL_IDX,
    HID_REPORT_MAP_IDX,
    HID_EXT_REPORT_REF_IDX,
    
    // 键盘输入
    HID_REPORT_KEY_IN_DECL_IDX,
    HID_REPORT_KEY_IN_IDX,
    HID_REPORT_KEY_IN_CCCD_IDX,
    HID_REPORT_REF_KEY_IN_IDX,
    
    // LED 输出
    HID_REPORT_LED_OUT_DECL_IDX,
    HID_REPORT_LED_OUT_IDX,
    HID_REPORT_REF_LED_OUT_IDX,
    
    // 鼠标输入
    HID_REPORT_MOUSE_IN_DECL_IDX,
    HID_REPORT_MOUSE_IN_IDX,
    HID_REPORT_MOUSE_IN_CCCD_IDX,
    HID_REPORT_REF_MOUSE_IN_IDX,
    
    // 多媒体输入
    HID_REPORT_CONSUMER_IN_DECL_IDX,
    HID_REPORT_CONSUMER_IN_IDX,
    HID_REPORT_CONSUMER_IN_CCCD_IDX,
    HID_REPORT_REF_CONSUMER_IN_IDX,
    
    // Boot 键盘输入
    HID_BOOT_KEY_IN_DECL_IDX,
    HID_BOOT_KEY_IN_IDX,
    HID_BOOT_KEY_IN_CCCD_IDX,
    
    // Boot 键盘输出
    HID_BOOT_KEY_OUT_DECL_IDX,
    HID_BOOT_KEY_OUT_IDX,
    
};

/* ==================== 服务回调 ==================== */

gattServiceCBs_t hidKbdMouseCBs = {
    HidDev_ReadAttrCB,
    HidDev_WriteAttrCB,
    NULL
};

/* ==================== 函数实现 ==================== */

bStatus_t HidKbdMouse_AddService(void)
{
    uint8_t status = SUCCESS;
    
    // 初始化客户端特性配置
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportKeyInClientCharCfg);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportMouseInClientCharCfg);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportConsumerInClientCharCfg);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportBootKeyInClientCharCfg);
    
    // 注册 GATT 属性
    status = GATTServApp_RegisterService(hidAttrTbl, GATT_NUM_ATTRS(hidAttrTbl),
                                          GATT_MAX_ENCRYPT_KEY_SIZE, &hidKbdMouseCBs);
    
    // 设置包含服务
    Batt_GetParameter(BATT_PARAM_SERVICE_HANDLE,
                      &GATT_INCLUDED_HANDLE(hidAttrTbl, HID_INCLUDED_SERVICE_IDX));
    
    // 构建报告映射表
    uint8_t idx = 0;
    
    // 键盘输入报告 (Report Mode)
    hidRptMap[idx].id = hidReportRefKeyIn[0];
    hidRptMap[idx].type = hidReportRefKeyIn[1];
    hidRptMap[idx].handle = hidAttrTbl[HID_REPORT_KEY_IN_IDX].handle;
    hidRptMap[idx].cccdHandle = hidAttrTbl[HID_REPORT_KEY_IN_CCCD_IDX].handle;
    hidRptMap[idx].mode = HID_PROTOCOL_MODE_REPORT;
    idx++;
    
    // LED 输出报告
    hidRptMap[idx].id = hidReportRefLedOut[0];
    hidRptMap[idx].type = hidReportRefLedOut[1];
    hidRptMap[idx].handle = hidAttrTbl[HID_REPORT_LED_OUT_IDX].handle;
    hidRptMap[idx].cccdHandle = 0;
    hidRptMap[idx].mode = HID_PROTOCOL_MODE_REPORT;
    idx++;
    
    // 鼠标输入报告
    hidRptMap[idx].id = hidReportRefMouseIn[0];
    hidRptMap[idx].type = hidReportRefMouseIn[1];
    hidRptMap[idx].handle = hidAttrTbl[HID_REPORT_MOUSE_IN_IDX].handle;
    hidRptMap[idx].cccdHandle = hidAttrTbl[HID_REPORT_MOUSE_IN_CCCD_IDX].handle;
    hidRptMap[idx].mode = HID_PROTOCOL_MODE_REPORT;
    idx++;
    
    // 多媒体输入报告
    hidRptMap[idx].id = hidReportRefConsumerIn[0];
    hidRptMap[idx].type = hidReportRefConsumerIn[1];
    hidRptMap[idx].handle = hidAttrTbl[HID_REPORT_CONSUMER_IN_IDX].handle;
    hidRptMap[idx].cccdHandle = hidAttrTbl[HID_REPORT_CONSUMER_IN_CCCD_IDX].handle;
    hidRptMap[idx].mode = HID_PROTOCOL_MODE_REPORT;
    idx++;
    
    // Boot 键盘输入报告
    hidRptMap[idx].id = hidReportRefKeyIn[0];
    hidRptMap[idx].type = hidReportRefKeyIn[1];
    hidRptMap[idx].handle = hidAttrTbl[HID_BOOT_KEY_IN_IDX].handle;
    hidRptMap[idx].cccdHandle = hidAttrTbl[HID_BOOT_KEY_IN_CCCD_IDX].handle;
    hidRptMap[idx].mode = HID_PROTOCOL_MODE_BOOT;
    idx++;
    
    // Boot 键盘输出报告
    hidRptMap[idx].id = hidReportRefLedOut[0];
    hidRptMap[idx].type = hidReportRefLedOut[1];
    hidRptMap[idx].handle = hidAttrTbl[HID_BOOT_KEY_OUT_IDX].handle;
    hidRptMap[idx].cccdHandle = 0;
    hidRptMap[idx].mode = HID_PROTOCOL_MODE_BOOT;
    idx++;
    
    // 电池报告
    Batt_GetParameter(BATT_PARAM_BATT_LEVEL_IN_REPORT, &(hidRptMap[idx]));
    idx++;
    
    // 注册报告映射表
    HidDev_RegisterReports(HID_NUM_REPORTS, hidRptMap);
    
    return status;
}

uint8_t HidKbdMouse_SetParameter(uint8_t id, uint8_t type, uint16_t uuid,
                                  uint8_t len, void *pValue)
{
    bStatus_t ret = SUCCESS;
    
    switch (uuid) {
        case REPORT_UUID:
            if (type == HID_REPORT_TYPE_OUTPUT) {
                if (len == 1) {
                    hidReportLedOut = *((uint8_t *)pValue);
                } else {
                    ret = ATT_ERR_INVALID_VALUE_SIZE;
                }
            }
            else {
                ret = ATT_ERR_ATTR_NOT_FOUND;
            }
            break;
            
        case BOOT_KEY_OUTPUT_UUID:
            if (len == 1) {
                hidReportBootKeyOut = *((uint8_t *)pValue);
            } else {
                ret = ATT_ERR_INVALID_VALUE_SIZE;
            }
            break;
            
        default:
            break;
    }
    
    return ret;
}

uint8_t HidKbdMouse_GetParameter(uint8_t id, uint8_t type, uint16_t uuid,
                                  uint16_t *pLen, void *pValue)
{
    switch (uuid) {
        case REPORT_UUID:
            if (type == HID_REPORT_TYPE_OUTPUT) {
                *((uint8_t *)pValue) = hidReportLedOut;
                *pLen = 1;
            }
            else {
                *pLen = 0;
            }
            break;
            
        case BOOT_KEY_OUTPUT_UUID:
            *((uint8_t *)pValue) = hidReportBootKeyOut;
            *pLen = 1;
            break;
            
        default:
            *pLen = 0;
            break;
    }
    
    return SUCCESS;
}
