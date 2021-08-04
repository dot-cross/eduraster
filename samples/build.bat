echo OFF

echo Building EduRaster samples

SETLOCAL

set SDL_HEADER_PATH=..\external\SDL2-2.0.14\i686-w64-mingw32\include\SDL2
set SDL_LIB_PATH=..\external\SDL2-2.0.14\i686-w64-mingw32\lib
set SDL_RUNTIME_PATH=..\external\SDL2-2.0.14-win32-x86
set LINK_LIBS=-leduraster -lmingw32 -lSDL2main -lSDL2 -lm

echo EduRaster lib

gcc -c -I..\include ..\src\*.c -lm -O2

ar -r libeduraster.a *.o

del *.o

echo Single triangle

gcc -I..\include -I%SDL_HEADER_PATH% -L. -L%SDL_LIB_PATH% triangle.c -o triangle %LINK_LIBS%

echo Surface Plot

gcc -I..\include -Isimple-parser -I%SDL_HEADER_PATH% -L. -L%SDL_LIB_PATH% surfaceplot.c  simple-parser\simple_parser.c -o splot %LINK_LIBS%

echo Tubeplot

gcc -I..\include -Isimple-parser -I%SDL_HEADER_PATH% -L. -L%SDL_LIB_PATH% tubeplot.c  simple-parser\simple_parser.c -o tplot %LINK_LIBS%

echo Texture Cube

gcc -I..\include -I%SDL_HEADER_PATH% -L. -L%SDL_LIB_PATH% texcube.c -o texcube %LINK_LIBS%

echo Environment Mapping

gcc -I..\include -I%SDL_HEADER_PATH% -L. -L%SDL_LIB_PATH% envmap.c -o envmap %LINK_LIBS% -O2

echo Tunnel Effect

gcc -I..\include -I%SDL_HEADER_PATH% -L. -L%SDL_LIB_PATH% tunnel.c -o tunnel  %LINK_LIBS% -O2

echo Copying SDL.dll

copy %SDL_RUNTIME_PATH%\SDL2.dll

ENDLOCAL

echo Done!
