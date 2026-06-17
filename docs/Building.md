# Building Explorer++

Explorer++ is built using Visual Studio and vcpkg.

## Requirements

- **Visual Studio 2022** (with "Desktop development with C++" workload)
- **Windows 10/11 SDK**

## Build via Command Line (Recommended)

To build Explorer++ without opening the IDE, follow these steps:

1. Open a **Developer Command Prompt for VS 2022**.
2. Navigate to the root of the project.
3. Run the build script:
   ```cmd
   build.bat [Configuration] [Platform]
   ```
   *Example:* `build.bat Release x64` or `build.bat Debug Win32`.

The script will automatically bootstrap `vcpkg` and build the entire solution using `msbuild`.

## Build via Visual Studio (IDE)

1. Open `Explorer++\Explorer++.sln` in Visual Studio 2022.
2. Build the solution (`Ctrl+Shift+B`).

## Dependencies

Explorer++ uses `vcpkg` for dependency management. The `build.bat` script handles the initialization of vcpkg. If you are using the IDE, ensure you have run `.\Explorer++\ThirdParty\vcpkg\bootstrap-vcpkg.bat` once before building.

## Translations

Building in `Release` configuration will build all translation DLLs.

## Tests

To run tests from the command line after building:
```cmd
vstest.console.exe Explorer++\bin\x64\Release\TestExplorer++.exe
```
(Adjust the path based on your build configuration and platform).
