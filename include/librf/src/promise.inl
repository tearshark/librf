
namespace librf
{

/*
	template<typename _Ty>
	template<typename _Uty>
	_Uty&& promise_impl_t<_Ty>::await_transform(_Uty&& _Whatever) noexcept
	{
		if constexpr (traits::has_state_v<_Uty>)
		{
			_Whatever._state->set_scheduler(get_state()->get_scheduler());
		}

		return std::forward<_Uty>(_Whatever);
	}
*/

	template <typename _Ty>
	auto promise_impl_t<_Ty>::get_state() noexcept -> state_type*
	{
		return _state.get();
	}

	// 如果去掉了调度器，则ref_state()实现为返回counted_ptr<>，以便于处理一些意外情况
	//template <typename _Ty>
	//auto promise_impl_t<_Ty>::ref_state() noexcept -> counted_ptr<state_type>
	//{
	//	return { get_state() };
	//}
	template <typename _Ty>
	auto promise_impl_t<_Ty>::ref_state() noexcept -> state_type*
	{
		return get_state();
	}

	template <typename _Ty>
	void* promise_impl_t<_Ty>::operator new(size_t _Size)
	{
		_Alloc_char _Al;
		char* ptr = _Al.allocate(_Size);
#if RESUMEF_DEBUG_COUNTER
		std::cout << "  future_promise::new, alloc size=" << (_Size) << std::endl;
		std::cout << "  future_promise::new, alloc ptr=" << (void*)ptr << std::endl;
		std::cout << "  future_promise::new, return ptr=" << (void*)ptr << std::endl;
#endif
		return ptr;
	}

	template <typename _Ty>
	void promise_impl_t<_Ty>::operator delete(void* _Ptr, size_t _Size)
	{
		_Alloc_char _Al;
		return _Al.deallocate(reinterpret_cast<char*>(_Ptr), _Size);
	}
}

