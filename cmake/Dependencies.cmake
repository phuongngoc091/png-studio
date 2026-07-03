find_package(Qt6 6.5 REQUIRED COMPONENTS
    Quick QuickControls2 Multimedia Network Concurrent Sql LinguistTools
)
qt_standard_project_setup()

set(PNGSTUDIO_VCPKG_PREFIX_CANDIDATES)
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    list(APPEND PNGSTUDIO_VCPKG_PREFIX_CANDIDATES
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
    )
endif()
if(DEFINED VCPKG_TARGET_TRIPLET)
    list(APPEND PNGSTUDIO_VCPKG_PREFIX_CANDIDATES
        "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}"
    )
endif()
foreach(PNGSTUDIO_VCPKG_PREFIX IN LISTS PNGSTUDIO_VCPKG_PREFIX_CANDIDATES)
    if(EXISTS "${PNGSTUDIO_VCPKG_PREFIX}")
        list(PREPEND CMAKE_PREFIX_PATH "${PNGSTUDIO_VCPKG_PREFIX}")
    endif()
endforeach()

# Find CURL via vcpkg (Manifest mode). Prefer the package config so CMake
# does not fall back to its FindCURL module and miss the vcpkg install tree.
find_package(CURL CONFIG REQUIRED)
find_package(ZLIB REQUIRED)


