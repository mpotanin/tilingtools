@echo off
rem %1 D:\TeamCityTestData\
rem %2 D:\TeamCity\buildAgent\work\160ea70fbfb7de1f

:test1
echo ##teamcity[testStarted name='Compare Files1']
set fileSource1=%1\TilingTools\5_16_8.jpg
set fileGenerated1=%2\testOutput\copy1\5\16\5_16_8.jpg
fc %fileSource1% %fileGenerated1% > nul
if errorlevel 1 goto errortest1
:nexttest1
echo ##teamcity[testFinished name='Compare Files1']
goto test2
:errortest1
echo ##teamcity[testFailed name='Compare Files1']

:test2
echo ##teamcity[testStarted name='Compare Files2']
set fileSource2=%1\TilingTools\5_32_7.png
set fileGenerated2=%2\testOutput\copy2\5_32_7.png
fc %fileSource2% %fileGenerated2% > nul
if errorlevel 1 goto errortest2
:nexttest2
echo ##teamcity[testFinished name='Compare Files2']
goto test3
:errortest2
echo ##teamcity[testFailed name='Compare Files2']

:test3
