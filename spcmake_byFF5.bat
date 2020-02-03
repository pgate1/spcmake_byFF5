@echo off

set InputMMLFile="sample.txt"
set OutputSPCFile="sample.spc"

rem ------------------------------------------------------
rem -i input_filename
rem -o output_filename
rem -ticks ticksの表示
rem -brr2wav brr2wavフォルダに各音源のwavを生成

spcmake_byFF5.exe -i %InputMMLFile% -o %OutputSPCFile%

if %ERRORLEVEL%==0 (
   start sample.spc
   exit
) else (
   echo.
   pause
)
