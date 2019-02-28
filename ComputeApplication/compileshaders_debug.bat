%VULKAN_SDK%\Bin32\glslc.exe -fshader-stage=vertex -o chunk_directionalLight.vert.spv chunk_directionalLight.vert
%VULKAN_SDK%\Bin32\glslc.exe -fshader-stage=fragment -o chunk_directionalLight.frag.spv chunk_directionalLight.frag
:: %VULKAN_SDK%\Bin32\glslc.exe -fshader-stage=compute -o compute.spv basic.comp
robocopy . ../x64/Debug/Data/ vert.spv frag.spv compute.spv
EXIT /B 0