# 项目根目录的 CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 只设置 components 目录，不要列出子目录
set(EXTRA_COMPONENT_DIRS components)

project(skydimo-mvp)