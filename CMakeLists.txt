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

    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

    string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /LTCG /INCREMENTAL:NO")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " /GL")
    string(APPEND CMAKE_C_FLAGS_DEBUG " /GL")
endif()

pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

set(SOURCES
    main.cpp
    pxl/PXLManager.cpp
    lockdownd/Crypto.cpp
    lockdownd/LockdownDaemonClient.cpp
    afc/AppleFileConduitSession.cpp
    usb/mux.c
)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

add_executable(pxlinstaller ${SOURCES})

target_compile_options(pxlinstaller PRIVATE ${STRICT_FLAGS})

target_link_libraries(pxlinstaller PRIVATE
    unofficial::libplist::libplist
    unofficial::libplist::libplist++
    PkgConfig::LIBUSB
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_include_directories(pxlinstaller PRIVATE ${CMAKE_SOURCE_DIR})

if(WIN32)
    target_link_libraries(pxlinstaller PRIVATE ws2_32)
endif()

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

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET pxlinstaller POST_BUILD
        COMMAND strip -x $<TARGET_FILE:pxlinstaller>
    )
endif()
