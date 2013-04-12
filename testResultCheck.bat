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
echo ##teamcity[testStarted name='Compare Files3']
set fileSource3=%1\TilingTools\6_0_16.png
set fileGenerated3=%2\testOutput\copy3\6\0\6_0_16.png
fc %fileSource3% %fileGenerated3% > nul
if errorlevel 1 goto errortest3
:nexttest3
echo ##teamcity[testFinished name='Compare Files3']
goto test4
:errortest3
echo ##teamcity[testFailed name='Compare Files3']

:test4
echo ##teamcity[testStarted name='Compare Files4']
set fileSource4=%1\TilingTools\6_-32_15.jpg
set fileGenerated4=%2\testOutput\copy4\6\-32\6_-32_15.jpg
fc %fileSource4% %fileGenerated4% > nul
if errorlevel 1 goto errortest4
:nexttest4
echo ##teamcity[testFinished name='Compare Files4']
goto test5
:errortest4
echo ##teamcity[testFailed name='Compare Files4']

:test5
echo ##teamcity[testStarted name='Compare Files5']
set fileSource5=%1\TilingTools\16_19307_24127.png
set fileGenerated5=%2\testOutput\o42073g8_tiles\16\19307\16_19307_24127.png
fc %fileSource5% %fileGenerated5% > nul
if errorlevel 1 goto errortest5
:nexttest5
echo ##teamcity[testFinished name='Compare Files5']
goto test6
:errortest5
echo ##teamcity[testFailed name='Compare Files5']

:test6
echo ##teamcity[testStarted name='Compare Files6']
set fileSource6=%1\TilingTools\16_19307_24127_2.png
set fileGenerated6=%2\testOutput\schenectady_tiles\16\19307\16_19307_24127.png
fc %fileSource6% %fileGenerated6% > nul
if errorlevel 1 goto errortest6
:nexttest6
echo ##teamcity[testFinished name='Compare Files6']
goto test7
:errortest6
echo ##teamcity[testFailed name='Compare Files6']

:test7
echo ##teamcity[testStarted name='Compare Files7']
set fileSource6=%1\TilingTools\11_275_299.jpg
set fileGenerated6=%2\testOutput\Eros-B_16bit_cut_tiles\11\275\11_275_299.jpg
fc %fileSource6% %fileGenerated6% > nul
if errorlevel 1 goto errortest7
:nexttest7
echo ##teamcity[testFinished name='Compare Files7']
goto test8
:errortest7
echo ##teamcity[testFailed name='Compare Files7']

:test8