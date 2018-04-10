REM AppVeyor treats a non-zero return code as an error. This is an issue, as the
REM hhc executable returns 1 on success and 0 on failure. This script only
REM exists to invert the error code that's returned.

hhc "%APPVEYOR_BUILD_FOLDER%\Documentation\User\Help Manual\explorer++.hhp"

IF NOT errorlevel 1 (
  EXIT /B 1
) ELSE (
  EXIT /B 0
)