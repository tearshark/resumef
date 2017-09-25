//代码来自：https://github.com/jimspr/awaituv
//https://blogs.msdn.microsoft.com/vcblog/2017/02/02/using-ibuv-with-c-resumable-functions/

#pragma once

#include <functional>
#include <memory>
#include <list>
#include <string.h>
#include <atomic>
#include <tuple>
#include <assert.h>

#ifdef __unix__
#include <coroutine.h>
#else
#include <experimental\resumable>
#endif

namespace awaituv
{
	enum struct future_error
	{
		not_ready,			// get_value called when value not available
		already_acquired	// attempt to get another future
	};

	struct future_exception : std::exception
	{
		future_error _error;
		future_exception(future_error fe) : _error(fe)
		{
		}
	};

	struct awaitable_state_base
	{
		std::function<void(void)> _coro;
		std::exception_ptr _ex;
		bool _ready = false;
		bool _future_acquired = false;

		awaitable_state_base() = default;
		awaitable_state_base(awaitable_state_base&&) = delete;
		awaitable_state_base(const awaitable_state_base&) = delete;

		void set_coro(std::function<void(void)> cb)
		{
			// Test to make sure nothing else is waiting on this future.
			assert(((cb == nullptr) || (_coro == nullptr)) && "This future is already being awaited.");
			_coro = cb;
		}

		void set_value()
		{
			// Set all members first as calling coroutine may reset stuff here.
			_ready = true;
			auto coro = _coro;
			_coro = nullptr;
			if (coro != nullptr)
				coro();
		}

		bool ready() const
		{
			return _ready;
		}

		void reset()
		{
			_coro = nullptr;
			_ex = nullptr;
			_ready = false;
			_future_acquired = false;
		}

		// functions that make this directly awaitable
		bool await_ready()
		{
			return _ready;
		}

		void await_suspend(std::experimental::coroutine_handle<> resume_cb)
		{
			set_coro(resume_cb);
		}

		void set_exception(std::exception_ptr && ex)
		{
			_ex = std::forward<std::exception_ptr>(ex);
		}
	};

	template <typename T>
	struct awaitable_state : public awaitable_state_base
	{
		T _value;

		void set_value(const T& t)
		{
			_value = t;
			awaitable_state_base::set_value();
		}

		void finalize_value()
		{
			awaitable_state_base::set_value();
		}

		auto get_value()
		{
			if (!_ready)
				throw future_exception{ future_error::not_ready };
			if (_ex)
				std::rethrow_exception(_ex);
			return _value;
		}

		void reset()
		{
			awaitable_state_base::reset();
			_value = T{};
		}

		// make this directly awaitable
		auto await_resume()
		{
			return _value;
		}
	};

	// specialization of awaitable_state<void>
	template <>
	struct awaitable_state<void> : public awaitable_state_base
	{
		void get_value()
		{
			if (!_ready)
				throw future_exception{ future_error::not_ready };
			if (_ex)
				std::rethrow_exception(_ex);
		}

		// make this directly awaitable
		void await_resume()
		{
		}
	};

	// We need to be able to manage reference count of the state object in the callback.
	template <typename awaitable_state_t>
	struct counted_awaitable_state : public awaitable_state_t
	{
		std::atomic<int> _count = 0; // tracks reference count of state object

		template <typename ...Args>
		counted_awaitable_state(Args&&... args) : _count{ 0 }, awaitable_state_t(std::forward<Args>(args)...)
		{
		}
		counted_awaitable_state(const counted_awaitable_state&) = delete;
		counted_awaitable_state(counted_awaitable_state&&) = delete;

		counted_awaitable_state* lock()
		{
			++_count;
			return this;
		}

		void unlock()
		{
			if (--_count == 0)
				delete this;
		}
	protected:
		~counted_awaitable_state() {}
	};

	// counted_ptr is similar to shared_ptr but allows explicit control
	// 
	template <typename T>
	struct counted_ptr
	{
		counted_ptr() = default;
		counted_ptr(const counted_ptr& cp) : _p(cp._p)
		{
			_lock();
		}

		counted_ptr(counted_awaitable_state<T>* p) : _p(p)
		{
			_lock();
		}

		counted_ptr(counted_ptr&& cp)
		{
			std::swap(_p, cp._p);
		}

		counted_ptr& operator=(const counted_ptr& cp)
		{
			if (&cp != this)
			{
				_unlock();
				_lock(cp._p);
			}
			return *this;
		}

		counted_ptr& operator=(counted_ptr&& cp)
		{
			if (&cp != this)
				std::swap(_p, cp._p);
			return *this;
		}

		~counted_ptr()
		{
			_unlock();
		}

		counted_awaitable_state<T>* operator->() const
		{
			return _p;
		}

		counted_awaitable_state<T>* get() const
		{
			return _p;
		}

	protected:
		void _unlock()
		{
			if (_p != nullptr)
			{
				auto t = _p;
				_p = nullptr;
				t->unlock();
			}
		}
		void _lock(counted_awaitable_state<T>* p)
		{
			if (p != nullptr)
				p->lock();
			_p = p;
		}
		void _lock()
		{
			if (_p != nullptr)
				_p->lock();
		}
		counted_awaitable_state<T>* _p = nullptr;
	};

	template <typename T, typename... Args>
	counted_ptr<T> make_counted(Args&&... args)
	{
		return new counted_awaitable_state<T>{ std::forward<Args>(args)... };
	}

	// The awaitable_state class is good enough for most cases, however there are some cases
	// where a libuv callback returns more than one "value".  In that case, the function can
	// define its own state type that holds more information.
	template <typename T, typename state_t = awaitable_state<T>>
	struct promise_t;

	template <typename T, typename state_t = awaitable_state<T>>
	struct future_t
	{
		typedef T type;
		typedef promise_t<T, state_t> promise_type;
		counted_ptr<state_t> _state;

		future_t(const counted_ptr<state_t>& state) : _state(state)
		{
			_state->_future_acquired = true;
		}

		// movable, but not copyable
		future_t(const future_t&) = delete;
		future_t(future_t&& f) = default;

		auto await_resume()
		{
			return _state->get_value();
		}

		bool await_ready()
		{
			return _state->_ready;
		}

		void await_suspend(std::experimental::coroutine_handle<> resume_cb)
		{
			_state->set_coro(resume_cb);
		}

/*
		bool ready()
		{
			return _state->_ready;
		}

		auto get_value()
		{
			return _state->get_value();
		}
*/
	};

	template <typename T, typename state_t>
	struct promise_t
	{
		typedef future_t<T, state_t> future_type;
		typedef counted_awaitable_state<state_t> state_type;
		counted_ptr<state_t> _state;

		// movable not copyable
		template <typename ...Args>
		promise_t(Args&&... args) : _state(make_counted<state_t>(std::forward<Args>(args)...))
		{
		}
		promise_t(const promise_t&) = delete;
		promise_t(promise_t&&) = default;

		future_type get_future()
		{
			if (_state->_future_acquired)
				throw future_exception{ future_error::already_acquired };
			return future_type(_state);
		}

		// Most functions don't need this but timers and reads from streams
		// cause multiple callbacks.
		future_type next_future()
		{
			// reset and return another future
			if (_state->_future_acquired)
				_state->reset();
			return future_type(_state);
		}

		future_type get_return_object()
		{
			return future_type(_state);
		}

		void set_exception(std::exception_ptr && ex)
		{
			_state->set_exception(std::forward<std::exception_ptr>(ex));
		}

		std::experimental::suspend_never initial_suspend()
		{
			return {};
		}

		std::experimental::suspend_never final_suspend()
		{
			return {};
		}

		void return_value(const T& val)
		{
			_state->set_value(val);
		}
	};

	template <typename state_t>
	struct promise_t<void, state_t>
	{
		typedef future_t<void, state_t> future_type;
		typedef counted_awaitable_state<state_t> state_type;
		counted_ptr<state_t> _state;

		// movable not copyable
		template <typename ...Args>
		promise_t(Args&&... args) : _state(make_counted<state_t>(std::forward<Args>(args)...))
		{
		}
		promise_t(const promise_t&) = delete;
		promise_t(promise_t&&) = default;

		future_type get_future()
		{
			return future_type(_state);
		}

		future_type get_return_object()
		{
			return future_type(_state);
		}

		void set_exception(std::exception_ptr && ex)
		{
			_state->set_exception(std::forward<std::exception_ptr>(ex));
		}

		std::experimental::suspend_never initial_suspend()
		{
			return {};
		}

		std::experimental::suspend_never final_suspend()
		{
			return {};
		}

		void return_void()
		{
			_state->set_value();
		}
	};

	// future_of_all is pretty trivial as we can just await on each argument

	template <typename T>
	future_t<void> future_of_all(T& f)
	{
		co_await f;
	}

	template <typename T, typename... Rest>
	future_t<void> future_of_all(T& f, Rest&... args)
	{
		co_await f;
		co_await future_of_all(args...);
	}

	// future_of_all_range can take a vector/array of futures, although
	// they must be of the same time. It returns a vector of all the results.
	template <typename Iterator>
	auto future_of_all_range(Iterator begin, Iterator end) -> future_t<std::vector<decltype(begin->await_resume())>>
	{
		std::vector<decltype(co_await *begin)> vec;
		while (begin != end)
		{
			vec.push_back(co_await *begin);
			++begin;
		}
		return vec;
	}

	template <typename tuple_t>
	void set_coro_helper(tuple_t& tuple, std::function<void(void)> cb)
	{
		// Define some helper templates to iterate through each element
		// of the tuple
		template <size_t N>
		struct coro_helper
		{
			static void set(tuple_t& tuple, std::function<void(void)> cb)
			{
				std::get<N>(tuple)._state->set_coro(cb);
				coro_helper<tuple_t, N - 1>::set(tuple, cb);
			}
		};
		// Specialization for last item
		template <>
		struct coro_helper<0>
		{
			static void set(tuple_t& tuple, std::function<void(void)> cb)
			{
				std::get<0>(tuple)._state->set_coro(cb);
			}
		};

		coro_helper<tuple_t, std::tuple_size<tuple_t>::value - 1>::set(tuple, cb);
	}

	// allows waiting for just one future to complete
	template <typename... Rest>
	struct multi_awaitable_state : public awaitable_state<void>
	{
		// Store references to all the futures passed in.
		std::tuple<Rest&...> _futures;
		multi_awaitable_state(Rest&... args) : _futures(args...)
		{
		}

		void set_coro(std::function<void(void)> cb)
		{
			set_coro_helper(_futures,
				[this]()
			{
				// reset callbacks on all futures to stop them
				set_coro_helper(_futures, nullptr);
				set_value();
			});
			awaitable_state<void>::set_coro(cb);
		}
	};

	// future_of_any is pretty complicated
	// We have to create a new promise with a custom awaitable state object
	template <typename T, typename... Rest>
	future_t<void, multi_awaitable_state<T, Rest...>> future_of_any(T& f, Rest&... args)
	{
		promise_t<void, multi_awaitable_state<T, Rest...>> promise(f, args...);
		return promise.get_future();
	}

	// iterator_awaitable_state will track the index of which future completed
	template <typename Iterator>
	struct iterator_awaitable_state : public awaitable_state<Iterator>
	{
		Iterator _begin;
		Iterator _end;
		iterator_awaitable_state(Iterator begin, Iterator end) : _begin(begin), _end(end)
		{
		}

		// any_completed will be called by any future completing
		void any_completed(Iterator completed)
		{
			// stop any other callbacks from coming in
			for (Iterator c = _begin; c != _end; ++c)
				c->_state->set_coro(nullptr);
			set_value(completed);
		}

		void set_coro(std::function<void(void)> cb)
		{
			for (Iterator c = _begin; c != _end; ++c)
			{
				std::function<void(void)> func = std::bind(&iterator_awaitable_state::any_completed, this, c);
				c->_state->set_coro(func);
			}
			awaitable_state<Iterator>::set_coro(cb);
		}
	};

	// returns the index of the iterator that succeeded
	template <typename Iterator>
	future_t<Iterator, iterator_awaitable_state<Iterator>> future_of_any_range(Iterator begin, Iterator end)
	{
		promise_t<Iterator, iterator_awaitable_state<Iterator>> promise(begin, end);
		return promise.get_future();
	}


	template<typename T1, typename S1, typename T2, typename S2>
	auto operator||(future_t<T1, S1>& t1, future_t<T2, S2>& t2)
	{
		return future_of_any(t1, t2);
	}

	template<typename T1, typename S1, typename T2, typename S2>
	auto operator&&(future_t<T1, S1>& t1, future_t<T2, S2>& t2)
	{
		return future_of_all(t1, t2);
	}


	template <typename T, typename state_t = awaitable_state<T>>
	using shared_promise_ptr = std::shared_ptr<promise_t<T, state_t>>;

	template <typename T, typename state_t = awaitable_state<T>>
	using weak_promise_ptr = std::weak_ptr<promise_t<T, state_t>>;

	template <typename T, typename state_t, typename ...Args>
	shared_promise_ptr<T, state_t> make_promise(Args&&... args)
	{
		return std::make_shared<promise_t<T, state_t>>(std::forward<Args>(args)...);
	}

} // namespace awaituv
