# Requirements

* Visual Studio 2019, 2022 (recommended)
* Windows 10 SDK

# Setup

[vcpkg](https://vcpkg.io/) is used to manage dependencies. Before you can build Explorer++, you'll first need to initialize vcpkg:

`git clone --recurse-submodules https://github.com/derceg/explorerplusplus.git`

`cd explorerplusplus`

`.\Explorer++\ThirdParty\vcpkg\bootstrap-vcpkg.bat`

The relevant packages should then be automatically installed during the first build.

# Compiling

Open `Explorer++\Explorer++.sln`. From within Visual Studio, select `Debug` > `Start Without Debugging` to compile and run the program.

# Translations

Building the program in release mode will also build all of the translations. The resulting DLLs can then be used with Explorer++.