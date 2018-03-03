The projects within this solution make use of the Boost C++ libraries (http://www.boost.org), the Pantheios logging library (http://www.pantheios.org/), and the STLSoft libraries (indirectly through Pantheios). The following environment variables must be defined in order to build:

- PANTHEIOS
  Pantheios installation directory (e.g. C:\pantheios-1.0.1-beta212).

- STLSOFT
  STLSoft installation directory (e.g. C:\stlsoft-1.9.109).

Boost is included via NuGet and should be automatically installed when building with Visual Studio.