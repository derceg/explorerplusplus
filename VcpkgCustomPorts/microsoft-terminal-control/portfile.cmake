vcpkg_download_distfile(
    ARCHIVE
    URLS "https://api.nuget.org/v3-flatcontainer/ci.microsoft.terminal.wpf/1.22.250204002/ci.microsoft.terminal.wpf.1.22.250204002.nupkg"
    FILENAME "ci.microsoft.terminal.wpf.1.22.250204002.nupkg"
    SHA512 7d6965693cb53efc077cd69ec385eb28dd69bb424c57411b32d27f669545d0d43a812e26d37dfda5bb094b8d9b54aa1d7479808da3c87d5772a171d1dd8b08bf
)

set(SOURCE_PATH "${CURRENT_BUILDTREES_DIR}/src")
file(REMOVE_RECURSE "${SOURCE_PATH}")
file(MAKE_DIRECTORY "${SOURCE_PATH}")
file(ARCHIVE_EXTRACT INPUT "${ARCHIVE}" DESTINATION "${SOURCE_PATH}")

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(TERMINAL_RUNTIME_ARCH "win-x86")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(TERMINAL_RUNTIME_ARCH "win-x64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(TERMINAL_RUNTIME_ARCH "win-arm64")
else()
    message(FATAL_ERROR "Unsupported architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

file(
    INSTALL "${SOURCE_PATH}/runtimes/${TERMINAL_RUNTIME_ARCH}/native/Microsoft.Terminal.Control.dll"
    DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}"
)

vcpkg_install_copyright(FILE_LIST "${CMAKE_CURRENT_LIST_DIR}/copyright")
