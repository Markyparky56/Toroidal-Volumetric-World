%VULKAN_SDK%\Bin32\glslc.exe -fshader-stage=vertex -o vert.spv basic.vert
%VULKAN_SDK%\Bin32\glslc.exe -fshader-stage=fragment -o frag.spv basic.frag
%VULKAN_SDK%\Bin32\glslc.exe -fshader-stage=compute -o compute.spv basic.comp
robocopy . ../x64/Debug/Data/ vert.spv frag.spv compute.spv
EXIT /B 0