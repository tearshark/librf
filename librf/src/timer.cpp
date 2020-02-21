#include "../librf.h"

RESUMEF_NS
{

	timer_manager::timer_manager()
	{
		_added_timers.reserve(128);
	}

	timer_manager::~timer_manager()
	{
		clear();
	}
	
	void timer_manager::call_target_(const timer_target_ptr & sptr, bool canceld)
	{
		auto cb = std::move(sptr->cb);
		sptr->st = timer_target::State::Invalid;
#if _DEBUG
		sptr->_manager = nullptr;
#endif

		if(cb) cb(canceld);
	}

	void timer_manager::clear()
	{
		auto _atimer = std::move(_added_timers);
		for (auto & sptr : _atimer)
			call_target_(sptr, true);

		auto _rtimer = std::move(_runing_timers);
		for (auto & kv : _rtimer)
			call_target_(kv.second, true);
	}

	detail::timer_target_ptr timer_manager::add_(const timer_target_ptr & sptr)
	{
		assert(sptr);
		assert(sptr->st == timer_target::State::Invalid);
#if _DEBUG
		assert(sptr->_manager == nullptr);
		sptr->_manager = this;
#endif

		sptr->st = timer_target::State::Added;
		_added_timers.push_back(sptr);

		return sptr;
	}

	bool timer_manager::stop(const timer_target_ptr & sptr)
	{
		if (!sptr || sptr->st == timer_target::State::Invalid) 
			return false;
#if _DEBUG
		assert(sptr->_manager == this);
#endif
		sptr->st = timer_target::State::Invalid;

		return true;
	}

	void timer_manager::update()
	{
		if (_added_timers.size() > 0)
		{
			for (auto & sptr : _added_timers)
			{
				if (sptr->st == timer_target::State::Added)
				{
					sptr->st = timer_target::State::Runing;
					_runing_timers.insert({ sptr->tp, sptr });
				}
				else
				{
					assert(sptr->st == timer_target::State::Invalid);
					call_target_(sptr, true);
				}
			}

			_added_timers.clear();
		}

		if (_runing_timers.size() > 0)
		{
			auto now_ = clock_type::now();
			timer_map_type _timers = std::move(_runing_timers);

			auto iter = _timers.begin();
			for (; iter != _timers.end(); ++iter)
			{
				auto & kv = *iter;
				if (kv.first > now_)
					break;

				call_target_(kv.second, kv.second->st == timer_target::State::Invalid);
			}
			_timers.erase(_timers.begin(), iter);

			_runing_timers = std::move(_timers);
		}
	}
}