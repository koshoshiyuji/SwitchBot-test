cmake_minimum_required(VERSION 3.16.0)

project(switchbot)

message("-- [INFO] Building Example")

add_executable(
  switchbot
  switchbot.cpp
  ../common/utils.cpp
)

include_directories(
  /usr/include/python3.7m 
  /home/pi/boost_1_67_0 
  /home/pi/SwitchBot-test/include 
  /usr/include/dbus-1.0
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -std=c++17 -Wno-unused-result -Wsign-compare -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdebug-prefix-map=/build/python3.7-uw3RRX/python3.7-3.7.3=.")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -specs=/usr/share/dpkg/no-pie-compile.specs -fstack-protector -Wformat")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=format-security  -DNDEBUG -g -fwrapv -O3 -Wall")

set(ld_flags "-Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ld_flags}")

link_directories(
  /usr/lib/python3.7/config-3.7m-aarch64-linux-gnu 
  /usr/lib
)

target_link_libraries(
  switchbot
  python3.7m 
  crypt 
  pthread 
  dl 
  util 
  m 
  simpleble-static 
  boost_python37 
  boost_system 
  simpledbus-static 
  dbus-1
)

add_custom_command(
  TARGET switchbot POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/get_device_list.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/switchbot_open_token.txt ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/switchbot_py3.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
)
