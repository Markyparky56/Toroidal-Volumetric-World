#include <array>
#include <memory>
#include <vector>

int main()
{
  struct voxel { float d; };
  using parr = std::unique_ptr<std::array<voxel, 36 * 36 * 36>>;
  
  std::vector<parr> arrvec;
  for (int i = 0; i < 125; i++)
  {
    arrvec.emplace_back(new std::array<voxel, 36 * 36 * 36>());
  }

  return 0;
}