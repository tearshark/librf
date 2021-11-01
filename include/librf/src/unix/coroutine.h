//===----------------------------- coroutine -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_EXPERIMENTAL_COROUTINE
#define _LIBCPP_EXPERIMENTAL_COROUTINE

#define _EXPERIMENTAL_COROUTINE_

#include <new>
#include <type_traits>
#include <functional>
#include <memory> // for hash<T*>
#include <cstddef>
#include <cassert>
#include <compare>

#if defined(__clang__)
#include "clang_builtin.h"
#elif defined(__GNUC__)
#include "gcc_builtin.h"
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#pragma GCC system_header
#endif

#ifndef _LIBCPP_HAS_NO_COROUTINES

namespace std {
#if defined(__GNUC__)
	inline
#endif
	namespace experimental {

		template <class _Tp, class = void>
		struct __coroutine_traits_sfinae {};

		template <class _Tp>
		struct __coroutine_traits_sfinae<_Tp, void_t<typename _Tp::promise_type>>
		{
			using promise_type = typename _Tp::promise_type;
		};

		// 17.12.2, coroutine traits
		template <typename _Ret, typename... _Args>
		struct coroutine_traits : public __coroutine_traits_sfinae<_Ret>
		{
		};

		// 17.12.3, coroutine handle
		template <typename _Promise = void>
		class coroutine_handle;

		template <>
		class coroutine_handle<void>
		{
		public:
			// 17.12.3.1, construct/reset
			constexpr coroutine_handle() noexcept : __handle_(nullptr) {}
			constexpr coroutine_handle(nullptr_t) noexcept : __handle_(nullptr) {}
			coroutine_handle& operator=(nullptr_t) noexcept {
				__handle_ = nullptr;
				return *this;
			}

			// 17.12.3.2, export/import
			constexpr void* address() const noexcept { return __handle_; }
			static constexpr coroutine_handle from_address(void* __addr) noexcept {
				coroutine_handle __tmp;
				__tmp.__handle_ = __addr;
				return __tmp;
			}
			// FIXME: Should from_address(nullptr) be allowed?
			static constexpr coroutine_handle from_address(nullptr_t) noexcept {
				return coroutine_handle(nullptr);
			}

			template <class _Tp, bool _CallIsValid = false>
			static coroutine_handle from_address(_Tp*) {
				static_assert(_CallIsValid,
					"coroutine_handle<void>::from_address cannot be called with "
					"non-void pointers");
			}

			// 17.12.3.3, observers
			constexpr explicit operator bool() const noexcept {
				return __handle_ != nullptr; 
			}
			bool done() const {
				return __builtin_coro_done(__handle_);
			}

			// 17.12.3.4, resumption 
			void operator()() const { resume(); }
			void resume() const {
				__builtin_coro_resume(__handle_);
			}
			void destroy() const{
				__builtin_coro_destroy(__handle_);
			}
		private:
			bool __is_suspended() const noexcept {
				// FIXME actually implement a check for if the coro is suspended.
				return __handle_;
			}

			template <class _PromiseT>
			friend class coroutine_handle;

			void* __handle_;
		};

		// 17.12.3.6, comparison operators
		inline bool operator==(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return __x.address() == __y.address();
		}
		inline bool operator!=(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return __x.address() != __y.address();
		}
		inline bool operator<(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return __x.address() < __y.address();
		}
		inline bool operator>(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return __x.address() > __y.address();
		}
		inline bool operator<=(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return __x.address() <= __y.address();
		}
		inline bool operator>=(coroutine_handle<> __x, coroutine_handle<> __y) noexcept {
			return __x.address() >= __y.address();
		}

		template <typename _Promise>
		class coroutine_handle : public coroutine_handle<> 
		{
			using _Base = coroutine_handle<>;
		public:
			// 17.12.3.1, construct/reset 
			using coroutine_handle<>::coroutine_handle;
			coroutine_handle& operator=(nullptr_t) noexcept {
				_Base::operator=(nullptr);
				return *this;
			}

			// 17.12.3.5, promise access
			_Promise& promise() const {
				return *static_cast<_Promise*>(
					__builtin_coro_promise(this->__handle_, alignof(_Promise), false));
			}

		public:
			// 17.12.3.2, export/import
			static constexpr coroutine_handle from_address(void* __addr) noexcept {
				coroutine_handle __tmp;
				__tmp.__handle_ = __addr;
				return __tmp;
			}

			// NOTE: this overload isn't required by the standard but is needed so
			// the deleted _Promise* overload doesn't make from_address(nullptr)
			// ambiguous.
			// FIXME: should from_address work with nullptr?
			static constexpr coroutine_handle from_address(nullptr_t) noexcept {
				return coroutine_handle(nullptr);
			}

			template <class _Tp, bool _CallIsValid = false>
			static coroutine_handle from_address(_Tp*) {
				static_assert(_CallIsValid,
					"coroutine_handle<promise_type>::from_address cannot be called with "
					"non-void pointers");
			}

			template <bool _CallIsValid = false>
			static coroutine_handle from_address(_Promise*) {
				static_assert(_CallIsValid,
					"coroutine_handle<promise_type>::from_address cannot be used with "
					"pointers to the coroutine's promise type; use 'from_promise' instead");
			}

			static coroutine_handle from_promise(_Promise& __promise) noexcept {
				typedef typename remove_cv<_Promise>::type _RawPromise;
				coroutine_handle __tmp;
				__tmp.__handle_ = __builtin_coro_promise(
					std::addressof(const_cast<_RawPromise&>(__promise)),
					alignof(_Promise), true);
				return __tmp;
			}
		};

		// 17.12.4, no-op coroutines
#if __has_builtin(__builtin_coro_noop)
		struct noop_coroutine_promise {};

		template <>
		class coroutine_handle<noop_coroutine_promise> : public coroutine_handle<>
		{
			using _Base = coroutine_handle<>;
			using _Promise = noop_coroutine_promise;
		public:
			// 17.12.4.2.3, promise access 
			_Promise& promise() const noexcept {
				return *static_cast<_Promise*>(
					__builtin_coro_promise(this->__handle_, alignof(_Promise), false));
			}
			
			// 17.12.4.2.4, address 
			constexpr void* address() const noexcept {
				return this->__handle_;
			}

			// 17.12.4.2.1, observers 
			constexpr explicit operator bool() const noexcept { return true; }
			constexpr bool done() const noexcept { return false; }

			// 17.12.4.2.2, resumption
			constexpr void operator()() const noexcept {}
			constexpr void resume() const noexcept {}
			constexpr void destroy() const noexcept {}
		private:
			friend coroutine_handle<noop_coroutine_promise> noop_coroutine() noexcept;
			coroutine_handle() noexcept {
				this->__handle_ = __builtin_coro_noop();
			}
		};

		using noop_coroutine_handle = coroutine_handle<noop_coroutine_promise>;
		
		inline noop_coroutine_handle noop_coroutine() noexcept {
			return noop_coroutine_handle();
		}
#endif // __has_builtin(__builtin_coro_noop)

		// 17.12.5, trivial awaitables
		struct suspend_never {
			constexpr bool await_ready() const noexcept { return true; }
			constexpr void await_suspend(coroutine_handle<>) const noexcept {}
			constexpr void await_resume() const noexcept {}
		};
		struct suspend_always {
			constexpr bool await_ready() const noexcept { return false; }
			constexpr void await_suspend(coroutine_handle<>) const noexcept {}
			constexpr void await_resume() const noexcept {}
		};
	}

	// 17.12.3.7, hash support
	template <class _Tp>
	struct hash<experimental::coroutine_handle<_Tp>> {
		using argument_type = experimental::coroutine_handle<_Tp>;
		using result_type = size_t;
		using __arg_type = experimental::coroutine_handle<_Tp>;
		size_t operator()(__arg_type const& __v) const noexcept
		{
			return hash<void*>()(__v.address());
		}
	};
}

#endif // !defined(_LIBCPP_HAS_NO_COROUTINES)

#endif /* _LIBCPP_EXPERIMENTAL_COROUTINE */