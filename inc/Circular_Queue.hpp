#pragma once
#include<memory>
#include<atomic>
#include<common.hpp>
#include<mutex>
#include<assert.h>

template<class _T, class _DeleteFunctionType = nullptr_t>
class Circular_Queue
{
	using _Type = std::remove_reference<_T>::type;
	using _ElementPtrType = std::conditional_t<std::is_null_pointer_v<_DeleteFunctionType>, std::unique_ptr<_Type>, AutoPtr<_Type, _DeleteFunctionType>>;
public:

	explicit Circular_Queue(unsigned char _bit_number = 4) :_mask(~((~0) << _bit_number))
	{
		assert(_bit_number >= 1, "_bit_number is too few");
		assert(_bit_number <= 64, "_bit_number is too much");
		_Arr.reset(new _ElementPtrType[_mask + 1]);
	}

	explicit Circular_Queue(Circular_Queue& target) = delete;

	bool try_push(_ElementPtrType&& target) noexcept
	{
		std::unique_lock _lock(_push_mtx);
		if (full()) return false;
		_Arr[_rear++ & _mask] = std::move(target);
		return true;
	}

	template<class ...Args>
	bool try_push(Args&& ...target) noexcept
	{
		std::unique_lock _lock(_push_mtx);
		if (full()) return false;
		_Arr[_rear & _mask].reset(new _Type(std::forward<Args>(target)...));
		_rear++;
		return true;
	}

	bool try_push(_Type&& target) noexcept requires std::is_move_constructible_v<_Type>
	{
		std::unique_lock _lock(_push_mtx);
		if (full()) return false;
		_Arr[_rear & _mask].reset(new _Type(std::move(target)));
		_rear++;
		return true;
	}

	bool try_push(_Type* target) noexcept
	{
		std::unique_lock _lock(_push_mtx);
		if (full()) return false;
		_Arr[_rear & _mask].reset(target);
		_rear++;
		return true;
	}

	_ElementPtrType try_pop() noexcept
	{
		std::unique_lock _lock(_pop_mtx);
		if (empty()) return nullptr;
		return _ElementPtrType{ _Arr[_front++ & _mask].release() };
	}

	_Type* try_pop_raw() noexcept
	{
		std::unique_lock _lock(_pop_mtx);
		if (empty()) return nullptr;
		return _Arr[_front++ & _mask].release();
	}

	_Type* front() noexcept
	{
		if (empty()) return nullptr;
		return reinterpret_cast<_Type*>(_Arr[_front & _mask].get());
	}

	_Type* rear() noexcept
	{
		if (full()) return nullptr;
		return reinterpret_cast<_Type*>(_Arr[_rear & _mask].get());
	}

	bool full() const noexcept
	{
		if (((_rear + 1) & _mask) == (_front & _mask)) return true;
		return false;
	}
	bool empty() const noexcept
	{
		if ((_rear & _mask) == (_front & _mask)) return true;
		return false;
	}

	size_t size() const noexcept { return _rear - _front; }

	void clear() noexcept { _front.store(_rear); }
private:
	const unsigned long long _mask;
	std::mutex _pop_mtx, _push_mtx;
	std::unique_ptr <_ElementPtrType[]> _Arr;
	std::atomic<size_t> _front = 0, _rear = 0;
};
