#pragma once
// Based on Vulkan Cookbook's VulkanDestroyer Class
#include <utility>
#include "VulkanInterface.Functions.hpp"

namespace VulkanInterface
{
  template<class Obj>
  class VulkanHandle 
  {
  public:
    VulkanHandle() 
      : Device(nullptr)
      , Object(nullptr)
    {}

    VulkanHandle(VkDevice device)
      : Device(device)
      , Object(nullptr)
    {}

    VulkanHandle(VulkanHandle<VkDevice> const & device)
      : Device(*device)
      , Object(nullptr)
    {}

    VulkanHandle(VkDevice device, Obj object)
      : Device(device)
      , Object(object)
    {}

    VulkanHandle(VulkanHandle<VkDevice> const & device, Obj object)
      : Device(*device)
      , Object(object)
    {}

    ~VulkanHandle()
    {
      if ((Device != nullptr) && (Object != nullptr))
      {
        Destroy();
      }
    }

    VulkanHandle(VulkanHandle<Obj> && other)
      : VulkanHandle()
    {
      *this = std::move(other);
    }

    VulkanHandle & operator=(VulkanHandle<Obj> && other)
    {
      if (&other != this)
      {
        VkDevice device = Device;
        Obj object = Object;
        Device = other.Device;
        Object = other.Object;
        other.Device = device;
        other.Object = object;
      }
      return *this;
    }

    Obj & Get()
    {
      return Object;
    }

    Obj * GetPtr()
    {
      return &Object;
    }

    Obj & operator*()
    {
      return Object;
    }

    Obj const & operator*() const 
    {
      return Object;
    }

    bool operator !() const 
    {
      return Object == nullptr;
    }

    explicit operator bool() const
    {
      return Object != nullptr;
    }

  private:
    VulkanHandle(VulkanHandle<Obj> const &);
    VulkanHandle& operator=(VulkanHandle<Obj> const &);
    void Destroy();

    VkDevice Device;
    Obj Object;
  };

  // VkInstance specialisation
  template<>
  class VulkanHandle<VkInstance>
  {
  public:
    VulkanHandle()
      : Instance(nullptr)
    {}

    VulkanHandle(VkInstance object)
      : Instance(object)
    {}

    VulkanHandle()
    {
      if (Instance != nullptr)
      {
        Destroy();
      }
    }

    VulkanHandle(VulkanHandle<VkInstance> && other)
    {
      *this = std::move(other);
    }

    VulkanHandle & operator=(VulkanHandle<VkInstance> && other)
    {
      if (&other != this)
      {
        Instance = other.Instance;
        other.Instance = nullptr;
      }
      return *this;
    }

    VkInstance & Get()
    {
      return Instance;
    }

    VkInstance * GetPtr()
    {
      return &Instance;
    }

    VkInstance & operator*()
    {
      return Instance;
    }

    VkInstance const & operator*() const 
    {
      return Instance;
    }

    bool operator !() const 
    {
      return Instance == nullptr;
    }

    explicit operator bool() const
    {
      return Instance != nullptr;
    }

  private:
    VulkanHandle(VulkanHandle<VkInstance> const &);
    VulkanHandle & operator=(VulkanHandle<VkInstance> const &);
    void Destroy();

    VkInstance Instance;
  };

  // VkDevice specialisation
  template<>
  class VulkanHandle<VkDevice>
  {
  public:
    VulkanHandle()
      : LogicalDevice(nullptr)
    {}

    VulkanHandle(VkDevice object)
      : LogicalDevice(object)
    {}

    ~VulkanHandle()
    {
      if (LogicalDevice != nullptr)
      {
        Destroy();
      }
    }

    VulkanHandle(VulkanHandle<VkDevice> && other)
    {
      *this = std::move(other);
    }

    VulkanHandle & operator=(VulkanHandle<VkDevice> && other)
    {
      if (&other != this)
      {
        LogicalDevice = other.LogicalDevice;
        other.LogicalDevice = nullptr;
      }
      return *this;
    }

    VkDevice & Get()
    {
      return LogicalDevice;
    }

    VkDevice * GetPtr()
    {
      return &LogicalDevice;
    }

    VkDevice & operator*()
    {
      return LogicalDevice;
    }

    VkDevice const & operator*() const
    {
      return LogicalDevice;
    }

    bool operator !() const
    {
      return LogicalDevice == nullptr;
    }

    explicit operator bool() const
    {
      return LogicalDevice != nullptr;
    }


  private:
    VulkanHandle(VulkanHandle<VkDevice> const &);
    VulkanHandle & operator=(VulkanHandle<VkDevice> const &);
    void Destroy();

    VkDevice LogicalDevice;
  };

  // VkSurfaceKHR specialisation
  template<>
  class VulkanHandle<VkSurfaceKHR>
  {
  public:
    VulkanHandle()
      : Instance(nullptr)
      , Object(nullptr)
    {}

    VulkanHandle(VkInstance instance)
      : Instance(instance)
      , Object(nullptr)
    {}

    VulkanHandle(VulkanHandle<VkInstance> const & instance)
      : Instance(*instance)
      , Object(nullptr)
    {}

    VulkanHandle(VkInstance instance, VkSurfaceKHR object)
      : Instance(instance)
      , Object(object)
    {}

    VulkanHandle(VulkanHandle<VkInstance> const & instance, VkSurfaceKHR object)
      : Instance(*instance)
      , Object(object)
    {}

    ~VulkanHandle()
    {
      if ((Instance != nullptr) && (Object != nullptr))
      {
        Destroy();
      }
    }

    VulkanHandle(VulkanHandle<VkSurfaceKHR> && other)
    {
      *this = std::move(other);
    }

    VulkanHandle & operator=(VulkanHandle<VkSurfaceKHR> && other)
    {
      if (&other != this)
      {
        Instance = other.Instance;
        Object = other.Object;
        other.Instance = nullptr;
        other.Object = nullptr;
      }
      return *this;
    }

    VkSurfaceKHR & Get()
    {
      return Object;
    }

    VkSurfaceKHR * GetPtr()
    {
      return &Object;
    }

    VkSurfaceKHR & operator*()
    {
      return Object;
    }

    VkSurfaceKHR const & operator*() const 
    {
      return Object;
    }

    bool operator !() const
    {
      return Object == nullptr;
    }

    explicit operator bool() const
    {
      return Object != nullptr;
    }

  private:
    VulkanHandle(VulkanHandle<VkSurfaceKHR> const &);
    VulkanHandle & operator=(VulkanHandle<VkSurfaceKHR> const &);
    void Destroy();

    VkInstance Instance;
    VkSurfaceKHR Object;
  };

  template<class Obj>
  inline bool operator==(VulkanHandle<Obj> const & lhs, nullptr_t null)
  {
    return !lhs;
  }

  template<class Obj>
  inline bool operator==(nullptr_t null, VulkanHandle<Obj> const & rhs)
  {
    return !rhs;
  }

  template<class Obj>
  inline bool operator!=(VulkanHandle<Obj> const & lhs, nullptr_t null)
  {
    return !!lhs;
  }

  template<class Obj>
  inline bool operator!=(nullptr_t null, VulkanHandle<Obj> const & rhs)
  {
    return !!rhs;
  }

  template<class Parent, class Obj>
  void InitVulkanHandle(Parent const & parent, Obj obj, VulkanHandle<Obj> & wrapper)
  {
    wrapper = VulkanHandle<Obj>(parent, obj);
  }

  template<class Parent, class Obj>
  void InitVulkanHandle(Parent const & parent, VulkanHandle<Obj> & wrapper)
  {
    wrapper = VulkanHandle<Obj>(parent);
  }
}
