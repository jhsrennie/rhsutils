if "%1" == "" then goto noarg

cd "%1"
nmake
cd ..

:noarg
