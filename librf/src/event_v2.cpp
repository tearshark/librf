#include "../librf.h"

RESUMEF_NS
{
	namespace detail
	{
		void event_v2_impl::notify_all() noexcept
		{
			// Needs to be 'release' so that subsequent 'co_await' has
			// visibility of our prior writes.
			// Needs to be 'acquire' so that we have visibility of prior
			// writes by awaiting coroutines.
			void* oldValue = m_state.exchange(this, std::memory_order_acq_rel);
			if (oldValue != this)
			{
				// Wasn't already in 'set' state.
				// Treat old value as head of a linked-list of waiters
				// which we have now acquired and need to resume.
				state_event_t* state = static_cast<state_event_t*>(oldValue);
				while (state != nullptr)
				{
					// Read m_next before resuming the coroutine as resuming
					// the coroutine will likely destroy the awaiter object.
					auto* next = state->m_next;
					state->on_notify();
					state = next;
				}
			}
		}

		void event_v2_impl::notify_one() noexcept
		{
			// Needs to be 'release' so that subsequent 'co_await' has
			// visibility of our prior writes.
			// Needs to be 'acquire' so that we have visibility of prior
			// writes by awaiting coroutines.
			void* oldValue = m_state.exchange(nullptr, std::memory_order_acq_rel);
			if (oldValue != this)
			{
				// Wasn't already in 'set' state.
				// Treat old value as head of a linked-list of waiters
				// which we have now acquired and need to resume.
				state_event_t* state = static_cast<state_event_t*>(oldValue);
				if (state != nullptr)
				{
					// Read m_next before resuming the coroutine as resuming
					// the coroutine will likely destroy the awaiter object.
					auto* next = state->m_next;
					state->on_notify();

					if (next != nullptr)
						add_notify_list(next);
				}
			}
		}

		bool event_v2_impl::add_notify_list(state_event_t* state) noexcept
		{
			// Try to atomically push this awaiter onto the front of the list.
			void* oldValue = m_state.load(std::memory_order_acquire);
			do
			{
				// Resume immediately if already in 'set' state.
				if (oldValue == this) return false;

				// Update linked list to point at current head.
				state->m_next = static_cast<state_event_t*>(oldValue);

				// Finally, try to swap the old list head, inserting this awaiter
				// as the new list head.
			} while (!m_state.compare_exchange_weak(
				oldValue,
				state,
				std::memory_order_release,
				std::memory_order_acquire));

			return true;
		}

		void state_event_t::resume()
		{
			coroutine_handle<> handler = _coro;
			if (handler)
			{
				_coro = nullptr;
				_scheduler->del_final(this);
				handler.resume();
			}
		}

		bool state_event_t::has_handler() const  noexcept
		{
			return (bool)_coro;
		}
	}

	namespace event_v2
	{
		event_t::event_t(bool initially)
			:_event(std::make_shared<detail::event_v2_impl>(initially))
		{

		}
	}

	void async_manual_reset_event::set() noexcept
	{
		// Needs to be 'release' so that subsequent 'co_await' has
		// visibility of our prior writes.
		// Needs to be 'acquire' so that we have visibility of prior
		// writes by awaiting coroutines.
		void* oldValue = m_state.exchange(this, std::memory_order_acq_rel);
		if (oldValue != this)
		{
			// Wasn't already in 'set' state.
			// Treat old value as head of a linked-list of waiters
			// which we have now acquired and need to resume.
			auto* waiters = static_cast<awaiter*>(oldValue);
			while (waiters != nullptr)
			{
				// Read m_next before resuming the coroutine as resuming
				// the coroutine will likely destroy the awaiter object.
				auto* next = waiters->m_next;
				waiters->m_awaitingCoroutine.resume();
				waiters = next;
			}
		}
	}

	bool async_manual_reset_event::awaiter::await_suspend(
		coroutine_handle<> awaitingCoroutine) noexcept
	{
		// Special m_state value that indicates the event is in the 'set' state.
		const void* const setState = &m_event;

		// Remember the handle of the awaiting coroutine.
		m_awaitingCoroutine = awaitingCoroutine;

		// Try to atomically push this awaiter onto the front of the list.
		void* oldValue = m_event.m_state.load(std::memory_order_acquire);
		do
		{
			// Resume immediately if already in 'set' state.
			if (oldValue == setState) return false;

			// Update linked list to point at current head.
			m_next = static_cast<awaiter*>(oldValue);

			// Finally, try to swap the old list head, inserting this awaiter
			// as the new list head.
		} while (!m_event.m_state.compare_exchange_weak(
			oldValue,
			this,
			std::memory_order_release,
			std::memory_order_acquire));

		// Successfully enqueued. Remain suspended.
		return true;
	}

}
