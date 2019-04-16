::%VULKAN_SDK%\Bin32\glslc.exe -O -fshader-stage=vertex -o vert.spv basic.vert
::%VULKAN_SDK%\Bin32\glslc.exe -O -fshader-stage=fragment -o frag.spv basic.frag
::%VULKAN_SDK%\Bin32\glslc.exe -O -fshader-stage=compute -o compute.spv basic.comp
%VULKAN_SDK%\Bin32\glslangValidator.exe --target-env vulkan1.0 -o vert.spv chunk_directionalLight.vert
%VULKAN_SDK%\Bin32\glslangValidator.exe --target-env vulkan1.0 -o frag.spv chunk_directionalLight.frag
::robocopy . ../x64/Release/Data/ vert.spv frag.spv compute.spv
robocopy . ../x64/Release/Data/ vert.spv frag.spv
EXIT /B 0