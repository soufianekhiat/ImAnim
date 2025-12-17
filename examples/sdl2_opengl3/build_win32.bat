@REM ImAnim example: SDL2 + OpenGL3
@REM Build script for Windows (requires Visual Studio)

@echo off
setlocal

set IMGUI_DIR=..\extern\imgui
set IMANIM_DIR=..\..

@REM Adjust SDL2 path as needed
set SDL2_DIR=C:\libs\SDL2

cl /nologo /Zi /MD ^
    /I%IMGUI_DIR% /I%IMGUI_DIR%\backends /I%IMANIM_DIR% /I%SDL2_DIR%\include ^
    main.cpp ^
    %IMGUI_DIR%\imgui.cpp ^
    %IMGUI_DIR%\imgui_draw.cpp ^
    %IMGUI_DIR%\imgui_tables.cpp ^
    %IMGUI_DIR%\imgui_widgets.cpp ^
    %IMGUI_DIR%\backends\imgui_impl_sdl2.cpp ^
    %IMGUI_DIR%\backends\imgui_impl_opengl3.cpp ^
    %IMANIM_DIR%\im_anim.cpp ^
    %IMANIM_DIR%\im_anim_demo.cpp ^
    %IMANIM_DIR%\im_anim_doc.cpp ^
    /Fe:im_anim_example.exe ^
    /link /LIBPATH:%SDL2_DIR%\lib\x64 SDL2.lib SDL2main.lib opengl32.lib shell32.lib /SUBSYSTEM:CONSOLE

endlocal
