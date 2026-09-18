cmake_minimum_required(VERSION 3.24)
project(WaySaver VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets DBus)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner REQUIRED)

if(NOT WAYLAND_PROTOCOLS_DIR)
    message(FATAL_ERROR "wayland-protocols was not found through pkg-config")
endif()

set(IDLE_NOTIFY_XML
    "${WAYLAND_PROTOCOLS_DIR}/staging/ext-idle-notify/ext-idle-notify-v1.xml")
if(NOT EXISTS "${IDLE_NOTIFY_XML}")
    message(FATAL_ERROR "ext-idle-notify-v1.xml was not found in ${WAYLAND_PROTOCOLS_DIR}")
endif()

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(IDLE_NOTIFY_HEADER "${GENERATED_DIR}/ext-idle-notify-v1-client-protocol.h")
set(IDLE_NOTIFY_CODE "${GENERATED_DIR}/ext-idle-notify-v1-protocol.c")
add_custom_command(
    OUTPUT "${IDLE_NOTIFY_HEADER}" "${IDLE_NOTIFY_CODE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_DIR}"
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header
            "${IDLE_NOTIFY_XML}" "${IDLE_NOTIFY_HEADER}"
    COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code
            "${IDLE_NOTIFY_XML}" "${IDLE_NOTIFY_CODE}"
    DEPENDS "${IDLE_NOTIFY_XML}"
    VERBATIM
)

add_executable(waysaver
    src/main.cpp
    src/appconfig.cpp src/appconfig.h
    src/daemon.cpp src/daemon.h
    src/mediaguard.cpp src/mediaguard.h
    src/packageapi.cpp src/packageapi.h
    src/saverwindow.cpp src/saverwindow.h
    src/scrimporter.cpp src/scrimporter.h
    src/settingswindow.cpp src/settingswindow.h
    src/waylandidlemonitor.cpp src/waylandidlemonitor.h
    "${IDLE_NOTIFY_CODE}" "${IDLE_NOTIFY_HEADER}"
)

target_link_libraries(waysaver PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus PkgConfig::WAYLAND_CLIENT
)
target_include_directories(waysaver PRIVATE "${GENERATED_DIR}")
target_compile_definitions(waysaver PRIVATE WAYSAVER_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)
configure_file(data/waysaver.service.in waysaver.service @ONLY)
configure_file(data/waysaver-daemon.service.in waysaver-daemon.service @ONLY)
install(TARGETS waysaver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/waysaver.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/waysaver-handler.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/waysaver.service DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/waysaver-daemon.service DESTINATION ${CMAKE_INSTALL_DATADIR}/systemd/user)
install(DIRECTORY plasmoid/ DESTINATION ${CMAKE_INSTALL_DATADIR}/plasma/plasmoids/org.waysaver.widget)
install(FILES api/waysaver-package-v1.schema.json DESTINATION ${CMAKE_INSTALL_DATADIR}/waysaver/api)
install(DIRECTORY integrations/ DESTINATION ${CMAKE_INSTALL_DATADIR}/waysaver/integrations)

add_custom_target(check-manifest
    COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen $<TARGET_FILE:waysaver> --self-test
    DEPENDS waysaver
)
