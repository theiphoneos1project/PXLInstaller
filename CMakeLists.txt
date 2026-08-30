cmake_minimum_required(VERSION 3.20)
project(pxlinstaller CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(STRICT_FLAGS
        /W4
        /WX
        /permissive-
    )
else()
    set(STRICT_FLAGS
        -Wall
        -Werror
        -Wpedantic
        -Wextra
        -Wshadow
        -Wformat=2
        -Wswitch-enum
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND STRICT_FLAGS
            -Wnull-dereference
            -Wno-zero-length-array
            -Wno-c99-extensions
            -Wno-strict-prototypes
        )
    endif()
endif()

find_package(unofficial-libplist CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)

if(MSVC)
    set(ENV{PKG_CONFIG_PATH} "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/pkgconfig;${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib/pkgconfig")
    
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " /LTCG /INCREMENTAL:NO /NODEFAULTLIB:msvcrtd")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /GL")
    string(APPEND CMAKE_C_FLAGS_RELEASE " /GL")
endif()

pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

add_library(pxlinstaller-core STATIC
    core/pxl/PXLManager.cpp
    core/lockdownd/Crypto.cpp
    core/lockdownd/LockdownDaemonClient.cpp
    core/afc/AppleFileConduitSession.cpp
    core/usb/mux.c
)

target_compile_options(pxlinstaller-core PRIVATE ${STRICT_FLAGS})

target_include_directories(pxlinstaller-core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(pxlinstaller-core PUBLIC
    unofficial::libplist::libplist
    unofficial::libplist::libplist++
    PkgConfig::LIBUSB
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_include_directories(pxlinstaller-core PRIVATE ${CMAKE_SOURCE_DIR})

if(WIN32)
    target_link_libraries(pxlinstaller-core PRIVATE ws2_32)
endif()

# CLI

add_executable(pxlinstaller cli/main.cpp)
target_compile_options(pxlinstaller PRIVATE ${STRICT_FLAGS})
target_link_libraries(pxlinstaller PRIVATE pxlinstaller-core)

add_custom_command(TARGET pxlinstaller POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/files
    $<TARGET_FILE_DIR:pxlinstaller>/files
)

# GUI

find_package(wxWidgets COMPONENTS core base QUIET)
if(wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})

    add_executable(pxlinstaller-gui WIN32
        gui/main.cpp
        gui/MainFrame.cpp
        gui/ControlPanel.cpp
        gui/DevicePanel.cpp
    )
    target_compile_options(pxlinstaller-gui PRIVATE ${STRICT_FLAGS})
    target_link_libraries(pxlinstaller-gui PRIVATE
        pxlinstaller-core
        ${wxWidgets_LIBRARIES}
    )

    if(APPLE)
        target_link_options(pxlinstaller-gui PRIVATE -Wl,-no_warn_duplicate_libraries)

        set_target_properties(pxlinstaller-gui PROPERTIES
            OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}"
            OSX_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}"

            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.nightwind.pxlinstaller"
            MACOSX_BUNDLE_BUNDLE_NAME "PXLInstaller"
        )

        add_custom_command(TARGET pxlinstaller-gui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/files
            $<TARGET_BUNDLE_CONTENT_DIR:pxlinstaller-gui>/Resources/files
        )
    endif()

    function(binary_to_header INPUT OUTPUT NAME)
        file(READ "${INPUT}" data HEX)
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " data "${data}")
        string(REGEX REPLACE ", $" "" data "${data}")
        file(WRITE "${OUTPUT}"
    "#pragma once
    #include <stddef.h>
    static const unsigned char ${NAME}_image[] = { ${data} };
    static const size_t ${NAME}_image_len = sizeof(${NAME}_image);
    ")
    endfunction()

    binary_to_header(${CMAKE_SOURCE_DIR}/gui/images/iphone1,1.png ${CMAKE_SOURCE_DIR}/gui/generated/iphone1_1.h iphone1_1)
    binary_to_header(${CMAKE_SOURCE_DIR}/gui/images/ipod1,1.png   ${CMAKE_SOURCE_DIR}/gui/generated/ipod1_1.h   ipod1_1)
    binary_to_header(${CMAKE_SOURCE_DIR}/gui/images/connect.png   ${CMAKE_SOURCE_DIR}/gui/generated/connect.h   connect)

    target_include_directories(pxlinstaller-gui PRIVATE ${CMAKE_SOURCE_DIR}/gui/generated)
else()
    message(STATUS "wxWidgets not found, skipping GUI target")
endif()

# Platform-specific

if(APPLE)
    target_link_options(pxlinstaller PRIVATE
        -Wl,-no_warn_duplicate_libraries
    )

    if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
    elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "" FORCE)
    endif()

    set_target_properties(pxlinstaller PROPERTIES
        OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}"
        OSX_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}"
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT WIN32)
    add_custom_command(TARGET pxlinstaller POST_BUILD
        COMMAND strip -x $<TARGET_FILE:pxlinstaller>
    )
endif()

# Distribution

set(DIST_DIR ${CMAKE_HOME_DIRECTORY}/dist)

if(APPLE AND wxWidgets_FOUND)
    set(DIST_DEPENDS pxlinstaller pxlinstaller-gui)
    set(DIST_GUI_CMD COMMAND ${CMAKE_COMMAND} -E copy_directory $<TARGET_BUNDLE_DIR:pxlinstaller-gui> ${DIST_DIR}/pxlinstaller-gui.app)
elseif(wxWidgets_FOUND)
    set(DIST_DEPENDS pxlinstaller pxlinstaller-gui)
    set(DIST_GUI_CMD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pxlinstaller-gui> ${DIST_DIR}/$<TARGET_FILE_NAME:pxlinstaller-gui>)
else()
    set(DIST_DEPENDS pxlinstaller)
    set(DIST_GUI_CMD "")
endif()

add_custom_target(dist
    DEPENDS ${DIST_DEPENDS}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pxlinstaller> ${DIST_DIR}/$<TARGET_FILE_NAME:pxlinstaller>
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/files ${DIST_DIR}/files
    ${DIST_GUI_CMD}
    COMMENT "Assembling dist/ folder"
)
