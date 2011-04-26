This project uses the Google Testing Framework (http://code.google.com/p/googletest/) to carry out a series of tests on the Helper project.
The following environment variables must be defined in order to build:

- GTEST
  Installation directiory of Google Test (e.g. C:\gtest).
  
- GTEST_LIB
  Path to the release library (multi-threaded) for Google Test (e.g. C:\gtest\msvc\gtest\Release\gtest.lib).
  
- GTEST_DEBUG_LIB
  Path to the debug library (multi-threaded debug) for Google Test (e.g. C:\gtest\msvc\gtest\Debug\gtestd.lib).