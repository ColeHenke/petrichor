cmake_minimum_required(VERSION 3.24)
project(petrichor VERSION 0.1.0 LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------
# CEF
# -------------------------------------------------------
set(CEF_ROOT "${CMAKE_SOURCE_DIR}/cef")
set(CMAKE_MODULE_PATH "${CEF_ROOT}/cmake" ${CMAKE_MODULE_PATH})

find_package(CEF REQUIRED)
include("${CEF_ROOT}/cmake/cef_macros.cmake")
add_subdirectory(${CEF_ROOT} ${CMAKE_BINARY_DIR}/cef)
set_property(DIRECTORY ${CEF_ROOT} PROPERTY EXCLUDE_FROM_ALL TRUE)

# -------------------------------------------------------
# Qt6
# -------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
message(STATUS "Qt6 found: ${Qt6_DIR}")
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

# -------------------------------------------------------
# Target names
# -------------------------------------------------------
set(CEF_TARGET "petrichor")
set(CEF_HELPER_TARGET "petrichor_Helper")
set(CEF_HELPER_OUTPUT_NAME "petrichor Helper")

# Output directory
SET_CEF_TARGET_OUT_DIR()

# -------------------------------------------------------
# Mac: ARC
# -------------------------------------------------------
option(OPTION_USE_ARC "Build with ARC (automatic Reference Counting) on macOS." ON)
if(OPTION_USE_ARC)
    list(APPEND CEF_COMPILER_FLAGS -fobjc-arc)
endif()

# -------------------------------------------------------
# Sources
# -------------------------------------------------------
set(PETRICHOR_SRCS
        src/app/PetrichorApp.cpp
        src/app/PetrichorApp.h
        src/ui/MainWindow.cpp
        src/ui/MainWindow.h
        src/ui/AddressBar.cpp
        src/ui/AddressBar.h
        src/ui/TabBar.cpp
        src/ui/TabBar.h
        src/browser/BrowserClient.cpp
        src/browser/BrowserClient.h
        src/browser/BrowserHandler.cpp
        src/browser/BrowserHandler.h
        src/browser/TabController.cpp
        src/browser/TabController.h
)

set(PETRICHOR_SRCS_MAC
        src/app/petrichor_mac.mm
        src/browser/BrowserHandler_mac.mm
)
APPEND_PLATFORM_SOURCES(PETRICHOR_SRCS)

set(PETRICHOR_HELPER_SRCS_MAC
        src/app/process_helper_mac.cc
)
APPEND_PLATFORM_SOURCES(PETRICHOR_HELPER_SRCS)

set(PETRICHOR_RESOURCES_MAC_SRCS_MAC
        mac/Info.plist.in
        mac/petrichor.icns
)
APPEND_PLATFORM_SOURCES(PETRICHOR_RESOURCES_MAC_SRCS)

set(PETRICHOR_RESOURCES_MAC_ENGLISH_LPROJ_SRCS_MAC
        mac/English.lproj/InfoPlist.strings
)
APPEND_PLATFORM_SOURCES(PETRICHOR_RESOURCES_MAC_ENGLISH_LPROJ_SRCS)

set(PETRICHOR_RESOURCES_SRCS
        ${PETRICHOR_RESOURCES_MAC_SRCS}
        ${PETRICHOR_RESOURCES_MAC_ENGLISH_LPROJ_SRCS}
)

# -------------------------------------------------------
# Mac: Main app bundle
# -------------------------------------------------------
set(CEF_APP "${CEF_TARGET_OUT_DIR}/${CEF_TARGET}.app")
set(EXECUTABLE_NAME "${CEF_TARGET}")
set(PRODUCT_NAME "${CEF_TARGET}")

add_executable(${CEF_TARGET} MACOSX_BUNDLE
        ${PETRICHOR_RESOURCES_SRCS}
        ${PETRICHOR_SRCS}
        resources/resources.qrc
)
SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
add_dependencies(${CEF_TARGET} libcef_dll_wrapper)
target_link_libraries(${CEF_TARGET}
        libcef_dll_wrapper
        ${CEF_STANDARD_LIBS}
        Qt6::Core
        Qt6::Widgets
)
set_target_properties(${CEF_TARGET} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/mac/Info.plist.in
)

target_include_directories(${CEF_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CEF_ROOT}
)

# Copy CEF framework into app bundle
COPY_MAC_FRAMEWORK("${CEF_TARGET}" "${CEF_BINARY_DIR}" "${CEF_APP}")

# -------------------------------------------------------
# Mac: Helper app bundles (one per suffix)
# -------------------------------------------------------
foreach(_suffix_list ${CEF_HELPER_APP_SUFFIXES})
    string(REPLACE ":" ";" _suffix_list ${_suffix_list})
    list(GET _suffix_list 0 _name_suffix)
    list(GET _suffix_list 1 _target_suffix)
    list(GET _suffix_list 2 _plist_suffix)

    set(_helper_target "${CEF_HELPER_TARGET}${_target_suffix}")
    set(_helper_output_name "${CEF_HELPER_OUTPUT_NAME}${_name_suffix}")

    # Generate helper plist
    set(_helper_info_plist "${CMAKE_CURRENT_BINARY_DIR}/helper-Info${_target_suffix}.plist")
    file(READ "${CMAKE_SOURCE_DIR}/mac/helper-Info.plist.in" _plist_contents)
    string(REPLACE "\${EXECUTABLE_NAME}" "${_helper_output_name}" _plist_contents ${_plist_contents})
    string(REPLACE "\${PRODUCT_NAME}" "${_helper_output_name}" _plist_contents ${_plist_contents})
    string(REPLACE "\${BUNDLE_ID_SUFFIX}" "${_plist_suffix}" _plist_contents ${_plist_contents})
    file(WRITE ${_helper_info_plist} ${_plist_contents})

    # Helper executable
    add_executable(${_helper_target} MACOSX_BUNDLE ${PETRICHOR_HELPER_SRCS})
    SET_EXECUTABLE_TARGET_PROPERTIES(${_helper_target})
    add_dependencies(${_helper_target} libcef_dll_wrapper)
    target_link_libraries(${_helper_target} libcef_dll_wrapper ${CEF_STANDARD_LIBS})
    target_include_directories(${_helper_target} PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CEF_ROOT}
    )
    set_target_properties(${_helper_target} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${_helper_info_plist}
            OUTPUT_NAME ${_helper_output_name}
    )

    add_dependencies(${CEF_TARGET} ${_helper_target})

    # Copy helper into main bundle's Frameworks dir
    add_custom_command(
            TARGET ${CEF_TARGET}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CEF_TARGET_OUT_DIR}/${_helper_output_name}.app"
            "${CEF_APP}/Contents/Frameworks/${_helper_output_name}.app"
            VERBATIM
    )
endforeach()

# Copy mac resources into bundle
set(PREFIXES "mac/")
COPY_MAC_RESOURCES(
        "${PETRICHOR_RESOURCES_SRCS}"
        "${PREFIXES}"
        "${CEF_TARGET}"
        "${CMAKE_SOURCE_DIR}"
        "${CEF_APP}"
)
