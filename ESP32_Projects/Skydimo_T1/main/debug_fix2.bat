@echo off
echo 综合调试修复脚本...

echo Step 1: 清理构建
idf.py fullclean

echo Step 2: 删除构建目录
rmdir /s /q build 2>nul

echo Step 3: 设置目标
idf.py set-target esp32s3

echo Step 4: 配置项目
idf.py menuconfig

echo Step 5: 构建项目
idf.py build

echo Step 6: 刷写到设备
idf.py -p COM11 flash

echo Step 7: 启动监视器
idf.py -p COM11 monitor

pause