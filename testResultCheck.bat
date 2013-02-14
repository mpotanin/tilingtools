@echo off
rem remember the directory path to this bat file
set dirPath=%~dp0

rem need to reverse windows names to posix names by changing \ to /
set dirPath=%dirPath:\=/%
rem remove blank at end of string
set dirPath=%dirPath:~0,-1%

rem - Customize for your installation, for instance you might want to add default parameters like the following:
java -jar "%dirPath%"/lib/jira-cli-3.1.0.jar --server http://5.9.141.80:8080 --user dkudelko --password JR4545hj %*

rem Exit with the correct error level.
EXIT /B %ERRORLEVEL%
