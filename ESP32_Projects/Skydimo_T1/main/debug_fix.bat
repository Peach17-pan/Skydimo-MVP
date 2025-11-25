@echo off
echo Debugging display issues...

echo Step 1: Clean build
idf.py fullclean

echo Step 2: Remove build directory
rmdir /s /q build 2>nul

echo Step 3: Build with optimizations
idf.py build

echo Step 4: Flash to device
idf.py -p COM11 flash  REM

echo Step 5: Monitor
idf.py -p COM11 monitor  REM 

pause