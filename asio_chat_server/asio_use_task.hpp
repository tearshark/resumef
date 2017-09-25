//代码来自：https://github.com/wangjieest/awaitable_tasks
//感谢来自QQ群：296561497（c++1z boost 交流）的网友“从未来过”!

#pragma once

#include "awaituv.h"
#include "asio/detail/config.hpp"
#include <memory>

#include "asio/detail/push_options.hpp"
#include "asio/async_result.hpp"
#include "asio/error_code.hpp"
#include "asio/handler_type.hpp"
#include "asio/system_error.hpp"

namespace asio {

	/// Class used to specify that an asynchronous operation should return a task.
	/**
	* The use_task_t class is used to indicate that an asynchronous operation
	* should return a task object. A use_task_t object may be passed as a
	* handler to an asynchronous operation, typically using the special value @c
	* asio::use_task. For example:
	*
	* @code std::task<std::size_t> my_task
	*   = my_socket.async_read_some(my_buffer, asio::use_task); @endcode
	*
	* The initiating function (async_read_some in the above example) returns a
	* task that will receive the result of the operation. If the operation
	* completes with an error_code indicating failure, it is converted into a
	* system_error and passed back to the caller via the task.
	*/
	template<typename Allocator = std::allocator<void> >
	class use_task_t
	{
	public:
		/// The allocator type. The allocator is used when constructing the
		/// @c std::promise object for a given asynchronous operation.
		typedef Allocator allocator_type;

		/// Construct using default-constructed allocator.
		ASIO_CONSTEXPR use_task_t() {}

		/// Construct using specified allocator.
		explicit use_task_t(const Allocator& allocator) : allocator_(allocator) {}

		/// Specify an alternate allocator.
		template<typename OtherAllocator>
		use_task_t<OtherAllocator> operator[](const OtherAllocator& allocator) const {
			return use_task_t<OtherAllocator>(allocator);
		}

		/// Obtain allocator.
		allocator_type get_allocator() const { return allocator_; }

	private:
		Allocator allocator_;
	};

	//constexpr use_task_t<> use_task;
#pragma warning(push)
#pragma warning(disable : 4592)
	__declspec(selectany) use_task_t<> use_task;
#pragma warning(pop)

	namespace detail {

		// Completion handler to adapt a promise as a completion handler.
		template<typename T>
		class promise_handler {
		public:
			using result_type_t = T;
			using state_type = awaituv::awaitable_state<result_type_t>;

			// Construct from use_task special value.
			template<typename Allocator>
			promise_handler(use_task_t<Allocator> uf) {
				state_ = awaituv::make_counted<state_type>();
			}

			void operator()(T t) {
				state_->set_value(std::move(t));
			}

			void operator()(const asio::error_code& ec, T t) {
				if (ec) {
					state_->set_exception(std::make_exception_ptr(asio::system_error(ec)));
				}
				else {
					state_->set_value(std::move(t));
				}
			}

			// private:
			awaituv::counted_ptr<state_type> state_;
		};

		// Completion handler to adapt a void promise as a completion handler.
		template<>
		class promise_handler<void> {
		public:
			using result_type_t = void;
			using state_type = awaituv::awaitable_state<result_type_t>;

			// Construct from use_task special value. Used during rebinding.
			template<typename Allocator>
			promise_handler(use_task_t<Allocator> uf) {
				state_ = awaituv::make_counted<state_type>();
			}

			void operator()() {
				state_->set_value();
			}

			void operator()(const asio::error_code& ec) {
				if (ec) {
					state_->set_exception(std::make_exception_ptr(asio::system_error(ec)));
				}
				else {
					state_->set_value();
				}
			}

			// private:
			awaituv::counted_ptr<state_type> state_;
		};

		// Handler traits specialisation for promise_handler.
		template<typename T>
		class async_result<detail::promise_handler<T> > {
		public:
			// The initiating function will return a task.
			typedef awaituv::future_t<T> type;

			// Constructor creates a new promise for the async operation, and obtains the
			// corresponding task.
			explicit async_result(detail::promise_handler<T> & h)
				: task_(std::move(h.state_))
			{ }

			// Obtain the task to be returned from the initiating function.
			awaituv::future_t<T> get() { return std::move(task_); }

		private:
			awaituv::future_t<T> task_;
		};

		// Handler type specialisation for zero arg.
		template<typename Allocator, typename ReturnType>
		struct handler_type<use_task_t<Allocator>, ReturnType()> {
			typedef detail::promise_handler<void> type;
		};

		// Handler type specialisation for one arg.
		template<typename Allocator, typename ReturnType, typename Arg1>
		struct handler_type<use_task_t<Allocator>, ReturnType(Arg1)> {
			typedef detail::promise_handler<Arg1> type;
		};

		// Handler type specialisation for two arg.
		template<typename Allocator, typename ReturnType, typename Arg2>
		struct handler_type<use_task_t<Allocator>, ReturnType(asio::error_code, Arg2)> {
			typedef detail::promise_handler<Arg2> type;
		};

		template<typename Allocator, typename ReturnType>
		struct handler_type<use_task_t<Allocator>, ReturnType(asio::error_code)> {
			typedef detail::promise_handler<void> type;
		};

	}  // namespace asio
}

#include "asio/detail/pop_options.hpp"
