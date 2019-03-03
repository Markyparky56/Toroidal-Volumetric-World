#pragma once
#include "dualmc.h"

class SurfaceExtractor
{
public:
  SurfaceExtractor();
  ~SurfaceExtractor();
private:
  dualmc::DualMC<uint16_t> dmc;
};
