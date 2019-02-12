%VULKAN_SDK%\Bin32\glslc.exe -O -fshader-stage=vertex -o vert.spv basic.vert
%VULKAN_SDK%\Bin32\glslc.exe -O -fshader-stage=fragment -o frag.spv basic.frag
%VULKAN_SDK%\Bin32\glslc.exe -O -fshader-stage=compute -o compute.spv basic.comp
robocopy . ../x64/Release/Data/ vert.spv frag.spv compute.spv
EXIT /B 0