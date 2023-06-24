cmake_minimum_required(VERSION 3.15)
project(layered-shot)

# PSD

add_library(psd
    STATIC
    third_party/psd_sdk/src/Psd/PsdAllocator.cpp
    third_party/psd_sdk/src/Psd/PsdBlendMode.cpp
    third_party/psd_sdk/src/Psd/PsdColorMode.cpp
    third_party/psd_sdk/src/Psd/PsdDecompressRle.cpp
    third_party/psd_sdk/src/Psd/PsdExport.cpp
    third_party/psd_sdk/src/Psd/PsdFile.cpp
    third_party/psd_sdk/src/Psd/PsdFixedSizeString.cpp
    third_party/psd_sdk/src/Psd/PsdInterleave.cpp
    third_party/psd_sdk/src/Psd/PsdLayerCanvasCopy.cpp
    third_party/psd_sdk/src/Psd/PsdMallocAllocator.cpp
    third_party/psd_sdk/src/Psd/Psdminiz.c
    third_party/psd_sdk/src/Psd/PsdNativeFile.cpp
    third_party/psd_sdk/src/Psd/PsdParseColorModeDataSection.cpp
    third_party/psd_sdk/src/Psd/PsdParseDocument.cpp
    third_party/psd_sdk/src/Psd/PsdParseImageDataSection.cpp
    third_party/psd_sdk/src/Psd/PsdParseImageResourcesSection.cpp
    third_party/psd_sdk/src/Psd/PsdParseLayerMaskSection.cpp
    third_party/psd_sdk/src/Psd/PsdPch.cpp
    third_party/psd_sdk/src/Psd/PsdStringUtil.cpp
    third_party/psd_sdk/src/Psd/PsdSyncFileReader.cpp
    third_party/psd_sdk/src/Psd/PsdSyncFileWriter.cpp
)
target_compile_features(psd PRIVATE cxx_std_11)
target_compile_definitions(psd PRIVATE _CRT_SECURE_NO_WARNINGS)
target_include_directories(psd INTERFACE third_party/psd_sdk/src)


# WIL
set(WIL_BUILD_TESTS OFF)
add_subdirectory(third_party/wil)


# Application

add_executable(layered-shot
    src/main.cpp
    src/dc.hpp src/dc.cpp
    src/bitmap.hpp src/bitmap.cpp
    src/psd.hpp src/psd.cpp
)
target_compile_features(layered-shot PRIVATE cxx_std_20)
target_compile_options(layered-shot PRIVATE /W4 /WX /permissive-)
target_link_libraries(layered-shot PRIVATE psd WIL::WIL)
