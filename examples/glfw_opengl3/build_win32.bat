@REM ImAnim example: GLFW + OpenGL3
@REM Build script for Windows (requires Visual Studio)

@echo off
setlocal

set IMGUI_DIR=..\extern\imgui
set IMANIM_DIR=..\..
set GLFW_DIR=%IMGUI_DIR%\examples\libs\glfw

cl /nologo /Zi /MD ^
    /I%IMGUI_DIR% /I%IMGUI_DIR%\backends /I%IMANIM_DIR% /I%GLFW_DIR%\include ^
    main.cpp ^
    %IMGUI_DIR%\imgui.cpp ^
    %IMGUI_DIR%\imgui_draw.cpp ^
    %IMGUI_DIR%\imgui_tables.cpp ^
    %IMGUI_DIR%\imgui_widgets.cpp ^
    %IMGUI_DIR%\backends\imgui_impl_glfw.cpp ^
    %IMGUI_DIR%\backends\imgui_impl_opengl3.cpp ^
    %IMANIM_DIR%\im_anim.cpp ^
    %IMANIM_DIR%\im_anim_demo.cpp ^
    %IMANIM_DIR%\im_anim_doc.cpp ^
    /Fe:im_anim_example.exe ^
    /link /LIBPATH:%GLFW_DIR%\lib-vc2010-64 glfw3.lib opengl32.lib gdi32.lib shell32.lib

endlocal
