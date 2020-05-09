/************************************************
* @ Bright Dream Robotics-Fundamental Research
* @ Author BIP:zhangwencong
* @ V1.0
*************************************************/

#pragma once

namespace milk
{

namespace concurrency
{

namespace details
{

struct stop_callback_base
{
    void(*callback)(stop_callback_base*) = nullptr;
    stop_callback_base* next = nullptr;
    stop_callback_base** prev = nullptr;
    bool* is_removed = nullptr;
    std::atomic<bool> finished_executing{false};

    void execute() noexcept
    {
        callback(this);
    }

protected:
    stop_callback_base(void(*fnptr)(stop_callback_base*))
        : callback(fnptr) {}

    ~stop_callback_base() = default;

};

struct stop_state
{
    static constexpr uint64_t kStopRequestedFlag = 1u;
    static constexpr uint64_t kLockedFlag = 2u;
    static constexpr uint64_t kTokenRefIncrement = 4u;
    static constexpr uint64_t kSourceRefIncrement = static_cast<uint64_t>(1u) << 33u;

    static constexpr uint64_t kZeroRef = kTokenRefIncrement + kSourceRefIncrement;
    static constexpr uint64_t kLockedAndZeroRef = kZeroRef + kZeroRef;
    static constexpr uint64_t kLockAndStopedFlag = kStopRequestedFlag | kLockedFlag;
    static constexpr uint64_t kUnlockAndTokenRefIncrement = kLockedFlag - kTokenRefIncrement;
    static constexpr uint64_t klockAndTokenRefIncrement = kLockedFlag + kTokenRefIncrement;

    // bit 0 - stop-requested
    // bit 1 - locked43
    // bits 2-32 - token ref count (31 bits)
    // bits 33-63 - source ref count (31 bits)
    std::atomic<uint64_t> state_{kSourceRefIncrement};

    stop_callback_base* head_ = nullptr;
    std::thread::id signalling_{};

    static bool is_locked(uint64_t state) noexcept
    {
        return (state & kLockedFlag) != 0;
    }

    static bool is_stop_requested(uint64_t state) noexcept
    {
        return (state & kStopRequestedFlag) != 0;
    }

    static bool is_stop_requestable(uint64_t state) noexcept
    {
        return is_stop_requested(state) || (state >= kSourceRefIncrement);
    }

    bool try_lock_and_signal_until_signalled() noexcept
    {
        uint64_t old_state = state_.load(std::memory_order_acquire);
        do
        {
            if (is_stop_requested(old_state))
            {
                return false;
            }
            while (is_locked(old_state))
            {
                std::this_thread::yield();
                old_state = state_.load(std::memory_order_acquire);
                if (is_stop_requested(old_state))
                {
                    return false;
                }
            }
        } while(!state_.compare_exchange_weak(old_state, old_state | kLockAndStopedFlag, std::memory_order_acq_rel, std::memory_order_acquire));
        return true;
    }

    void lock() noexcept
    {
        uint64_t old_state = state_.load(std::memory_order_relaxed);
        do
        {
            while (is_locked(old_state))
            {
                std::this_thread::yield();
                old_state = state_.load(std::memory_order_relaxed);
            }
        }
        while (!state_.compare_exchange_weak(old_state, old_state | kLockedFlag, std::memory_order_acquire, std::memory_order_relaxed));
    }

    void unlock() noexcept
    {
        state_.fetch_sub(kLockedFlag, std::memory_order_release);
    }

    void unlock_and_increment_token_ref_count() noexcept
    {
        state_.fetch_sub(kUnlockAndTokenRefIncrement, std::memory_order_release);
    }

    void unlock_and_decrement_token_ref_count() noexcept
    {
        if (state_.fetch_sub(klockAndTokenRefIncrement, std::memory_order_acq_rel) < kLockedAndZeroRef)
        {
			clear_callback();
            delete this;
        }
    }

public:
    void add_token_reference() noexcept
    {
        state_.fetch_add(kTokenRefIncrement, std::memory_order_relaxed);
    }

    void remove_token_reference() noexcept
    {
        auto old_state = state_.fetch_sub(kTokenRefIncrement, std::memory_order_acq_rel);
        if (old_state < kZeroRef)
        {
			clear_callback();
            delete this;
        }
    }

    void add_source_reference() noexcept
    {
        state_.fetch_add(kSourceRefIncrement, std::memory_order_relaxed);
    }

    void remove_source_reference() noexcept
    {
        auto old_state = state_.fetch_sub(kSourceRefIncrement, std::memory_order_acq_rel);
        if (old_state < kZeroRef)
        {
            clear_callback();
            delete this;
        }
    }

    bool request_stop() noexcept
    {
        if (!try_lock_and_signal_until_signalled())
        {
            return false;
        }
        signalling_ = std::this_thread::get_id();

        while (head_)
        {
            auto* cb = head_;
            head_ = cb->next;
            const bool any_more = head_ != nullptr;
            if (any_more)
            {
                head_->prev = &head_;
            }
            cb->prev = nullptr;
            unlock();
            bool removed = false;
            cb->is_removed = &removed;
            cb->execute();
            if (!removed)
            {
                cb->is_removed = nullptr;
                cb->finished_executing.store(true, std::memory_order_release);
            }
            if (!any_more)
            {
                return true;
            }
            lock();
        }
        unlock();
        return true;
    }

    bool is_stop_requested() noexcept
    {
        return is_stop_requested(state_.load(std::memory_order_acquire));
    }

    bool is_stop_requestable() noexcept
    {
        return is_stop_requestable(state_.load(std::memory_order_acquire));
    }

    bool try_add_callback(stop_callback_base* cb, bool increment_ref_count_if_successful) noexcept
    {
        uint64_t old_state;
        goto __load_state;
        do
        {
            goto __check_state;
            do
            {
                std::this_thread::yield();
__load_state:
                old_state = state_.load(std::memory_order_acquire);
__check_state:
                if (is_stop_requested(old_state))
                {
                    cb->execute();
                    return false;
                }
                else if (!is_stop_requestable(old_state))
                {
                    return false;
                }
            } while (is_locked(old_state));
        }while (!state_.compare_exchange_weak(old_state, old_state | kLockedFlag, std::memory_order_acquire));

        // callback入队列
        cb->next = head_;
        if (cb->next)
        {
            cb->next->prev = &cb->next;
        }
        cb->prev = &head_;
        head_ = cb;

        if (increment_ref_count_if_successful)
        {
            unlock_and_increment_token_ref_count();
        }
        else
        {
            unlock();
        }
        return true;
    }

    void remove_callback(stop_callback_base* cb) noexcept
    {
        lock();
        if (cb->prev)
        {
            *cb->prev = cb->next;
            if (cb->next)
            {
                cb->next->prev = cb->prev;
            }
            unlock_and_decrement_token_ref_count();
            return;
        }
        unlock();
        // Callback要么已经执行，要么正在另一个线程上并发执行。
        if (signalling_ == std::this_thread::get_id())
        {
            // 若Callback是当前线程上执行的回调或当前仍在执行并在回调中注销自身的回调。
            if (cb->is_removed)
            {
                // 当前在Callback执行中，告知request_stop()知道对象即将被销毁，不能尝试访问
                *cb->is_removed = true;
            }
        }
        else
        {
            // 等待其他线程执行Callback完成
            while (!cb->finished_executing.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
        }
        remove_token_reference();
    }

    void clear_callback() noexcept
    {
		lock();
        stop_callback_base* cb = head_;
        head_ = nullptr;

        while (cb)
        {
            stop_callback_base* tmp = cb->next;
            cb->prev = nullptr;
            cb->next = nullptr;
            cb = tmp;
        }
        unlock();
    }

};

}//namespace details

struct nostopstate_t { explicit nostopstate_t() = default; };
inline constexpr nostopstate_t nostopstate{};

class stop_source;

template<typename Callback>
class stop_callback;

class stop_token
{
private:
    details::stop_state* state_;

    friend class stop_source;
    template<typename> friend class stop_callback;

    explicit stop_token(details::stop_state* state) noexcept
        : state_(state)
    {
        if (state_)
        {
            state_->add_token_reference();
        }
    }

public:
    stop_token() noexcept
        : state_(nullptr)
    {
    }

    stop_token(const stop_token& other) noexcept
        : state_(other.state_)
    {
        if (state_)
        {
            state_->add_token_reference();
        }
    }

    stop_token(stop_token&& other) noexcept
        : state_(std::exchange(other.state_, nullptr))
    {
    }

    ~stop_token()
    {
        if (state_)
        {
            state_->remove_token_reference();
        }
    }

    stop_token& operator=(const stop_token& other) noexcept
    {
        if (state_ != other.state_)
        {
            stop_token tmp{other};
            swap(tmp);
        }
        return *this;
    }

    stop_token& operator=(stop_token&& other) noexcept
    {
        stop_token tmp{std::move(other)};
        swap(tmp);
        return *this;
    }

    void swap(stop_token& other) noexcept
    {
        std::swap(state_, other.state_);
    }

    [[nodiscard]]
    bool stop_requested() const noexcept
    {
        return state_ && state_->is_stop_requested();
    }

    [[nodiscard]]
    bool stop_possible() const noexcept
    {
        return state_ && state_->is_stop_requestable();
    }

    [[nodiscard]]
    bool operator==(const stop_token& other) noexcept
    {
        return state_ == other.state_;
    }

    [[nodiscard]]
    bool operator!=(const stop_token& other) noexcept
    {
        return state_ != other.state_;
    }

};

class stop_source
{
private:
    details::stop_state* state_;

public:
    stop_source()
        : state_(new details::stop_state())
    {
    }

    explicit stop_source(nostopstate_t) noexcept
        : state_(nullptr)
    {
    }

    ~stop_source()
    {
        if (state_)
        {
            state_->remove_source_reference();
        }
    }

    stop_source(const stop_source& other) noexcept
        : state_(other.state_)
    {
        if (state_)
        {
            state_->add_source_reference();
        }
    }

    stop_source(stop_source&& other) noexcept
        : state_(std::exchange(other.state_, nullptr))
    {
    }

    stop_source& operator=(stop_source&& other) noexcept
    {
        stop_source tmp{std::move(other)};
        swap(tmp);
        return *this;
    }

    stop_source& operator=(const stop_source& other) noexcept
    {
        if (state_ != other.state_)
        {
            stop_source tmp{other};
            swap(tmp);
        }
        return *this;
    }

    [[nodiscard]]
    bool stop_requested() const noexcept
    {
        return state_ && state_->is_stop_requested();
    }

    [[nodiscard]]
    bool stop_possible() const noexcept
    {
        return state_ != nullptr;
    }

	void make_possible()
	{
        if (state_ == nullptr)
        {
            details::stop_state* st = new details::stop_state();
            details::stop_state* tmp = nullptr;
            if (!std::atomic_compare_exchange_strong_explicit(
                reinterpret_cast<std::atomic<details::stop_state*>*>(&state_), &tmp, st, std::memory_order_release, std::memory_order_acquire))
            {
                st->remove_source_reference();
            }
        }
	}

    bool request_stop() const noexcept
    {
        if (state_)
        {
            return state_->request_stop();
        }
        return false;
    }

    [[nodiscard]]
    stop_token get_token() const noexcept
    {
        return stop_token{state_};
    }

    void clear_callback() const noexcept
    {
		if (state_)
		{
			state_->clear_callback();
		}
    }

    void swap(stop_source& other) noexcept
    {
        std::swap(state_, other.state_);
    }

    [[nodiscard]]
    bool operator==(const stop_source& other) noexcept
    {
        return state_ == other.state_;
    }

    [[nodiscard]]
    bool operator!=(const stop_source& other) noexcept
    {
        return state_ != other.state_;
    }

};

template <typename Callback>
class [[nodiscard]] stop_callback : private details::stop_callback_base
{
private:
    using details::stop_callback_base::execute;
    using callack_type = Callback;
    details::stop_state* state_;
    callack_type callack_;

    void execute() noexcept
    {
        callack_();
    }

public:
    using callback_type = Callback;

    template<typename Callable, typename = typename std::enable_if<std::is_constructible<Callback, Callable>::value, int>::type>
    explicit stop_callback(const stop_token& token, Callable&& cb) noexcept(std::is_nothrow_constructible<Callback, Callable>::value)
        : details::stop_callback_base([](details::stop_callback_base* that) noexcept { static_cast<stop_callback*>(that)->execute(); })
        , state_(nullptr)
        , callack_(static_cast<Callable&&>(cb))
    {
        if (token.state_ && token.state_->try_add_callback(this, true))
        {
            state_ = token.state_;
        }
    }

    template<typename Callable, typename = typename std::enable_if<std::is_constructible<Callback, Callable>::value, int>::type>
    explicit stop_callback(stop_token&& token, Callable&& cb) noexcept(std::is_nothrow_constructible<Callback, Callable>::value)
        : details::stop_callback_base([](details::stop_callback_base* that) noexcept { static_cast<stop_callback*>(that)->execute(); })
        , state_(nullptr)
        , callack_(static_cast<Callable&&>(cb))
    {
        if (token.state_ && token.state_->try_add_callback(this, false))
        {
            state_ = std::exchange(token.state_, nullptr);
        }
    }

    ~stop_callback()
    {
        if (state_)
        {
            state_->remove_callback(this);
        }
    }

    stop_callback& operator=(const stop_callback&) = delete;
    stop_callback& operator=(stop_callback&&) = delete;
    stop_callback(const stop_callback&) = delete;
    stop_callback(stop_callback&&) = delete;

};

template<typename Callback>
stop_callback(stop_token, Callback)->stop_callback<Callback>;//C++17 deduction guides

}//namespace concurrency

}//namespace milk
