@echo off

set InputMMLFile="sample.txt"
set OutputSPCFile="sample.spc"

rem ------------------------------------------------------

spcmake_byFF5.exe %InputMMLFile% %OutputSPCFile%

if %ERRORLEVEL%==0 (
   start sample.spc
   exit
) else (
   pause
)
