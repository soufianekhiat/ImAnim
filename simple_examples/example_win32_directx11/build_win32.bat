@REM ImAnim example: Win32 + DirectX 11
@REM Build script for Windows (requires Visual Studio)

@echo off
setlocal

set IMGUI_DIR=..\..\extern\imgui
set IMANIM_DIR=..\..\src
set DEMO_DIR=..\..\demo

cl /nologo /Zi /MD ^
    /I%IMGUI_DIR% /I%IMGUI_DIR%\backends /I%IMANIM_DIR% ^
    main.cpp ^
    %IMGUI_DIR%\imgui.cpp ^
    %IMGUI_DIR%\imgui_draw.cpp ^
    %IMGUI_DIR%\imgui_tables.cpp ^
    %IMGUI_DIR%\imgui_widgets.cpp ^
    %IMGUI_DIR%\backends\imgui_impl_win32.cpp ^
    %IMGUI_DIR%\backends\imgui_impl_dx11.cpp ^
    %IMANIM_DIR%\im_anim.cpp ^
    %DEMO_DIR%\demo_im_anim.cpp ^
    /Fe:im_anim_example.exe ^
    /link d3d11.lib d3dcompiler.lib

endlocal
