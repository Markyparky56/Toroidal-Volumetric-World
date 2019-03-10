%VULKAN_SDK%\Bin32\glslangValidator.exe --target-env vulkan1.0 -g -o vert.spv chunk_directionalLight.vert
%VULKAN_SDK%\Bin32\glslangValidator.exe --target-env vulkan1.0 -g -o frag.spv chunk_directionalLight.frag
::%VULKAN_SDK%\Bin32\glslangValidator.exe --target-env vulkan1.0 -g -e main --vn chunk_directionalLightVertSPV -o chunkDirectionalLight.vert.spv.h chunk_directionalLight.vert
::%VULKAN_SDK%\Bin32\glslangValidator.exe --target-env vulkan1.0 -g -e main --vn chunk_directionalLightFragSPV -o chunkDirectionalLight.frag.spv.h chunk_directionalLight.frag
:: %VULKAN_SDK%\Bin32\glslc.exe -fshader-stage=compute -o compute.spv basic.comp

::%VULKAN_SDK%\Bin32\glslc.exe -O -fshader-stage=vertex -o chunk_directionalLight.vert.spv  vert.vert
::%VULKAN_SDK%\Bin32\glslc.exe -O -fshader-stage=fragment -o chunk_directionalLight.frag.spv  frag.frag

robocopy . ../x64/Debug/Data/ vert.spv frag.spv
EXIT /B 0