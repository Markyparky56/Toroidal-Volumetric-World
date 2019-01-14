%VULKAN_SDK%\Bin32\glslc.exe -O -fshader-stage=vertex -o vert.spv vert.glsl
%VULKAN_SDK%\Bin32\glslc.exe -O -fshader-stage=fragment -o frag.spv frag.glsl
robocopy . ../x64/Release/Data/ vert.spv frag.spv
EXIT /B 0