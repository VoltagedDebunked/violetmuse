cmake_minimum_required(VERSION 3.12)
project(VioletMuse VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_executable(VioletMuse src/main.c)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(VLC REQUIRED libvlc)

target_include_directories(VioletMuse PRIVATE ${GTK3_INCLUDE_DIRS} ${VLC_INCLUDE_DIRS})
target_link_libraries(VioletMuse PRIVATE ${GTK3_LIBRARIES} ${VLC_LIBRARIES})

install(TARGETS VioletMuse DESTINATION bin)

set(CPACK_PACKAGE_NAME "VioletMuse")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A free, open-source Spotify alternative for Linux, built with GTK and VLC")
set(CPACK_PACKAGE_VENDOR "Voltaged")
set(CPACK_PACKAGE_CONTACT "voltageddebunked@github.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_EXECUTABLES "VioletMuse" "VioletMuse")

include(CPack)
