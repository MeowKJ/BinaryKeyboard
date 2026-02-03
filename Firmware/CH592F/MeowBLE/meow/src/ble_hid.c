/********************************** (C) COPYRIGHT *******************************
 * File Name          : ble_hid.c
 * Author             : Custom Keyboard Library
 * Version            : V1.0
 * Date               : 2024/11/07
 * Description        : 蓝牙 HID 层封装实现
 *******************************************************************************/

#include "ble_hid.h"
#include "ble_hid_service.h"
#include "kbd_mode_config.h"
#include "battservice.h"
#include "devinfoservice.h"
#include "scanparamservice.h"
#include "debug.h"
#include <string.h>

/* ==================== 常量定义 ==================== */

// 参数更新延迟（625us 单位）
#define PARAM_UPDATE_DELAY 12800

// PHY 更新延迟
#define PHY_UPDATE_DELAY 1600

/* ==================== 全局变量 ==================== */

uint8_t bleHidTaskId = INVALID_TASK_ID;

/* ==================== 私有变量 ==================== */

static gapRole_States_t g_ble_state = GAPROLE_INIT;
static uint16_t g_conn_handle = GAP_CONNHANDLE_INIT;
static ble_hid_callbacks_t *g_pCallbacks = NULL;
static uint8_t g_keyboard_leds = 0;
static bool g_ble_enabled = true;

static const char *TAG = "BLE_HID";

// HID 配置
static hidDevCfg_t g_hidDevCfg = {
    BLE_HID_IDLE_TIMEOUT,  // 空闲超时
    HID_KBD_FLAGS          // HID 特性标志
};

// 广播数据
static uint8_t g_advertData[] = {
    // Flags
    0x02,
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    // Appearance (键盘)
    0x03,
    GAP_ADTYPE_APPEARANCE,
    LO_UINT16 (GAP_APPEARE_HID_KEYBOARD),
    HI_UINT16 (GAP_APPEARE_HID_KEYBOARD),
};

// 扫描响应数据
static uint8_t g_scanRspData[] = {
    // 设备名称
    BLE_DEVICE_NAME_LEN + 1, GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'D', 'u', 'a', 'l', 'M', 'o', 'd', 'e', ' ',
    'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd',

    // 连接间隔范围
    0x05, GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16 (BLE_CONN_INT_MIN),
    HI_UINT16 (BLE_CONN_INT_MIN),
    LO_UINT16 (BLE_CONN_INT_MAX),
    HI_UINT16 (BLE_CONN_INT_MAX),

    // 服务 UUID
    0x05, GAP_ADTYPE_16BIT_MORE,
    LO_UINT16 (HID_SERV_UUID),
    HI_UINT16 (HID_SERV_UUID),
    LO_UINT16 (BATT_SERV_UUID),
    HI_UINT16 (BATT_SERV_UUID),
};

// 设备名称
static const uint8_t g_attDeviceName[GAP_DEVICE_NAME_LEN] = BLE_DEVICE_NAME;

/* ==================== 私有函数声明 ==================== */

static void BLE_HID_ProcessTMOSMsg (tmos_event_hdr_t *pMsg);
static uint8_t BLE_HID_RptCallback (uint8_t id, uint8_t type, uint16_t uuid,
                                    uint8_t oper, uint16_t *pLen, uint8_t *pData);
static void BLE_HID_EvtCallback (uint8_t evt);
static void BLE_HID_StateCallback (gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void BLE_HID_BattCallback (uint8_t event);
static void BLE_HID_ScanParamCallback (uint8_t event);

// HID 设备回调
static hidDevCB_t g_hidDevCallbacks = {
    BLE_HID_RptCallback,   // 报告回调
    BLE_HID_EvtCallback,   // 事件回调
    NULL,                  // Passcode 回调
    BLE_HID_StateCallback  // 状态回调
};

/* ==================== 初始化实现 ==================== */

int BLE_HID_Init (ble_hid_callbacks_t *pCBs) {
    g_pCallbacks = pCBs;

    // 注册 TMOS 任务
    bleHidTaskId = TMOS_ProcessEventRegister (BLE_HID_ProcessEvent);

    // 设置 GAP 角色参数
    {
        uint8_t enable = FALSE;  // 初始不广播
        GAPRole_SetParameter (GAPROLE_ADVERT_ENABLED, sizeof (uint8_t), &enable);
        GAPRole_SetParameter (GAPROLE_ADVERT_DATA, sizeof (g_advertData), g_advertData);
        GAPRole_SetParameter (GAPROLE_SCAN_RSP_DATA, sizeof (g_scanRspData), g_scanRspData);
    }

    // 设置 GAP 特性
    GGS_SetParameter (GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, (void *)g_attDeviceName);

    // 设置连接参数
    {
        gapPeriConnectParams_t connParams;
        connParams.intervalMin = BLE_CONN_INT_MIN;
        connParams.intervalMax = BLE_CONN_INT_MAX;
        connParams.latency = BLE_SLAVE_LATENCY;
        connParams.timeout = BLE_CONN_TIMEOUT;
        GGS_SetParameter (GGS_PERI_CONN_PARAM_ATT, sizeof (gapPeriConnectParams_t), &connParams);
    }

    // 设置 Bond Manager 参数
    {
        uint32_t passcode = 0;
        uint8_t pairMode = BLE_PAIRING_MODE;
        uint8_t mitm = BLE_MITM_MODE;
        uint8_t ioCap = BLE_IO_CAPABILITIES;
        uint8_t bonding = BLE_BONDING_MODE;

        GAPBondMgr_SetParameter (GAPBOND_PERI_DEFAULT_PASSCODE, sizeof (uint32_t), &passcode);
        GAPBondMgr_SetParameter (GAPBOND_PERI_PAIRING_MODE, sizeof (uint8_t), &pairMode);
        GAPBondMgr_SetParameter (GAPBOND_PERI_MITM_PROTECTION, sizeof (uint8_t), &mitm);
        GAPBondMgr_SetParameter (GAPBOND_PERI_IO_CAPABILITIES, sizeof (uint8_t), &ioCap);
        GAPBondMgr_SetParameter (GAPBOND_PERI_BONDING_ENABLED, sizeof (uint8_t), &bonding);
    }

    // 设置电池参数
    {
        uint8_t critical = 6;
        Batt_SetParameter (BATT_PARAM_CRITICAL_LEVEL, sizeof (uint8_t), &critical);
    }

    // 添加 HID 服务
    HidKbdMouse_AddService();

    // 注册 HID 设备回调
    HidDev_Register (&g_hidDevCfg, &g_hidDevCallbacks);

    HidDev_Init();

    // 注册电池服务回调
    Batt_Register (BLE_HID_BattCallback);

    // 注册扫描参数服务回调
    ScanParam_Register (BLE_HID_ScanParamCallback);

    // 启动设备
    tmos_set_event (bleHidTaskId, BLE_HID_START_DEVICE_EVT);

    LOG_I (TAG, "Init done");

    return 0;
}

/* ==================== TMOS 事件处理 ==================== */

uint16_t BLE_HID_ProcessEvent (uint8_t task_id, uint16_t events) {
    if (events & SYS_EVENT_MSG) {
        uint8_t *pMsg = tmos_msg_receive (bleHidTaskId);
        if (pMsg) {
            BLE_HID_ProcessTMOSMsg ((tmos_event_hdr_t *)pMsg);
            tmos_msg_deallocate (pMsg);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & BLE_HID_START_DEVICE_EVT) {
        // 启动设备不再需要特殊处理，HidDev_Init 已完成
        return (events ^ BLE_HID_START_DEVICE_EVT);
    }

    if (events & BLE_HID_PARAM_UPDATE_EVT) {
        // 请求连接参数更新
        GAPRole_PeripheralConnParamUpdateReq (g_conn_handle,
                                              BLE_CONN_INT_MIN,
                                              BLE_CONN_INT_MAX,
                                              BLE_SLAVE_LATENCY,
                                              BLE_CONN_TIMEOUT,
                                              bleHidTaskId);
        return (events ^ BLE_HID_PARAM_UPDATE_EVT);
    }

    if (events & BLE_HID_PHY_UPDATE_EVT) {
        // 请求 PHY 更新到 2M
        GAPRole_UpdatePHY (g_conn_handle, 0,
                           GAP_PHY_BIT_LE_2M, GAP_PHY_BIT_LE_2M, 0);
        return (events ^ BLE_HID_PHY_UPDATE_EVT);
    }

    return 0;
}

static void BLE_HID_ProcessTMOSMsg (tmos_event_hdr_t *pMsg) {
    switch (pMsg->event) {
    default:
        break;
    }
}

/* ==================== 连接管理实现 ==================== */

int BLE_HID_StartAdvertising (void) {
    if (!g_ble_enabled) {
        return -1;
    }

    // 设置广播参数
    GAP_SetParamValue (TGAP_DISC_ADV_INT_MIN, BLE_ADV_INT_MIN);
    GAP_SetParamValue (TGAP_DISC_ADV_INT_MAX, BLE_ADV_INT_MAX);
    GAP_SetParamValue (TGAP_LIM_ADV_TIMEOUT, BLE_ADV_TIMEOUT);

    // 开始广播
    uint8_t enable = TRUE;
    GAPRole_SetParameter (GAPROLE_ADVERT_ENABLED, sizeof (uint8_t), &enable);

    LOG_I (TAG, "Start advertising");
    return 0;
}

int BLE_HID_StopAdvertising (void) {
    uint8_t enable = FALSE;
    GAPRole_SetParameter (GAPROLE_ADVERT_ENABLED, sizeof (uint8_t), &enable);

    LOG_I (TAG, "Stop advertising");
    return 0;
}

int BLE_HID_Disconnect (void) {
    if (g_conn_handle != GAP_CONNHANDLE_INIT) {
        GAPRole_TerminateLink (g_conn_handle);
        LOG_I (TAG, "Disconnect");
    }
    return 0;
}

bool BLE_HID_IsConnected (void) {
    return ((g_ble_state & GAPROLE_STATE_ADV_MASK) == GAPROLE_CONNECTED);
}

gapRole_States_t BLE_HID_GetState (void) {
    return g_ble_state;
}

int BLE_HID_ClearBonds (void) {
    HidDev_SetParameter (HIDDEV_ERASE_ALLBONDS, 0, NULL);
    LOG_I (TAG, "Clear bonds");
    return 0;
}

uint8_t BLE_HID_GetBondCount (void) {
    uint8_t count = 0;
    GAPBondMgr_GetParameter (GAPBOND_BOND_COUNT, &count);
    return count;
}

/* ==================== HID 报告发送实现 ==================== */

int BLE_HID_SendKeyboardReport (uint8_t modifier, uint8_t *keys, uint8_t key_count) {
    if (!BLE_HID_IsConnected()) {
        return -1;
    }

    uint8_t buf[8] = {0};
    buf[0] = modifier;
    buf[1] = 0;  // Reserved

    uint8_t count = (key_count > 6) ? 6 : key_count;
    if (keys && count > 0) {
        memcpy (&buf[2], keys, count);
    }

    return HidDev_Report (HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, 8, buf);
}

int BLE_HID_SendMouseReport (uint8_t buttons, int8_t x, int8_t y, int8_t wheel) {
    if (!BLE_HID_IsConnected()) {
        return -1;
    }

    uint8_t buf[4];
    buf[0] = buttons;
    buf[1] = (uint8_t)x;
    buf[2] = (uint8_t)y;
    buf[3] = (uint8_t)wheel;

    return HidDev_Report (HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, 4, buf);
}

int BLE_HID_SendConsumerReport (uint16_t key) {
    if (!BLE_HID_IsConnected()) {
        return -1;
    }

    uint8_t buf[2];
    buf[0] = LO_UINT16 (key);
    buf[1] = HI_UINT16 (key);

    return HidDev_Report (HID_RPT_ID_CONSUMER_IN, HID_REPORT_TYPE_INPUT, 2, buf);
}

uint8_t BLE_HID_GetKeyboardLEDs (void) {
    return g_keyboard_leds;
}

/* ==================== 电源管理实现 ==================== */

void BLE_HID_Enable (void) {
    g_ble_enabled = true;
}

void BLE_HID_Disable (void) {
    g_ble_enabled = false;
    BLE_HID_StopAdvertising();
    if (BLE_HID_IsConnected()) {
        BLE_HID_Disconnect();
    }
}

/* ==================== 回调函数实现 ==================== */

static uint8_t BLE_HID_RptCallback (uint8_t id, uint8_t type, uint16_t uuid,
                                    uint8_t oper, uint16_t *pLen, uint8_t *pData) {
    uint8_t status = SUCCESS;

    if (oper == HID_DEV_OPER_WRITE) {
        // 写操作 - 处理 LED 输出报告
        if (uuid == REPORT_UUID && type == HID_REPORT_TYPE_OUTPUT) {
            if (*pLen >= 1) {
                g_keyboard_leds = pData[0];
                LOG_D (TAG, "LED=0x%02X", g_keyboard_leds);

                // 调用回调
                if (g_pCallbacks && g_pCallbacks->onLedReport) {
                    g_pCallbacks->onLedReport (g_keyboard_leds);
                }
            }
        }

        // 保存参数
        if (status == SUCCESS) {
            status = HidKbdMouse_SetParameter (id, type, uuid, *pLen, pData);
        }
    } else if (oper == HID_DEV_OPER_READ) {
        // 读操作
        status = HidKbdMouse_GetParameter (id, type, uuid, pLen, pData);
    } else if (oper == HID_DEV_OPER_ENABLE) {
        // 通知已启用
        LOG_D (TAG, "Report %d enabled", id);
    }

    return status;
}

static void BLE_HID_EvtCallback (uint8_t evt) {
    switch (evt) {
    case HID_DEV_SUSPEND_EVT:
        LOG_I (TAG, "Suspend");
        break;

    case HID_DEV_EXIT_SUSPEND_EVT:
        LOG_I (TAG, "Exit suspend");
        break;

    case HID_DEV_SET_BOOT_EVT:
        LOG_I (TAG, "Boot mode");
        break;

    case HID_DEV_SET_REPORT_EVT:
        LOG_I (TAG, "Report mode");
        break;

    default:
        break;
    }
}

static void BLE_HID_StateCallback (gapRole_States_t newState, gapRoleEvent_t *pEvent) {
    g_ble_state = newState;

    switch (newState & GAPROLE_STATE_ADV_MASK) {
    case GAPROLE_STARTED: {
        uint8_t ownAddr[6];
        GAPRole_GetParameter (GAPROLE_BD_ADDR, ownAddr);
        GAP_ConfigDeviceAddr (ADDRTYPE_STATIC, ownAddr);
        LOG_I (TAG, "Started, Addr=%02X:%02X:%02X:%02X:%02X:%02X",
               ownAddr[5], ownAddr[4], ownAddr[3],
               ownAddr[2], ownAddr[1], ownAddr[0]);
    } break;

    case GAPROLE_ADVERTISING:
        if (pEvent->gap.opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT) {
            LOG_I (TAG, "Advertising");
        }
        break;

    case GAPROLE_CONNECTED:
        if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT) {
            gapEstLinkReqEvent_t *event = (gapEstLinkReqEvent_t *)pEvent;
            g_conn_handle = event->connectionHandle;

            // 延迟请求参数更新
            tmos_start_task (bleHidTaskId, BLE_HID_PARAM_UPDATE_EVT, PARAM_UPDATE_DELAY);

            LOG_I (TAG, "Connected, handle=%d", g_conn_handle);
        }
        break;

    case GAPROLE_CONNECTED_ADV:
        if (pEvent->gap.opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT) {
            LOG_I (TAG, "Connected advertising");
        }
        break;

    case GAPROLE_WAITING:
        g_conn_handle = GAP_CONNHANDLE_INIT;

        if (pEvent->gap.opcode == GAP_END_DISCOVERABLE_DONE_EVENT) {
            LOG_I (TAG, "Advertising timeout");
        } else if (pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT) {
            LOG_I (TAG, "Disconnected, reason=%02X",
                   pEvent->linkTerminate.reason);
        }

        // 重新开始广播
        {
            uint8_t enable = TRUE;
            GAPRole_SetParameter (GAPROLE_ADVERT_ENABLED, sizeof (uint8_t), &enable);
        }
        break;

    case GAPROLE_ERROR:
        LOG_W (TAG, "Error %02X", pEvent->gap.opcode);
        break;

    default:
        break;
    }

    // 调用外部回调
    if (g_pCallbacks && g_pCallbacks->onStateChange) {
        g_pCallbacks->onStateChange (newState);
    }
}

static void BLE_HID_BattCallback (uint8_t event) {
    if (event == BATT_LEVEL_NOTI_ENABLED) {
        LOG_I (TAG, "Battery notification enabled");
    } else if (event == BATT_LEVEL_NOTI_DISABLED) {
        LOG_I (TAG, "Battery notification disabled");
    }
}

static void BLE_HID_ScanParamCallback (uint8_t event) {
    // 扫描参数更新
}
