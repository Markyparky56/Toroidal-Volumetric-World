#pragma once
struct float5_s { float x, y, z, w, v; };
struct float4_s { float x, y, z, w; };
struct float3_s {
  float x, y, z;
  float3_s operator+=(float3_s const rhs)
  {
    x += rhs.x; y += rhs.y; z += rhs.z;
    return *this;
  }
};
struct float2_s {
  float x, y;
  float2_s operator+=(float2_s const rhs)
  {
    x += rhs.x; y += rhs.y;
    return *this;
  }
};
inline float5_s operator-(float5_s const lhs, float5_s const rhs)
{
  return {
      lhs.x - rhs.x,
      lhs.y - rhs.y,
      lhs.z - rhs.z,
      lhs.w - rhs.w,
      lhs.v - rhs.v
  };
}
inline float5_s operator+(float5_s const lhs, float5_s const rhs)
{
  return {
      lhs.x + rhs.x,
      lhs.y + rhs.y,
      lhs.z + rhs.z,
      lhs.w + rhs.w,
      lhs.v + rhs.v
  };
}
inline float4_s operator-(float4_s const lhs, float4_s const rhs)
{
  return {
      lhs.x - rhs.x,
      lhs.y - rhs.y,
      lhs.z - rhs.z,
      lhs.w - rhs.w
  };
}
inline float3_s operator-(float3_s const lhs, float3_s const rhs)
{
  return {
      lhs.x - rhs.x,
      lhs.y - rhs.y,
      lhs.z - rhs.z
  };
}
inline float2_s operator-(float2_s const lhs, float2_s const rhs)
{
  return {
      lhs.x - rhs.x,
      lhs.y - rhs.y
  };
}

struct int5_s { int x, y, z, w, v; };
struct int4_s { int x, y, z, w; };
struct int3_s {
  int x, y, z;
  int3_s operator+=(int3_s const rhs)
  {
    x += rhs.x; y += rhs.y; z += rhs.z;
    return *this;
  }
};
struct int2_s {
  int x, y;
  int2_s operator+=(int2_s const rhs)
  {
    x += rhs.x; y += rhs.y;
    return *this;
  }
};

inline int4_s operator-(int4_s const lhs, int4_s const rhs)
{
  return {
      lhs.x - rhs.x,
      lhs.y - rhs.y,
      lhs.z - rhs.z,
      lhs.w - rhs.w
  };
}
inline int3_s operator-(int3_s const lhs, int3_s const rhs)
{
  return {
      lhs.x - rhs.x,
      lhs.y - rhs.y,
      lhs.z - rhs.z
  };
}
inline int2_s operator-(int2_s const lhs, int2_s const rhs)
{
  return {
      lhs.x - rhs.x,
      lhs.y - rhs.y
  };
}
inline int4_s operator+(int4_s const lhs, int4_s const rhs)
{
  return {
      lhs.x + rhs.x,
      lhs.y + rhs.y,
      lhs.z + rhs.z,
      lhs.w + rhs.w
  };
}
inline int3_s operator+(int3_s const lhs, int3_s const rhs)
{
  return {
      lhs.x + rhs.x,
      lhs.y + rhs.y,
      lhs.z + rhs.z
  };
}
inline int2_s operator+(int2_s const lhs, int2_s const rhs)
{
  return {
      lhs.x + rhs.x,
      lhs.y + rhs.y
  };
}

inline float sum(float5_s a) { return a.x + a.y + a.z + a.w + a.v; }
inline float sum(float4_s a) { return a.x + a.y + a.z + a.w; }
inline float sum(float3_s a) { return a.x + a.y + a.z; }
inline float sum(float2_s a) { return a.x + a.y; }
inline int sum(int5_s a) { return a.x + a.y + a.z + a.w + a.v; }
inline int sum(int4_s a) { return a.x + a.y + a.z + a.w; }
inline int sum(int3_s a) { return a.x + a.y + a.z; }
inline int sum(int2_s a) { return a.x + a.y; }

inline int5_s step(float5_s edge, float5_s x)
{
  return {
      x.x <= edge.x ? 0 : 1,
      x.y <= edge.y ? 0 : 1,
      x.z <= edge.z ? 0 : 1,
      x.w <= edge.w ? 0 : 1,
      x.v <= edge.v ? 0 : 1
  };
}
inline int4_s step(float4_s edge, float4_s x)
{
  return {
      x.x <= edge.x ? 0 : 1,
      x.y <= edge.y ? 0 : 1,
      x.z <= edge.z ? 0 : 1,
      x.w <= edge.w ? 0 : 1
  };
}
inline int3_s step(float3_s edge, float3_s x)
{
  return {
      x.x <= edge.x ? 0 : 1,
      x.y <= edge.y ? 0 : 1,
      x.z <= edge.z ? 0 : 1
  };
}
inline int2_s step(float2_s edge, float2_s x)
{
  return {
      x.x <= edge.x ? 0 : 1,
      x.y <= edge.y ? 0 : 1,
  };
}

inline float4_s stepf(float4_s edge, float4_s x)
{
  return {
      x.x <= edge.x ? 0.0f : 1.f,
      x.y <= edge.y ? 0.0f : 1.f,
      x.z <= edge.z ? 0.0f : 1.f,
      x.w <= edge.w ? 0.0f : 1.f
  };
}
inline float3_s stepf(float3_s edge, float3_s x)
{
  return {
      x.x <= edge.x ? 0.0f : 1.f,
      x.y <= edge.y ? 0.0f : 1.f,
      x.z <= edge.z ? 0.0f : 1.f
  };
}

template<typename T>
float magnitudeSqrd(T const f)
{
  return (f.x * f.x) + (f.y * f.y) + (f.z * f.z) + (f.w * f.w) + (f.v * f.v);
}