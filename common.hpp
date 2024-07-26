#pragma once

#ifdef _WIN32
    #include<windows.h>
#elif __linux__

#else
    #error "Unknown"
#endif 

#include<type_traits>
#include<memory>

template <auto F>
using Functor = std::integral_constant<std::remove_reference_t<decltype(F)>, F>;

template<class T, class FreeFunc, bool isSecPtr = false>
struct AutoPtr
{
    AutoPtr() noexcept {}
    AutoPtr(AutoPtr& Autoptr) = delete;
    explicit AutoPtr(AutoPtr&& Autoptr) noexcept : _ptr(Autoptr.release()) {}
    AutoPtr(T* ptr) noexcept : _ptr(ptr) {}


    void operator=(T* ptr) noexcept { _ptr.reset(ptr); }
    void operator=(AutoPtr&& Autoptr) noexcept { _ptr.reset(Autoptr.release()); }


    operator const T* () const noexcept { return _ptr.get(); }
    operator T*& () noexcept { return *reinterpret_cast<T**>(this); }
    operator bool() const noexcept { return _ptr.get() != nullptr; }


    T** operator&() { static_assert(sizeof(*this) == sizeof(void*)); return reinterpret_cast<T**>(this); }
 
    T* operator->() const noexcept { return _ptr.get(); }



    void reset(T* ptr = nullptr) noexcept { _ptr.reset(ptr); }
    T* release() noexcept { return _ptr.release(); }
    T* get() const noexcept { return _ptr.get(); }
private:
    struct DeletePrimaryPtr { void operator()(void* ptr) { FreeFunc()(static_cast<T*>(ptr)); } };
    struct DeleteSecPtr { void operator()(void* ptr) { FreeFunc()(reinterpret_cast<T**>(&ptr)); } };
    using DeletePtr = std::conditional<isSecPtr, DeleteSecPtr, DeletePrimaryPtr>::type;
    std::unique_ptr<T, DeletePtr> _ptr;
};


template<class _T1>
inline void reverse_bit(_T1& val,_T1 local) noexcept requires std::is_pod_v<_T1>
{
    val = (val & local)?(val & (~local)):(val | local);
}
