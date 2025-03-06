import hid

# 设备配置
VENDOR_ID = 0x1209  # 设备VID（十六进制）
PRODUCT_ID = 0xC55D  # 设备PID（十六进制）
REPORT_ID = 0x04  # 特征报告ID
DATA_SIZE = 31  # 有效数据长度（不含Report ID）

def send_hid_data(data: list):
    """发送数据到 HID 设备"""
    try:
        # 确保数据长度为 31 字节，不足补 0，超出截断
        if len(data) < DATA_SIZE:
            data += [0x00] * (DATA_SIZE - len(data))
        elif len(data) > DATA_SIZE:
            data = data[:DATA_SIZE]

        # 构造完整数据包
        packet = [REPORT_ID] + data

        # 打开设备
        dev = hid.device()
        dev.open(VENDOR_ID, PRODUCT_ID)  # VID/PID 需为十进制
        print(f"🔌 设备已连接: VID={VENDOR_ID}, PID={PRODUCT_ID}")
        print(dev.get_manufacturer_string())
        print(dev.get_product_string())
        print(f"🔗 报告 ID: {REPORT_ID}")
        print(f"📦 发送数据: {packet}")
        
        # 发送数据
        bytes_written = dev.write(packet)
        if bytes_written != len(packet):
            print(f"⚠️ 发送数据不完整！已发送 {bytes_written}/{len(packet)} 字节")
            print(f"设备错误: {dev.error()}")  # 可能会提供更具体的错误信息
            dev.close()
            return False

        print(f"✅ 成功发送 {bytes_written} 字节")
        
        # 关闭设备
        dev.close()
        return True

    except Exception as e:
        print(f"❌ 未知错误: {str(e)}")
    
    return False

# 示例数据发送
test_data = [0x01] * 31  # 31 字节测试数据
send_hid_data(test_data)
