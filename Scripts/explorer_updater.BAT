@ECHO OFF
REM # encoding: IBM850
: DATE: 28/04/2020 12:36
: FUNC: Update Explorer++ (explorerplusplus.com) and log it in explorer_updater.log 
: Needs '7za' tool (7-zip.org) in the path

ECHO ===========================================================
ECHO Explorer++ updater v1.0
If NOT EXIST Explorer++.exe GOTO letsgo
FOR /f "delims=" %%A in ('powershell "(Get-Item -path 'Explorer++.exe').VersionInfo.ProductVersion"') DO SET oldVer=%%A
ECHO Actual Explorer++ version: v%oldVer%

:letsgo
IF %1d == d GOTO sintax
TASKLIST /FI "IMAGENAME eq Explorer++.exe" | FINDSTR /I "Explorer++.exe" > nul
IF "%ERRORLEVEL%" == "0" GOTO close
GOTO extract

:sintax
ECHO ===========================================================
ECHO SINTAX: explorer_updater [filename.zip]
ECHO         zip file - the new downloaded version
ECHO -----------------------------------------------------------
ECHO You can drag and drop the .zip onto this .bat updater from 
ECHO Windows Explorer or use the CMD with the parameter
ECHO         TIP: The updater will expand the .zip where it's
ECHO              saved, not where the .bat is
ECHO ===========================================================
GOTO end

:close
SET task=y
SET /p task=WARNING: Explorer++ is running. Close it now [y]/n ?  
IF %task% == n GOTO end
TASKKILL /IM Explorer++.exe

:extract
WHERE 7za.exe >nul 2>&1 && ECHO Processing... || ECHO ERROR: The '7za.exe' tool not found! && GOTO end
7za.exe e %1 -y
FOR /f "delims=" %%A in ('powershell "(Get-Item -path 'Explorer++.exe').VersionInfo.ProductVersion"') DO SET newVer=%%A
ECHO New Explorer++ version installed: v%newVer%
ECHO [%date:~0,2%-%date:~3,2%-%date:~6,4% %time:~0,2%:%time:~3,2%:%time:~6,2%] - %oldVer% =^> %newVer% >> explorer_updater.log 

:end
ECHO= & ECHO FIN del proceso
PAUSE
