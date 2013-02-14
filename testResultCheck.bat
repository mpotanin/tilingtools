@echo off

rem %1 D:\TeamCityTestData\
rem %2 D:\TeamCity\buildAgent\work\160ea70fbfb7de1f

:test0
echo ##teamcity[testStarted name='.tiles exist']
set filename="%2\testOutput\scn_120719_Vrangel_island_SWA.tiles"

if exist %filename% goto nexttest0
:nexttest0
echo ##teamcity[testFinished name='.tiles exist']
goto test1
:errortest0
echo  ##teamcity[testFailed name='.tiles exist']


:test1
echo ##teamcity[testStarted name='Check .tiles file size']
set tileFileSize=0
for %%A in (%filename%) do set tileFileSize= %%~zA 
echo %tileFileSize%

if %tileFileSize% == 3978095 goto nexttest1
:errortest1
echo ##teamcity[testFailed name='Check .tiles file size']
goto test2

:nexttest1
echo ##teamcity[testFinished name='Check .tiles file size']



:test2
echo ##teamcity[testStarted name='Compare Files']
set fileSource=%1\TilingTools\5_16_8.jpg
set fileGenerated=%2\testOutput\copy1\5\16\5_16_8.jpg
fc %fileSource% %fileGenerated% > nul
if errorlevel 1 goto errortest2
:nexttest2
echo ##teamcity[testFinished name='Compare Files']
goto test3
:errortest2
echo ##teamcity[testFailed name='Compare Files']

:test3
