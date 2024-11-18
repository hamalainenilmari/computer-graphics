@echo off

if "%cs3100_renderer"=="" call set_renderer_debug
if not exist out mkdir out

echo Using renderer at %cs3100_renderer%

set inputfile=%1
echo %inputfile%
set scenename=%inputfile:.txt=%
rem depth must be last argument passed from the caller
%cs3100_renderer% -input %* %scenename%_depth.png -output %scenename%_color.png -normals %scenename%_normals.png
move %scenename%_color.png out\
move %scenename%_depth.png out\
move %scenename%_normals.png out\
