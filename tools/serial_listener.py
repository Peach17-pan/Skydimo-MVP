#!/usr/bin/env python3
"""
Skydimo 串口设备信息监听器
通过 USB 串口读取设备上报的 JSON 信息

使用方法:
    python serial_listener.py [--port COM11]
"""

import serial
import serial.tools.list_ports
import json
import argparse
import sys
import re

def find_esp32_port():
    """自动查找 ESP32 串口"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if 'USB' in port.description.upper() or 'UART' in port.description.upper():
            return port.device
    return None

def listen_serial(port, baudrate=115200):
    """监听串口并解析设备信息"""
    print(f"\n{'=' * 50}")
    print(f"  🔌 Skydimo 串口监听器")
    print(f"  📡 端口: {port} @ {baudrate}")
    print(f"  ⏳ 等待设备信息...")
    print(f"{'=' * 50}\n")
    
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"✅ 串口已打开: {port}")
        print(f"   按 Ctrl+C 停止监听\n")
    except Exception as e:
        print(f"❌ 无法打开串口 {port}: {e}")
        sys.exit(1)
    
    buffer = ""
    in_device_info = False
    
    try:
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if "DEVICE_INFO_START" in line:
                    in_device_info = True
                    buffer = ""
                    continue
                
                if "DEVICE_INFO_END" in line:
                    in_device_info = False
                    if buffer:
                        try:
                            data = json.loads(buffer)
                            print(f"\n🆕 收到设备信息!")
                            print(f"{'=' * 50}")
                            print(f"  📱 名称:   {data.get('name', 'N/A')}")
                            print(f"  🔧 类型:   {data.get('device_type', 'N/A')}")
                            print(f"  🌐 IP:     {data.get('ip', 'N/A')}")
                            print(f"  🔗 MAC:    {data.get('mac', 'N/A')}")
                            print(f"  📶 状态:   {data.get('status', 'N/A')}")
                            print(f"{'=' * 50}")
                            print(f"\n📝 原始 JSON:\n{json.dumps(data, indent=2)}\n")
                        except json.JSONDecodeError:
                            print(f"⚠️ JSON 解析失败: {buffer}")
                    continue
                
                if in_device_info:
                    buffer += line
                    
    except KeyboardInterrupt:
        print(f"\n\n👋 停止监听")
    finally:
        ser.close()

def main():
    parser = argparse.ArgumentParser(description='Skydimo 串口设备信息监听器')
    parser.add_argument('-p', '--port', type=str, default=None,
                        help='串口端口 (默认自动检测)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200,
                        help='波特率 (默认: 115200)')
    
    args = parser.parse_args()
    
    port = args.port
    if not port:
        port = find_esp32_port()
        if not port:
            print("❌ 未找到 ESP32 设备，请使用 --port 指定串口")
            print("\n可用串口:")
            for p in serial.tools.list_ports.comports():
                print(f"  - {p.device}: {p.description}")
            sys.exit(1)
    
    listen_serial(port, args.baudrate)

if __name__ == '__main__':
    main()

