---
name: Crash report
about: Please use this template to file a crash report
title: ''
labels: bug, crash
assignees: ''

---

Before reporting a crash, please make sure you've gone through the following steps first:
1. Check to see whether the crash is still present in the latest dev version.
2. Once you've confirmed the crash is still present, go through the steps necessary to trigger the crash. Once the application has crashed, it may show a dialog indicating that a crash dump has been created. If that's the case, attach the crash dump here. The dump file should be created in %TEMP% and be named something like Explorer++1.4.0.0-11102020-194414.dmp.
3. If no crash dump is generated, run the latest dev version with the --enable-logging command line flag. A few logging files should be created in %TEMP%. If a file named something like Explorer++.exe.DESKTOP-1234.User.log.FATAL.20240405-190058 contains any logging lines, attach that file here. Depending on how the crash occurs, it's possible that the log file may contain useful information, even if no crash dump is generated.
4. If no crash dump is generated and the log files contain no relevant information, please describe the crash in detail.

**Describe the crash**
A clear and concise description of the crash.

**To Reproduce**
Steps to reproduce the crash:
1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. Explorer++ crashes

**Screenshots**
If applicable, add screenshots to help explain your problem.

**Version (please complete the following information):**
 - Explorer++ version: [e.g. 1.4.0.1616 dev (64-bit build)]
 - OS: [e.g. Windows 10]

**Additional context**
Add any other context about the problem here.
