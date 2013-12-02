copy "..\tilingtools.config" "..\release\tilingtools.config"
copy "..\msvcp100.dll" "..\release\msvcp100.dll"
copy "..\msvcr71.dll" "..\release\msvcr71.dll"
copy "..\msvcr100.dll" "..\release\msvcr100.dll"
copy "..\openjp2.dll" "..\release\openjp2.dll"
del Release/version*.txt
echo.%DATE%>Release/version_%DATE%.txt
