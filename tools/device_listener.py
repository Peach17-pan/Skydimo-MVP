#!/usr/bin/env python3
"""
Skydimo 设备信息接收器
用于接收 SK-LCD 设备通过 UDP 广播上报的设备信息

使用方法:
    python device_listener.py [--port PORT]

依赖:
    Python 3.6+
"""

import socket
import json
import argparse
from datetime import datetime
import sys

DEFAULT_PORT = 9527

def format_device_info(data: dict) -> str:
    """格式化设备信息为可读字符串"""
    lines = [
        "=" * 50,
        f"  🔌 设备发现: {data.get('name', 'Unknown')}",
        "=" * 50,
        f"  📱 类型:      {data.get('device_type', 'N/A')}",
        f"  🌐 IP:        {data.get('ip', 'N/A')}",
        f"  🔗 MAC:       {data.get('mac', 'N/A')}",
        f"  📶 RSSI:      {data.get('rssi', 0)} dBm",
        f"  ⏱️  运行时间:  {data.get('uptime_s', 0)} 秒",
        f"  💾 空闲内存:  {data.get('free_heap_kb', 0)} KB",
        f"  🕐 时间戳:    {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}",
        "=" * 50,
    ]
    return "\n".join(lines)


def listen_for_devices(port: int):
    """监听设备广播"""
    print(f"\n{'=' * 50}")
    print(f"  🎧 Skydimo 设备监听器")
    print(f"  📡 监听端口: {port}")
    print(f"  ⏳ 等待设备上报...")
    print(f"{'=' * 50}\n")
    
    # 创建 UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    # 绑定到所有网络接口
    try:
        sock.bind(('', port))
    except OSError as e:
        print(f"❌ 无法绑定端口 {port}: {e}")
        print(f"   请检查端口是否被占用，或尝试使用管理员权限运行")
        sys.exit(1)
    
    print(f"✅ 成功绑定到 0.0.0.0:{port}")
    print(f"   按 Ctrl+C 停止监听\n")
    
    devices_seen = {}
    
    try:
        while True:
            data, addr = sock.recvfrom(4096)
            
            try:
                # 解析 JSON 数据
                payload = json.loads(data.decode('utf-8'))
                
                # 检查消息类型
                if payload.get('type') != 'device_report':
                    continue
                
                # 记录设备
                device_key = payload.get('mac', addr[0])
                is_new = device_key not in devices_seen
                devices_seen[device_key] = {
                    'last_seen': datetime.now(),
                    'data': payload,
                    'addr': addr,
                }
                
                # 输出设备信息
                if is_new:
                    print(f"\n🆕 发现新设备! 来自 {addr[0]}:{addr[1]}")
                else:
                    print(f"\n🔄 设备更新: {addr[0]}:{addr[1]}")
                
                print(format_device_info(payload))
                
                # 显示原始 JSON（调试用）
                print(f"\n📝 原始数据:\n{json.dumps(payload, indent=2)}\n")
                
            except json.JSONDecodeError:
                print(f"⚠️  收到无效数据来自 {addr}: {data[:100]}...")
            except Exception as e:
                print(f"⚠️  处理数据时出错: {e}")
                
    except KeyboardInterrupt:
        print(f"\n\n👋 停止监听")
        print(f"   共发现 {len(devices_seen)} 个设备")
        
        if devices_seen:
            print("\n📋 已发现的设备列表:")
            for mac, info in devices_seen.items():
                data = info['data']
                print(f"   - {data.get('name', 'Unknown')} ({data.get('ip', 'N/A')}) - MAC: {mac}")
    
    finally:
        sock.close()


def main():
    parser = argparse.ArgumentParser(
        description='Skydimo 设备信息接收器',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
示例:
    python device_listener.py
    python device_listener.py --port 9527
    python device_listener.py -p 8888
        '''
    )
    
    parser.add_argument(
        '-p', '--port',
        type=int,
        default=DEFAULT_PORT,
        help=f'UDP 监听端口 (默认: {DEFAULT_PORT})'
    )
    
    args = parser.parse_args()
    
    listen_for_devices(args.port)


if __name__ == '__main__':
    main()





