@echo off

set InputMMLFile="sample.txt"
set OutputSPCFile="sample.spc"

rem ------------------------------------------------------
rem -i input_filename
rem -o output_filename
rem -ticks ticks�̕\��
rem -brr2wav brr2wav�t�H���_�Ɋe������wav�𐶐�

spcmake_byFF5.exe -i %InputMMLFile% -o %OutputSPCFile%

if %ERRORLEVEL%==0 (
   start sample.spc
   exit
) else (
   echo.
   pause
)
