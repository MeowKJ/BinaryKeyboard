/********************************** (C) COPYRIGHT *******************************
 * File Name          : ble_hid.c
 * Author             : Custom Keyboard Library
 * Version            : V1.0
 * Date               : 2024/11/07
 * Description        : 闂佽棄鍟换鍕櫠?HID 闁诲繒鍋涢崐鎼佹儍濠靛牊鍟戦柛娑卞幘閺変粙鏌?
 *******************************************************************************/

#include "ble_hid.h"
#include "ble_hid_service.h"
#include "kbd_mode_config.h"
#include "battservice.h"
#include "devinfoservice.h"
#include "scanparamservice.h"
#include "kbd_log.h"
#include "debug.h"
#include <string.h>

/* ==================== 闁汇埄鍨遍幃鍌炲闯閾忓厜鍋撶憴鍕叝缂?==================== */

// 闂佸憡鐟ラ崐褰掑汲閻旂厧鍗抽悗娑櫳戦悡鈧悗鐐瑰€栭崕鑲╂崲濠婂牊鏅?25us 闂佸憡顨嗗ú鎴犵礊閸涘瓨鏅?
#define PARAM_UPDATE_DELAY 12800

// PHY 闂佸搫娲ら悺銊╁蓟婵犲偆鍤堥柟顖涘缁?
#define PHY_UPDATE_DELAY 1600

// HID notify readiness check delay (ms)
#define HID_READY_CHECK_DELAY_MS 20000

#define SECURITY_REQ_DELAY_MS       4800
#define SECURITY_REQ_RETRY_DELAY_MS 4800
#define SECURITY_REQ_MAX_RETRY      3
#ifndef BLE_AUTO_SECURITY_REQ
#define BLE_AUTO_SECURITY_REQ 0
#endif

// Extra BLE diagnostic opcodes
#define BLE_DIAG_OP_HID_NOTIFY_ENABLED 0xD0
#define BLE_DIAG_OP_HID_READY_TIMEOUT  0xD1
#define BLE_DIAG_OP_HID_NOTIFY_TOGGLE  0xD2
#define BLE_DIAG_OP_SECURITY_REQ_CALL  0xBB
#define BLE_DIAG_OP_SECURITY_REQ_SKIP  0xBC
#define BLE_DIAG_OP_STATE_CB_ENTER     0xC8
#define BLE_DIAG_OP_STATE_CB_EXIT      0xC9

/* 安全请求事件内不打 BLE 诊断日志，降低 SECURITY_REQUEST 后卡死风险（见 FAQ#6）；设为 1 可恢复调试用 */
#ifndef BLE_SECURITY_REQ_DIAG_LOG
#define BLE_SECURITY_REQ_DIAG_LOG 1
#endif
/* 状态回调内不打 BLE 诊断日志，降低连接建立后卡死风险（见 FAQ#6）；设为 1 可恢复调试用 */
#ifndef BLE_STATE_CB_DIAG_LOG
#define BLE_STATE_CB_DIAG_LOG 1
#endif

/* ==================== 闂佺绻堥崝宀勬儑椤掑嫬鐭楁慨妞诲亾闁?==================== */

uint8_t bleHidTaskId = INVALID_TASK_ID;

/* ==================== 缂備礁顦悞锕€锕㈡笟鈧畷锝咁潨閳ь剟宕?==================== */

static gapRole_States_t g_ble_state = GAPROLE_INIT;
static uint16_t g_conn_handle = GAP_CONNHANDLE_INIT;
static ble_hid_callbacks_t *g_pCallbacks = NULL;
/* 延后到主循环执行的状态回调，避免在 BLE 栈上下文中调用应用层（见 FAQ#6） */
static volatile gapRole_States_t s_pending_state = GAPROLE_INIT;
static volatile uint8_t s_state_pending = 0;
static uint8_t g_keyboard_leds = 0;
static bool g_ble_enabled = true;
static bool g_adv_manual_requested = false;
static bool g_hid_notify_ready = false;
static uint8_t g_adv_filter_policy = GAP_FILTER_POLICY_ALL;
static uint8_t g_security_req_retry = 0;

static const char *TAG = "BLE_HID";

static inline void BLE_HID_DiagLog (uint8_t state, uint8_t opcode, uint8_t reason, uint16_t value) {
    if (!g_ble_enabled) {
        return;
    }
    KBD_Log_BleDiagEvent (state, opcode, reason, value);
}

// HID 闂備焦婢樼粔鍫曟偪?
static hidDevCfg_t g_hidDevCfg = {
    BLE_HID_IDLE_TIMEOUT,  // 缂備礁鏈钘壩涢悾灞炬儱闁告侗鍠楅ˇ?
    HID_KBD_FLAGS          // HID 闂佺粯顨夐～澶愬焵椤戭剙瀚崹濂哥叓?
};

// 濡ょ姷鍋炵€笛囧箰闁秴鏋侀柣妤€鐗嗙粊?
static uint8_t g_advertData[] = {
    // Flags
    0x02,
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    // Appearance (闂備焦顑欓崰姘?
    0x03,
    GAP_ADTYPE_APPEARANCE,
    LO_UINT16 (GAP_APPEARE_HID_KEYBOARD),
    HI_UINT16 (GAP_APPEARE_HID_KEYBOARD),

    // Local Name
    BLE_DEVICE_NAME_LEN + 1, GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'B', 'i', 'n', 'a', 'r', 'y', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd', ' ', 'B', 'T',
};

// 闂佽顔栭崑鍛嚕閸ф浼犵€广儱鎳愮€瑰鏌℃担鍝勵暭鐎?
static uint8_t g_scanRspData[] = {
    // 闁荤姳鐒﹂崕鎶剿囬鍕Е鐎广儱娉?
    BLE_DEVICE_NAME_LEN + 1, GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'B', 'i', 'n', 'a', 'r', 'y', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd', ' ', 'B', 'T',

    // 闁哄鏅濋崑鐐垫暜閹绢喗鈷掗柟缁㈠枟椤撻箖鏌ら悡搴℃殭婵?
    0x05, GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16 (BLE_CONN_INT_MIN),
    HI_UINT16 (BLE_CONN_INT_MIN),
    LO_UINT16 (BLE_CONN_INT_MAX),
    HI_UINT16 (BLE_CONN_INT_MAX),

    // 闂佸搫鐗嗙粔瀛樻叏?UUID
    0x05, GAP_ADTYPE_16BIT_MORE,
    LO_UINT16 (HID_SERV_UUID),
    HI_UINT16 (HID_SERV_UUID),
    LO_UINT16 (BATT_SERV_UUID),
    HI_UINT16 (BATT_SERV_UUID),
};

// 闁荤姳鐒﹂崕鎶剿囬鍕Е鐎广儱娉?
static const uint8_t g_attDeviceName[GAP_DEVICE_NAME_LEN] = BLE_DEVICE_NAME;

/* ==================== 缂備礁顦悞锕€锕㈡笟鈧畷娆撴嚍閵夛附顔嶆繝銏犳贡婢ф危?==================== */

static void BLE_HID_ProcessTMOSMsg (tmos_event_hdr_t *pMsg);
static uint8_t BLE_HID_RptCallback (uint8_t id, uint8_t type, uint16_t uuid,
                                    uint8_t oper, uint16_t *pLen, uint8_t *pData);
static void BLE_HID_EvtCallback (uint8_t evt);
static void BLE_HID_StateCallback (gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void BLE_HID_BattCallback (uint8_t event);
static void BLE_HID_ScanParamCallback (uint8_t event);
static uint8_t BLE_HID_MapStateToLog (gapRole_States_t state);

// HID 闁荤姳鐒﹂崕鎶剿囬鍕倞闁绘劕澧庡▓?
static hidDevCB_t g_hidDevCallbacks = {
    BLE_HID_RptCallback,   // 闂佺缈伴崕閬嶅箟閿熺姴鐐婇柣鎰濞?
    BLE_HID_EvtCallback,   // 婵炲瓨绮岄鍕枎閵忋倕鐐婇柣鎰濞?
    NULL,                  // Passcode 闂佹悶鍎抽崑鐘绘儍?
    BLE_HID_StateCallback  // 闂佺粯顭堥崺鏍焵椤戣法顦︽繛鍙夌墱閹?
};

/* ==================== 闂佸憡甯楃换鍌烇綖閹版澘绀岄柡宓懏婢栭梺?==================== */

int BLE_HID_Init (ble_hid_callbacks_t *pCBs) {
    bStatus_t status = SUCCESS;
    g_pCallbacks = pCBs;

    // 濠电偛顦崝宀勫船?TMOS 婵炲濮鹃褎鎱?
    bleHidTaskId = TMOS_ProcessEventRegister (BLE_HID_ProcessEvent);

    // 闁荤姳绀佹晶浠嬫偪?GAP 闁荤喐鐟︾敮鐔哥珶婵犲洤鐭楅柛灞剧⊕濞?
    {
        uint8_t enable = FALSE;  // 闂佸憡甯楃换鍌烇綖閹邦厾鈻旂€广儱鎳愰悾顓㈡煙?
        GAPRole_SetParameter (GAPROLE_ADVERT_ENABLED, sizeof (uint8_t), &enable);
        GAPRole_SetParameter (GAPROLE_ADV_FILTER_POLICY, sizeof (uint8_t), &g_adv_filter_policy);
        GAPRole_SetParameter (GAPROLE_ADVERT_DATA, sizeof (g_advertData), g_advertData);
        GAPRole_SetParameter (GAPROLE_SCAN_RSP_DATA, sizeof (g_scanRspData), g_scanRspData);
    }

    // 闁荤姳绀佹晶浠嬫偪?GAP 闂佺粯顨夐～澶愬焵?
    GGS_SetParameter (GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, (void *)g_attDeviceName);

    // 闁荤姳绀佹晶浠嬫偪閸℃ɑ浜ら柣鎰綑婢跺秹鏌涘▎蹇撯偓褰掑汲?
    {
        gapPeriConnectParams_t connParams;
        connParams.intervalMin = BLE_CONN_INT_MIN;
        connParams.intervalMax = BLE_CONN_INT_MAX;
        connParams.latency = BLE_SLAVE_LATENCY;
        connParams.timeout = BLE_CONN_TIMEOUT;
        GGS_SetParameter (GGS_PERI_CONN_PARAM_ATT, sizeof (gapPeriConnectParams_t), &connParams);
    }

    // 闁荤姳绀佹晶浠嬫偪?Bond Manager 闂佸憡鐟ラ崐褰掑汲?
    {
        uint32_t passcode = 0;
        uint8_t pairMode = BLE_PAIRING_MODE;
        uint8_t mitm = BLE_MITM_MODE;
        uint8_t ioCap = BLE_IO_CAPABILITIES;
#if (defined(BLE_SNV_PERSIST_ENABLE)) && (BLE_SNV_PERSIST_ENABLE == TRUE)
        uint8_t bonding = BLE_BONDING_MODE;
#else
        /* Keep encrypted link, but do not trigger bond-save path while SNV persistence is disabled. */
        uint8_t bonding = FALSE;
#endif
        /* Minimize key distribution to reduce repeated SNV writes during pairing.
         * Peripheral only distributes its encryption key; do not request master keys. */
        uint8_t periKeyDist = GAPBOND_KEYDIST_SENCKEY;
        uint8_t centKeyDist = 0x00;

        GAPBondMgr_SetParameter (GAPBOND_PERI_DEFAULT_PASSCODE, sizeof (uint32_t), &passcode);
        GAPBondMgr_SetParameter (GAPBOND_PERI_PAIRING_MODE, sizeof (uint8_t), &pairMode);
        GAPBondMgr_SetParameter (GAPBOND_PERI_MITM_PROTECTION, sizeof (uint8_t), &mitm);
        GAPBondMgr_SetParameter (GAPBOND_PERI_IO_CAPABILITIES, sizeof (uint8_t), &ioCap);
        GAPBondMgr_SetParameter (GAPBOND_PERI_BONDING_ENABLED, sizeof (uint8_t), &bonding);
        GAPBondMgr_SetParameter (GAPBOND_PERI_KEY_DIST_LIST, sizeof (uint8_t), &periKeyDist);
        GAPBondMgr_SetParameter (GAPBOND_CENT_KEY_DIST_LIST, sizeof (uint8_t), &centKeyDist);
#if !((defined(BLE_SNV_PERSIST_ENABLE)) && (BLE_SNV_PERSIST_ENABLE == TRUE))
        {
            uint8_t bondAuto = FALSE;
            GAPBondMgr_SetParameter (GAPBOND_BOND_AUTO, sizeof (uint8_t), &bondAuto);
        }
#endif
    }


    // Initialize HID device core and callbacks.
    // HidDev_Init adds GAP/GATT/DevInfo/Batt/ScanParam services.
    HidDev_Register (&g_hidDevCfg, &g_hidDevCallbacks);
    HidDev_Init();

    // Fill Device Information characteristics.
    {
        const uint8_t devinfoManuf[] = KBD_USB_MANUFACTURER_STRING;
        const uint8_t devinfoModel[] = BLE_DEVICE_NAME;
        const uint8_t devinfoFw[] = "2.0.0";
        uint8_t pnpId[DEVINFO_PNP_ID_LEN] = {
            0x01,
            LO_UINT16 (KBD_USB_VENDOR_ID), HI_UINT16 (KBD_USB_VENDOR_ID),
            LO_UINT16 (KBD_USB_PRODUCT_ID), HI_UINT16 (KBD_USB_PRODUCT_ID),
            0x00, 0x01
        };

        DevInfo_SetParameter (DEVINFO_MANUFACTURER_NAME, sizeof (devinfoManuf) - 1, (void *)devinfoManuf);
        DevInfo_SetParameter (DEVINFO_MODEL_NUMBER, sizeof (devinfoModel) - 1, (void *)devinfoModel);
        DevInfo_SetParameter (DEVINFO_FIRMWARE_REV, sizeof (devinfoFw) - 1, (void *)devinfoFw);
        DevInfo_SetParameter (DEVINFO_SOFTWARE_REV, sizeof (devinfoFw) - 1, (void *)devinfoFw);
        DevInfo_SetParameter (DEVINFO_PNP_ID, DEVINFO_PNP_ID_LEN, pnpId);
    }

    {
        uint8_t critical = 6;
        Batt_SetParameter (BATT_PARAM_CRITICAL_LEVEL, sizeof (uint8_t), &critical);
    }

    // Add HID report service after Batt service exists.
    status = HidKbdMouse_AddService();
    if (status != SUCCESS) {
        LOG_E (TAG, "HidKbdMouse_AddService failed: 0x%02X", status);
        return -1;
    }
    // 闂佸憡鍑归崹鐗堟叏閳哄啯濯奸柟顖嗗本校
    tmos_set_event (bleHidTaskId, BLE_HID_START_DEVICE_EVT);

    LOG_I (TAG, "Init done");

    return 0;
}

/* ==================== TMOS 婵炲瓨绮岄鍕枎閵忊€崇窞闁告洦鍘介崐?==================== */

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
        // 闂佸憡鍑归崹鐗堟叏閳哄啯濯奸柟顖嗗本校婵炴垶鎸哥粔鎾疮閳ь剟姊婚崶锝呬壕闁荤喐娲戦懗鑸电珶閹烘垟鏋斿┑鐘插亞濡查亶鏌ｉ悙鍙夛紨缂佽鲸缂搃dDev_Init 閻庣懓鎲¤ぐ鍐偩椤掑嫬绠?
        return (events ^ BLE_HID_START_DEVICE_EVT);
    }

    if (events & BLE_HID_PARAM_UPDATE_EVT) {
        // 闁荤姴娲弨閬嶆儑閻楀牊浜ら柣鎰綑婢跺秹鏌涘▎蹇撯偓褰掑汲閻旂厧鍗抽悗娑櫳戦悡鈧?
        GAPRole_PeripheralConnParamUpdateReq (g_conn_handle,
                                              BLE_CONN_INT_MIN,
                                              BLE_CONN_INT_MAX,
                                              BLE_SLAVE_LATENCY,
                                              BLE_CONN_TIMEOUT,
                                              bleHidTaskId);
        return (events ^ BLE_HID_PARAM_UPDATE_EVT);
    }

    if (events & BLE_HID_PHY_UPDATE_EVT) {
        // 闁荤姴娲弨閬嶆儑?PHY 闂佸搫娲ら悺銊╁蓟婵犲洤绀?2M
        GAPRole_UpdatePHY (g_conn_handle, 0,
                           GAP_PHY_BIT_LE_2M, GAP_PHY_BIT_LE_2M, 0);
        return (events ^ BLE_HID_PHY_UPDATE_EVT);
    }

    if (events & BLE_HID_SECURITY_REQ_EVT) {
#if BLE_AUTO_SECURITY_REQ
        if (BLE_HID_IsConnected() && (g_conn_handle != GAP_CONNHANDLE_INIT)) {
#if BLE_SECURITY_REQ_DIAG_LOG
            BLE_HID_DiagLog (BLE_HID_MapStateToLog (g_ble_state), BLE_DIAG_OP_SECURITY_REQ_CALL,
                             g_security_req_retry, g_conn_handle);
#endif
            bStatus_t secReq = GAPBondMgr_PeriSecurityReq (g_conn_handle);
#if BLE_SECURITY_REQ_DIAG_LOG
            BLE_HID_DiagLog (BLE_HID_MapStateToLog (g_ble_state), 0xB7,
                             (uint8_t)secReq, g_conn_handle);
#endif

            if ((secReq != SUCCESS) && (g_security_req_retry < SECURITY_REQ_MAX_RETRY)) {
                g_security_req_retry++;
#if BLE_SECURITY_REQ_DIAG_LOG
                BLE_HID_DiagLog (BLE_HID_MapStateToLog (g_ble_state), 0xB8,
                                 g_security_req_retry, g_conn_handle);
#endif
                tmos_start_task (bleHidTaskId, BLE_HID_SECURITY_REQ_EVT,
                                 MS1_TO_SYSTEM_TIME (SECURITY_REQ_RETRY_DELAY_MS));
            }
        } else {
#if BLE_SECURITY_REQ_DIAG_LOG
            BLE_HID_DiagLog (BLE_HID_MapStateToLog (g_ble_state), BLE_DIAG_OP_SECURITY_REQ_SKIP,
                             0x01, g_conn_handle);
#endif
        }
#endif
        return (events ^ BLE_HID_SECURITY_REQ_EVT);
    }

    if (events & BLE_HID_HID_READY_CHECK_EVT) {
        // If host does not enable input notification after link established,
        // terminate the stale link to recover automatically.
        if (BLE_HID_IsConnected() && !g_hid_notify_ready && (g_conn_handle != GAP_CONNHANDLE_INIT)) {
            LOG_W (TAG, "HID notify not enabled in time, terminate link=%u", g_conn_handle);
            BLE_HID_DiagLog (BLE_HID_MapStateToLog (g_ble_state),
                             BLE_DIAG_OP_HID_READY_TIMEOUT, 0x00, g_conn_handle);
            GAPRole_TerminateLink (g_conn_handle);
        }
        return (events ^ BLE_HID_HID_READY_CHECK_EVT);
    }

    return 0;
}

static void BLE_HID_ProcessTMOSMsg (tmos_event_hdr_t *pMsg) {
    switch (pMsg->event) {
    default:
        break;
    }
}

/* ==================== 闁哄鏅濋崑鐐垫暜鐎靛摜涓嶉柨娑樺閸婄偤鎮楅崷顓炰户妤?==================== */

int BLE_HID_StartAdvertising (void) {
    if (!g_ble_enabled) {
        return -1;
    }
    g_adv_manual_requested = true;

    // 闁荤姳绀佹晶浠嬫偪閸℃稓宓侀柟绋垮鐏忥繝鏌涘▎蹇撯偓褰掑汲?
    GAP_SetParamValue (TGAP_DISC_ADV_INT_MIN, BLE_ADV_INT_MIN);
    GAP_SetParamValue (TGAP_DISC_ADV_INT_MAX, BLE_ADV_INT_MAX);
    GAP_SetParamValue (TGAP_LIM_ADV_TIMEOUT, BLE_ADV_TIMEOUT);

    // 閻庢鍠掗崑鎾斥攽椤旂⒈鍎忕紒鐘茬Ч楠?
    uint8_t enable = TRUE;
    GAPRole_SetParameter (GAPROLE_ADVERT_ENABLED, sizeof (uint8_t), &enable);

    LOG_I (TAG, "Start advertising");
    return 0;
}

int BLE_HID_StopAdvertising (void) {
    uint8_t enable = FALSE;
    GAPRole_SetParameter (GAPROLE_ADVERT_ENABLED, sizeof (uint8_t), &enable);
    g_adv_manual_requested = false;

    LOG_I (TAG, "Stop advertising");
    return 0;
}

int BLE_HID_Disconnect (void) {
    if (g_conn_handle != GAP_CONNHANDLE_INIT) {
        tmos_stop_task (bleHidTaskId, BLE_HID_HID_READY_CHECK_EVT);
        tmos_stop_task (bleHidTaskId, BLE_HID_SECURITY_REQ_EVT);
        g_hid_notify_ready = false;
        g_security_req_retry = 0;
        GAPRole_TerminateLink (g_conn_handle);
        LOG_I (TAG, "Disconnect");
    }
    return 0;
}

bool BLE_HID_IsConnected (void) {
    uint8_t state = (g_ble_state & GAPROLE_STATE_ADV_MASK);
    return (state == GAPROLE_CONNECTED || state == GAPROLE_CONNECTED_ADV);
}

gapRole_States_t BLE_HID_GetState (void) {
    return g_ble_state;
}

bool BLE_HID_PollStateChange (gapRole_States_t *out_state) {
    if (!s_state_pending || !out_state) {
        return false;
    }
    *out_state = (gapRole_States_t)s_pending_state;
    s_state_pending = 0;
    return true;
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

/* ==================== HID 闂佺缈伴崕閬嶅箟閿熺姴鐭楅柟杈捐吂閸嬫挻鎷呴崫銉︽緰闂?==================== */

int BLE_HID_SendKeyboardReport (uint8_t modifier, uint8_t *keys, uint8_t key_count) {
    if (!BLE_HID_IsConnected() || !g_hid_notify_ready) {
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
    if (!BLE_HID_IsConnected() || !g_hid_notify_ready) {
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
    if (!BLE_HID_IsConnected() || !g_hid_notify_ready) {
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

/* ==================== 闂佹眹鍨归悘姘辫姳椤斿墽涓嶉柨娑樺閸婄偤鎮楅崷顓炰户妤?==================== */

void BLE_HID_Enable (void) {
    g_ble_enabled = true;
}

void BLE_HID_Disable (void) {
    g_ble_enabled = false;
    g_adv_manual_requested = false;
    g_hid_notify_ready = false;
    tmos_stop_task (bleHidTaskId, BLE_HID_HID_READY_CHECK_EVT);
    tmos_stop_task (bleHidTaskId, BLE_HID_SECURITY_REQ_EVT);
    g_security_req_retry = 0;
    BLE_HID_StopAdvertising();
    if (BLE_HID_IsConnected()) {
        BLE_HID_Disconnect();
    }
}

bool BLE_HID_IsEnabled (void) {
    return g_ble_enabled;
}

/* ==================== 闂佹悶鍎抽崑鐘绘儍閻旂厧绀勯柤鎭掑劜濞堝爼鎮楅崷顓炰户妤?==================== */

static uint8_t BLE_HID_RptCallback (uint8_t id, uint8_t type, uint16_t uuid,
                                    uint8_t oper, uint16_t *pLen, uint8_t *pData) {
    uint8_t status = SUCCESS;

    if (oper == HID_DEV_OPER_WRITE) {
        // 闂佸憡鍔栭悷锕傚箹闁垮濯?- 婵犮垼娉涚€氼噣骞?LED 闁哄鐗婇幐鎼佸吹椤撱垹绠柕澶堝劜閸?
        if (uuid == REPORT_UUID && type == HID_REPORT_TYPE_OUTPUT) {
            if (*pLen >= 1) {
                g_keyboard_leds = pData[0];
                LOG_D (TAG, "LED=0x%02X", g_keyboard_leds);

                // 闁荤姴顑呴崯浼村极閵堝鐐婇柣鎰濞?
                if (g_pCallbacks && g_pCallbacks->onLedReport) {
                    g_pCallbacks->onLedReport (g_keyboard_leds);
                }
            }
        }

        // 婵烇絽娲︾换鍌炴偤閵娾晛鐭楅柛灞剧⊕濞?
        if (status == SUCCESS) {
            status = HidKbdMouse_SetParameter (id, type, uuid, *pLen, pData);
        }
    } else if (oper == HID_DEV_OPER_READ) {
        // 闁荤姴娲ㄧ划顖炲箹闁垮濯?
        status = HidKbdMouse_GetParameter (id, type, uuid, pLen, pData);
    } else if (oper == HID_DEV_OPER_ENABLE) {
        // Input report notification enabled by host.
        LOG_D (TAG, "Report %d enabled", id);
        BLE_HID_DiagLog (BLE_HID_MapStateToLog (g_ble_state),
                         BLE_DIAG_OP_HID_NOTIFY_TOGGLE, HID_DEV_OPER_ENABLE,
                         ((uint16_t)type << 8) | id);
        if (type == HID_REPORT_TYPE_INPUT) {
            g_hid_notify_ready = true;
            tmos_stop_task (bleHidTaskId, BLE_HID_HID_READY_CHECK_EVT);
            BLE_HID_DiagLog (BLE_HID_MapStateToLog (g_ble_state),
                             BLE_DIAG_OP_HID_NOTIFY_ENABLED, id, g_conn_handle);
        }
    } else if (oper == HID_DEV_OPER_DISABLE) {
        BLE_HID_DiagLog (BLE_HID_MapStateToLog (g_ble_state),
                         BLE_DIAG_OP_HID_NOTIFY_TOGGLE, HID_DEV_OPER_DISABLE,
                         ((uint16_t)type << 8) | id);
        if (type == HID_REPORT_TYPE_INPUT && id == HID_RPT_ID_KEY_IN) {
            g_hid_notify_ready = false;
        }
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

static uint8_t BLE_HID_MapStateToLog (gapRole_States_t state) {
    switch (state & GAPROLE_STATE_ADV_MASK) {
    case GAPROLE_ADVERTISING:
        return 1;
    case GAPROLE_CONNECTED:
    case GAPROLE_CONNECTED_ADV:
        return 2;
    case GAPROLE_WAITING:
    default:
        return 0;
    }
}

static void BLE_HID_StateCallback (gapRole_States_t newState, gapRoleEvent_t *pEvent) {
    uint8_t opcode = 0xFF;
    uint8_t reason = 0;
    uint16_t diag_handle = g_conn_handle;

    if (pEvent) {
        opcode = pEvent->gap.opcode;
        reason = pEvent->gap.hdr.status;

        if (opcode == GAP_LINK_ESTABLISHED_EVENT) {
            diag_handle = pEvent->linkCmpl.connectionHandle;
        } else if (opcode == GAP_LINK_PARAM_UPDATE_EVENT) {
            diag_handle = pEvent->linkUpdate.connectionHandle;
            reason = pEvent->linkUpdate.status;
        } else if (opcode == GAP_LINK_TERMINATED_EVENT) {
            diag_handle = pEvent->linkTerminate.connectionHandle;
            reason = pEvent->linkTerminate.reason;
        }
    }

#if BLE_STATE_CB_DIAG_LOG
    BLE_HID_DiagLog (BLE_HID_MapStateToLog (newState), BLE_DIAG_OP_STATE_CB_ENTER,
                     opcode, diag_handle);
#endif
    g_ble_state = newState;

    switch (newState & GAPROLE_STATE_ADV_MASK) {
    case GAPROLE_STARTED: {
        uint8_t ownAddr[6];
        GAPRole_GetParameter (GAPROLE_BD_ADDR, ownAddr);
        LOG_I (TAG, "Started, Addr=%02X:%02X:%02X:%02X:%02X:%02X",
               ownAddr[5], ownAddr[4], ownAddr[3],
               ownAddr[2], ownAddr[1], ownAddr[0]);
    } break;

    case GAPROLE_ADVERTISING:
        if (opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT) {
            LOG_I (TAG, "Advertising");
        }
        break;

    case GAPROLE_CONNECTED:
        if (opcode == GAP_LINK_ESTABLISHED_EVENT) {
            gapEstLinkReqEvent_t *event = (gapEstLinkReqEvent_t *)pEvent;
            g_conn_handle = event->connectionHandle;
            diag_handle = g_conn_handle;
            g_adv_manual_requested = false;
            g_hid_notify_ready = false;
            g_security_req_retry = 0;
            {
                uint8_t adv_enable = FALSE;
                GAPRole_SetParameter (GAPROLE_ADVERT_ENABLED, sizeof (uint8_t), &adv_enable);
            }

            // 閻庣偣鍊栭崕鑲╂崲濠婂懏瀚氶梺鍨儑濠€鎾煕濞嗗繐鈧綊寮抽悢鐓庡嵆閻庢稒蓱閻撯偓
            tmos_start_task (bleHidTaskId, BLE_HID_PARAM_UPDATE_EVT, PARAM_UPDATE_DELAY);
#if BLE_AUTO_SECURITY_REQ
            tmos_stop_task (bleHidTaskId, BLE_HID_SECURITY_REQ_EVT);
            tmos_start_task (bleHidTaskId, BLE_HID_SECURITY_REQ_EVT,
                             MS1_TO_SYSTEM_TIME (SECURITY_REQ_DELAY_MS));
#endif
            tmos_stop_task (bleHidTaskId, BLE_HID_HID_READY_CHECK_EVT);
            tmos_start_task (bleHidTaskId, BLE_HID_HID_READY_CHECK_EVT,
                             MS1_TO_SYSTEM_TIME (HID_READY_CHECK_DELAY_MS));

            LOG_I (TAG,
                   "Connected, handle=%d peer=%02X:%02X:%02X:%02X:%02X:%02X type=%u",
                   g_conn_handle,
                   event->devAddr[5], event->devAddr[4], event->devAddr[3],
                   event->devAddr[2], event->devAddr[1], event->devAddr[0],
                   event->devAddrType);

#if BLE_STATE_CB_DIAG_LOG
            BLE_HID_DiagLog (BLE_HID_MapStateToLog (newState), 0xC0, event->devAddrType,
                             ((uint16_t)event->devAddr[1] << 8) | event->devAddr[0]);
            BLE_HID_DiagLog (BLE_HID_MapStateToLog (newState), 0xC1, event->devAddrType,
                             ((uint16_t)event->devAddr[3] << 8) | event->devAddr[2]);
            BLE_HID_DiagLog (BLE_HID_MapStateToLog (newState), 0xC2, event->devAddrType,
                             ((uint16_t)event->devAddr[5] << 8) | event->devAddr[4]);
#endif
        } else if (opcode == GAP_LINK_PARAM_UPDATE_EVENT) {
            gapLinkUpdateEvent_t *event = (gapLinkUpdateEvent_t *)pEvent;
            LOG_I (TAG, "Conn param update st=%02X int=%d lat=%d to=%d",
                   event->status, event->connInterval, event->connLatency, event->connTimeout);
        }
        break;

    case GAPROLE_CONNECTED_ADV:
        if (opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT) {
            LOG_I (TAG, "Connected advertising");
        }
        break;

    case GAPROLE_WAITING:
        tmos_stop_task (bleHidTaskId, BLE_HID_PARAM_UPDATE_EVT);
        tmos_stop_task (bleHidTaskId, BLE_HID_PHY_UPDATE_EVT);
        tmos_stop_task (bleHidTaskId, BLE_HID_HID_READY_CHECK_EVT);
        tmos_stop_task (bleHidTaskId, BLE_HID_SECURITY_REQ_EVT);
        g_hid_notify_ready = false;
        g_security_req_retry = 0;
        g_conn_handle = GAP_CONNHANDLE_INIT;

        if (opcode == GAP_END_DISCOVERABLE_DONE_EVENT) {
            LOG_I (TAG, "Advertising timeout");
        } else if (opcode == GAP_LINK_TERMINATED_EVENT) {
            LOG_I (TAG, "Disconnected, reason=%02X",
                   reason);
        }

        // 婵炲濮撮幊搴★耿?BLE 婵炶揪缍€閸庡宕楀鈧顕€鎳滈棃娑辨瀫闂佺厧顨庢禍婊勬叏閳哄懎绠掗柕蹇曞濡茬粯顨ラ悙鏉戭嚋闁规悂浜堕弫宥呯暆閸曨亞绱氶梺?USB 濠碘槅鍨埀顒€纾涵鈧繛鎴炴尭椤戝洭骞嬫搴ｇ＜妞ゆ挾鍋涘В澶愭煟濠婂嫭绶叉繝鈧鍛枖缂備焦蓱缁ㄦ岸鏌﹂崘鈺冪畼濠⒀屽櫍瀹曟艾螖閸曗斁鍋?
        if (g_ble_enabled && (KBD_AUTO_START_ADVERTISING || g_adv_manual_requested)) {
            uint8_t enable = TRUE;
            GAPRole_SetParameter (GAPROLE_ADVERT_ENABLED, sizeof (uint8_t), &enable);
        }
        break;

    case GAPROLE_ERROR:
        LOG_W (TAG, "Error %02X", opcode);
        break;

    default:
        break;
    }

#if BLE_STATE_CB_DIAG_LOG
    BLE_HID_DiagLog (BLE_HID_MapStateToLog (newState), opcode, reason, diag_handle);
    BLE_HID_DiagLog (BLE_HID_MapStateToLog (newState), BLE_DIAG_OP_STATE_CB_EXIT,
                     opcode, diag_handle);
#endif

    /* 延后到主循环 KBD_Mode_Process 中执行，避免在 BLE 栈上下文中调用应用层导致卡死 */
    s_pending_state = newState;
    s_state_pending = 1;
}

static void BLE_HID_BattCallback (uint8_t event) {
    if (event == BATT_LEVEL_NOTI_ENABLED) {
        LOG_I (TAG, "Battery notification enabled");
    } else if (event == BATT_LEVEL_NOTI_DISABLED) {
        LOG_I (TAG, "Battery notification disabled");
    }
}

static void BLE_HID_ScanParamCallback (uint8_t event) {
    // 闂佽顔栭崑鍛嚕閸ф鐭楅柛灞剧⊕濞堝爼鏌￠崶褏鎽犻柡?
}
