#pragma once

#ifdef _WIN32
    #include<windows.h>
#elif __linux__

#else
    #error "Unknown"
#endif 

#include<type_traits>
#include<memory>
#include<iostream>
template <auto F>
using Functor = std::integral_constant<std::remove_reference_t<decltype(F)>, F>;

template<class T, class FreeFunc>
struct AutoPtr
{
    using _Type = std::remove_reference_t<T>;
    static_assert(std::is_invocable_v< FreeFunc::value_type, _Type**> || std::is_invocable_v<FreeFunc::value_type, _Type*>);
    constexpr static bool isSecPtr = std::is_invocable_v<FreeFunc::value_type, _Type**>;

    AutoPtr() noexcept {}
    AutoPtr(AutoPtr& Autoptr) = delete;
    AutoPtr(AutoPtr&& Autoptr) noexcept : _ptr(Autoptr.release()) {}
    AutoPtr(_Type* ptr) noexcept : _ptr(ptr) {}
    //~AutoPtr() { std::cout << typeid(*this).name() << std::endl; }

    void operator=(_Type* ptr) noexcept { _ptr.reset(ptr); }
    AutoPtr& operator=(AutoPtr&& Autoptr) noexcept { _ptr.reset(Autoptr.release()); return *this; }

    operator const _Type* () const noexcept { return _ptr.get(); }
    operator _Type*& () noexcept { return *reinterpret_cast<_Type**>(this); }
    operator bool() const noexcept { return _ptr.get() != nullptr; }

    _Type** operator&() { static_assert(sizeof(*this) == sizeof(void*)); return reinterpret_cast<_Type**>(this); }

    _Type* operator->() const noexcept { return _ptr.get(); }

    void reset(_Type* ptr = nullptr) noexcept { _ptr.reset(ptr); }

    void swap()noexcept { _ptr.swap(); }

    auto release() noexcept { return _ptr.release(); }
    auto get() const noexcept { return _ptr.get(); }
private:
    struct DeletePrimaryPtr { void operator()(void* ptr) { FreeFunc()(static_cast<_Type*>(ptr)); } };
    struct DeleteSecPtr { void operator()(void* ptr) { FreeFunc()(reinterpret_cast<_Type**>(&ptr)); } };
    using DeletePtr = std::conditional<isSecPtr, DeleteSecPtr, DeletePrimaryPtr>::type;
    std::unique_ptr<_Type, DeletePtr> _ptr;
};

template<class _T1>
inline void reverse_bit(std::remove_reference_t<_T1>& val, std::remove_reference_t<_T1> local) noexcept 
    requires std::is_pod_v<std::remove_reference_t<_T1>>
{
    val = (val & local) ? (val & (~local)) : (val | local);
}
