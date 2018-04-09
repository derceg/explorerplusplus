hhc "%APPVEYOR_BUILD_FOLDER%\Documentation\User\Help Manual\explorer++.hhp"

IF NOT errorlevel 1 (
  EXIT /B 1
) ELSE (
  EXIT /B 0
)