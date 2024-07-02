#pragma once
#include<memory>
#include<atomic>

//循环队列抽象接口
template<class _T>
struct Circular_Queue_API
{
	using _Type = std::remove_reference<_T>::type;

	virtual inline size_t size() noexcept = 0;

	virtual inline bool push(_Type&& target) noexcept = 0;
	virtual inline _Type* pop() noexcept = 0;


	virtual inline _Type* front() noexcept = 0;
	virtual inline _Type* rear() noexcept = 0;


	virtual inline bool full() noexcept = 0;
	virtual inline bool empty() noexcept = 0;

	virtual inline void clear() noexcept = 0;
};

//循环队列
template<class _T, unsigned char _bit_number = 4>
class Circular_Queue :public Circular_Queue_API<_T>
{
	static_assert(_bit_number > 0 && _bit_number <= 64);
	using _Type = std::remove_reference<_T>::type;
	constexpr static unsigned long long _mask = ~((~0) << _bit_number);

public:
	explicit Circular_Queue() :_front(0), _rear(0) { _Arr.reset(new _Type[_mask + 1]); }
	explicit Circular_Queue(Circular_Queue&& target) :_front(target._front), _rear(target._rear) { _Arr.reset(target._Arr.release()); }
	
    size_t size() noexcept override { return _rear - _front; }

	bool push(_Type&& target) noexcept 	override
	{
		if (full()) return false;
        _Arr[_rear++ & _mask] = std::forward<_Type>(target);
		return true;
	}

	_Type* pop() noexcept override
	{
		if (empty()) return nullptr;
		return reinterpret_cast<_Type*>(&_Arr[_front++ & _mask]);
	}

	_Type* front() noexcept override
	{
		if (empty()) return nullptr; 
		return reinterpret_cast<_Type*>(&_Arr[_front & _mask]);
	}

	_Type* rear() noexcept override
	{
		if (full()) return nullptr;
		return reinterpret_cast<_Type*>(&_Arr[_rear & _mask]) ;
	}

	bool full() noexcept override
	{
		if (((_rear + 1) & _mask) == (_front & _mask)) return true;
		return false;
	}
	bool empty() noexcept override
	{
		if ((_rear & _mask) == (_front & _mask)) return true;
		return false;
	}
	void clear() noexcept override
	{
		_front.store(_rear);
	}
private:
	std::unique_ptr<_Type[]> _Arr;
	std::atomic<size_t> _front, _rear;
};
