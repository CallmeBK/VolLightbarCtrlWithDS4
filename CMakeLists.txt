cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{DOLCESDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{DOLCESDK}/share/dolce.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define DOLCESDK to point to your SDK path!")
  endif()
endif()

project(VolLightbarCtrlWithDS4)
include("${DOLCESDK}/share/dolce.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

if (NOT ${RELEASE})
  add_definitions(-DENABLE_LOGGING)
endif()

add_executable(${PROJECT_NAME}
	main.c
)

target_link_libraries(${PROJECT_NAME}
  taihenForKernel_stub
  taihenModuleUtils_stub
  SceLibKernel_stub
  SceCtrlForDriver_stub
  SceModulemgrForKernel_stub
  SceDebugForDriver_stub
  SceSysmemForDriver_stub
  SceThreadmgrForDriver_stub
  SceSblAIMgrForDriver_stub
  SceRegMgrForDriver_stub
  SceDisplayForDriver_stub
  SceSysclibForDriver_stub
  SceSblACMgrForDriver_stub
  SceAppMgrForDriver_stub
  SceSysrootForKernel_stub
  SceIofilemgrForDriver_stub
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES LINK_FLAGS "-nostdlib"
)

dolce_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
  UNSAFE
  CONFIG ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.yml
)
