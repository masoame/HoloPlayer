#pragma once

#ifdef _WIN32
    #include<windows.h>
#elif __linux__

#else
    #error "Unknown"
#endif 

#include<type_traits>
#include<memory>
#include<functional>
#include<queue>
#include<mutex>
#include<condition_variable>
#include<future>
#include<assert.h>
#include<optional>

extern void close(int fd);

namespace common
{
	/*
	* Functor用于静态封装一个函数，使其可以作为模板参数。
	* _F: 被封装的函数
	*/
	template <auto _F>
	using Functor = std::integral_constant<std::remove_reference_t<decltype(_F)>, _F>;

	/*
	* AutoPtr用于封装智能指针，使其可以自动管理内存，
	* 方便对于一些c库自动析构进行适配
	* 支持两级指针因为有些库封装析构的指针是二级指针。（即释放对二级指向对象的指针对对应的一级指针进行置空）
	* 重载 operator& 返回一个指针_Type**，用于获取原始指针的地址。
	* _T: 被封装的智能指针类型
	*/
	template<class _T, class _FreeFunc>
	struct AutoHandle
	{
		using _Type = std::remove_reference_t<_T>;
		static_assert(std::is_invocable_v<typename _FreeFunc::value_type, _Type**> || std::is_invocable_v<typename _FreeFunc::value_type, _Type*>);
		constexpr static bool isSecPtr = std::is_invocable_v<typename _FreeFunc::value_type, _Type**>;

		AutoHandle() noexcept {}
		AutoHandle(AutoHandle& _handle) = delete;
		AutoHandle(AutoHandle&& _handle) noexcept : _ptr(_handle.release()) {}
		AutoHandle(_Type* ptr) noexcept : _ptr(ptr) {}

		AutoHandle& operator=(_Type* ptr) noexcept { _ptr.reset(ptr); return *this; }
		AutoHandle& operator=(AutoHandle&& _handle) noexcept { _ptr.reset(_handle.release()); return *this; }

		operator const _Type* () const noexcept { return _ptr.get(); }
		operator _Type*& () noexcept { return *reinterpret_cast<_Type**>(this); }
		operator bool() const noexcept { return _ptr.get() != nullptr; }

		_Type** operator&() { static_assert(sizeof(*this) == sizeof(void*)); return reinterpret_cast<_Type**>(this); }

		_Type* operator->() const noexcept { return _ptr.get(); }

		void reset(_Type* ptr = nullptr) noexcept { _ptr.reset(ptr); }

		auto release() noexcept { return _ptr.release(); }
		auto get() const noexcept { return _ptr.get(); }
	private:
		struct DeletePrimaryPtr { void operator()(void* ptr) { _FreeFunc()(static_cast<_Type*>(ptr)); } };
		struct DeleteSecPtr { void operator()(void* ptr) { _FreeFunc()(reinterpret_cast<_Type**>(&ptr)); } };
		using DeletePtr = std::conditional<isSecPtr, DeleteSecPtr, DeletePrimaryPtr>::type;
		std::unique_ptr<_Type, DeletePtr> _ptr;
	};

	/*
	* unique_fd用于封装一个文件描述符，使其可以自动管理内存，
	* 方便对于一些c库自动关闭文件描述符进行适配。
	* 重载 operator bool 返回一个布尔值，用于判断是否有效。
	* 重载 operator int 返回一个文件描述符。
	* 重载 operator= 用于赋值。
	* 重载 reset 用于重置文件描述符。
	* 重载 release 用于获取文件描述符。
	* 重载 swap 用于交换两个unique_fd。
	*/

	template<class T = int, class FreeFunc = Functor<::close>>
	struct unique_fd
	{
		T _fd;
		unique_fd() : _fd(-1) {}
		explicit unique_fd(int fd) : _fd(fd) {}
		explicit unique_fd(const unique_fd&) = delete;
		explicit unique_fd(unique_fd&& other) noexcept : _fd(other._fd) { other._fd = -1; }

		unique_fd& operator=(const unique_fd&) = delete;
		unique_fd& operator=(unique_fd&& other) noexcept { _fd = other._fd; other._fd = -1; return *this; }

		~unique_fd() { if (_fd >= 0) FreeFunc{}(_fd); }

		operator int() const noexcept { return _fd; }

		int release() noexcept { int ret = _fd; _fd = -1; return ret; }
		unique_fd& reset(int fd = -1) noexcept { if (_fd >= 0) FreeFunc{}(_fd); _fd = fd; return *this; }
		void swap(unique_fd& other) noexcept { std::swap(_fd, other._fd); }
		T get() const noexcept { return _fd; }

		explicit operator bool() const noexcept { return _fd >= 0; }
	};


	/*
	* reverse_bit用于反转位域reverse_bit
	*/
	template<class _T1>
	inline void reverse_bit(_T1& val, _T1 local) noexcept requires std::is_pod_v<_T1>
	{
		val = (val & local) ? (val & (~local)) : (val | local);
	}

	/*
	* circular_queue用于实现一个循环队列，支持线程安全的入队和出队操作。内部资源都是通过智能指针管理，支持自定义的删除函数。
	* _IsThreadSafe == true 时，队列是线程安全性最高的。
	* _IsThreadSafe == false 时，队列在一读一写的情况下是线程安全的，但是在多读多写的情况下，需要用户自行保证线程安全。
	* _buf_level 用于设置队列的大小，默认为4，最大为64。 实际大小为2的_buf_level次方。
	*/
	template<class _T, class _DeleteFunctionType = std::nullptr_t, bool _IsThreadSafe = false>
	class circular_queue
	{
		using _Type = std::remove_reference<_T>::type;
		using _ElementPtrType = std::conditional_t<std::is_null_pointer_v<_DeleteFunctionType>, std::unique_ptr<_Type>, AutoHandle<_Type, _DeleteFunctionType>>;
		using _HasMutex = std::conditional_t<_IsThreadSafe, std::mutex, std::nullptr_t>;
		using _IsLock = std::conditional_t<_IsThreadSafe, std::unique_lock<std::mutex>, nullptr_t>;

	public:
		circular_queue(circular_queue& target) = delete;

		circular_queue(unsigned char _buf_level = 4) :_mask(~((~0) << _buf_level))
		{
			//assert(_buf_level >= 1);
			//assert(_buf_level <= 64);
			_Arr.reset(new _ElementPtrType[_mask + 1]);
		}

		bool try_push(_ElementPtrType&& target) noexcept
		{
			_IsLock _lock(_push_mtx);
			if (full()) return false;
			_Arr[_rear++ & _mask] = std::move(target);
			return true;
		}

		template<class ...Args>
		bool try_emplace(Args&& ...target) noexcept
		{
			_IsLock _lock(_push_mtx);
			if (full()) return false;
			_Arr[_rear & _mask].reset(new _Type(std::forward<Args>(target)...));
			_rear++;
			return true;
		}

		bool try_push(_Type&& target) noexcept requires std::is_move_constructible_v<_Type>
		{
			_IsLock _lock(_push_mtx);
			if (full()) return false;
			_Arr[_rear & _mask].reset(new _Type(std::move(target)));
			_rear++;
			return true;
		}

		bool try_push(_Type* target) noexcept
		{
			_IsLock _lock(_push_mtx);
			if (full()) return false;
			_Arr[_rear & _mask].reset(target);
			_rear++;
			return true;
		}

		_ElementPtrType try_pop() noexcept
		{
			_IsLock _lock(_pop_mtx);
			if (empty()) return nullptr;
			return _ElementPtrType{ _Arr[_front++ & _mask].release() };
		}

		_Type* try_pop_raw() noexcept
		{
			_IsLock _lock(_pop_mtx);
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

		_HasMutex _pop_mtx{}, _push_mtx{};

		std::unique_ptr <_ElementPtrType[]> _Arr;
		std::atomic<size_t> _front = 0, _rear = 0;
		std::condition_variable _cv_could_push, _cv_could_pop;
	};
	/*
	* BoundedQueue用于实现一个有界队列，支持线程安全的入队和出队操作。内部资源都是通过智能指针管理，支持自定义的删除函数。
	*/
	template<class _T>
	class bounded_queue
	{
		using _Type = std::remove_reference_t<_T>;
	public:
		bounded_queue(size_t max_size = ULLONG_MAX) : _max_size(max_size), _is_closed(false) {}

		~bounded_queue() {
			_is_closed = true;
			_cv_could_push.notify_all();
			_cv_could_pop.notify_all();
		}

		template<typename... Args>
		void emplace(Args&&... args) noexcept
		{
			std::unique_lock lock(_mtx);
			_cv_could_push.wait(lock, [this] { return (_queue.size() < this->_max_size) || _is_closed; });
			if (_is_closed) return;
			_queue.emplace_back(std::forward<Args>(args)...);
			_cv_could_pop.notify_one();
			return;
		}

		void push(_Type&& value) noexcept
		{
			std::unique_lock lock(_mtx);
			_cv_could_push.wait(lock, [this] { return (_queue.size() < this->_max_size) || _is_closed; });
			if (_is_closed) return;
			_queue.push_back(std::move(value));
			_cv_could_pop.notify_one();
		}

		void push(const _Type& value) noexcept
		{
			std::unique_lock lock(_mtx);
			_cv_could_push.wait(lock, [this] { return (_queue.size() < this->_max_size) || _is_closed; });
			if (_is_closed) return;
			_queue.push_back(value);
			_cv_could_pop.notify_one();
		}

		std::optional<_Type> pop() noexcept
		{
			std::unique_lock lock(_mtx);
			_cv_could_pop.wait(lock, [this] { return (this->_queue.empty() == false) || _is_closed; });
			if (_is_closed) return std::nullopt;
			_Type _ret{ std::move(_queue.front()) };
			_queue.pop_front();
			_cv_could_push.notify_one();
			return _ret;
		}

		template <class _Rep, class _Period>
		std::optional<_Type> pop_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) noexcept
		{
			std::unique_lock lock(_mtx);
			bool cv_status = _cv_could_pop.wait_for(lock, _Rel_time, [this] { return (this->_queue.empty() == false) || _is_closed; });
			if (_is_closed || !cv_status) return std::nullopt;
			_Type _ret{ std::move(_queue.front()) };
			_queue.pop_front();
			_cv_could_push.notify_one();
			return _ret;
		}

		inline size_t size() const noexcept
		{
			return _queue.size();
		}

		inline bool empty() const noexcept
		{
			return _queue.empty();
		}

		inline bool full() const noexcept
		{
			return _queue.size() >= _max_size;
		}

		inline void lock() noexcept
		{
			_mtx.lock();
		}

		inline void unlock() noexcept
		{
			_mtx.unlock();
		}

		void clear() noexcept
		{
			_queue.clear();
		}
		bool _is_closed = true;
		std::condition_variable _cv_could_push, _cv_could_pop;
	private:
		std::deque<_Type> _queue;
		const size_t _max_size;
		std::mutex _mtx;
	};

	/*
	* singletont_thread_pool用于实现一个单例线程池，支持线程安全的任务队列和线程管理。
	* 项目来源: https://github.com/progschj/ThreadPool
	* 适配C++20，并增加stop_source参数，用于控制线程池的关闭。
	* 使用std::jthread替代原来的std::thread，支持stop_token参数，用于控制线程的关闭。
	*/
	class ThreadPool {

	public:
		ThreadPool(size_t threads);
		~ThreadPool();

		template<class F, class... Args>
		auto enqueue(F&& f, Args&&... args)
			-> std::future<typename std::invoke_result<F, Args...>::type>;
	private:

		// need to keep track of threads so we can join them
		std::vector< std::jthread > workers;
		// the task queue
		std::queue< std::function<void()> > tasks;

		// synchronization
		std::mutex queue_mutex;
		std::condition_variable condition;
		//bool stop;
		std::stop_source stop;
	};

	// the constructor just launches some amount of workers
	inline ThreadPool::ThreadPool(size_t threads)
	{
		for (size_t i = 0; i < threads; ++i)
			workers.emplace_back(
				[this]()
				{
					for (;;)
					{
						std::function<void()> task;

						{
							std::unique_lock<std::mutex> lock(this->queue_mutex);
							this->condition.wait(lock,
								[this] { return (this->stop.stop_requested() == true) || this->tasks.empty() == false; });
							if (this->stop.stop_requested() && this->tasks.empty())
								return;
							task = std::move(this->tasks.front());
							this->tasks.pop();
						}
						task();
					}
				}
			);
	}

	// add new work item to the pool
	template<class F, class... Args>
	auto ThreadPool::enqueue(F&& f, Args&&... args)
		-> std::future<typename std::invoke_result<F, Args...>::type>
	{
		using return_type = typename std::invoke_result<F, Args...>::type;

		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

		std::future<return_type> res = task->get_future();
		{
			std::unique_lock<std::mutex> lock(queue_mutex);

			if (stop.stop_requested())
				throw std::runtime_error("enqueue on stopped ThreadPool");

			tasks.emplace([task]() { (*task)(); });
		}
		condition.notify_one();
		return res;
	}

	// the destructor joins all threads
	inline ThreadPool::~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			stop.request_stop();
		}
		condition.notify_all();
	}
}