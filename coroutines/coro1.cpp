/*
 * https://www.modernescpp.com/index.php/a-coroutines-based-single-consumer-single-producer-workflow-by-ljubic-damir/
 */
/*
    P -> get_retutn_type -> ADR(handle) -> co_yield -> yield_value -> <wait> -+
    C -> co_await ADR -> Awaiter(promise)::await_ready() -> await_resume() <--+
         ADR::resume() 
	 co_await ADR -> Awaiter(promise)::await_ready() -> await_resume() -> ADR::resume
	   #1  0x0000555555556b1c in producer (frame_ptr=<optimized out>) at coro1.cpp:192 ----> co_yield
	   #2  producer (data=...) at coro1.cpp:189
	   #3  0x0000555555556efd in std::__n4861::coroutine_handle<AudioDataResult::promise_type>::resume (this=0x7fffffffe688) at /usr/include/c++/11/coroutine:231
	   #4  AudioDataResult::resume (this=0x7fffffffe688) at coro1.cpp:177
	   #5  AudioDataResult::resume (this=0x7fffffffe688) at coro1.cpp:177
	   #6  consumer(_Z8consumerR15AudioDataResult.Frame *) (frame_ptr=0x7ffff0000b70) at coro1.cpp:210
	   #7  0x0000555555557095 in consumer (audioDataResult=...) at coro1.cpp:200
*/

#include <iostream>
#include <vector>
#include <coroutine>
#include <chrono>
//#include <format>
#include <source_location>
#include <string_view>
#include <thread>
#include <utility>
#include <functional>
#include <memory>
#include <algorithm>
#include <iterator>
#include <atomic>



namespace 
{
    void log(const std::string_view message,
             const std::source_location location = std::source_location::current())
    {
        // TODO: use lock
	// no <format> yet
	//std::clog << std::format("T:{} {}:{}: {} \n",
	//                         std::this_thread::get_id(),
	//			 location.function_name(),
	//			 ocation.line(),
	//			 message);
        std::clog //<< location.file_name() << '('
                  //<< location.column() << ") `"
		  << std::hex << std::this_thread::get_id() << ": " << std::dec
                  << location.function_name() << ": "
                  << location.line() << ':'
                  << message << '\n'
		  ;
    }
    void log(const std::source_location location = std::source_location::current())
    {
        std::clog //<< location.file_name() << '('
                  //<< location.column() << ") `"
		  << std::hex << std::this_thread::get_id() << ": " << std::dec
                  << location.function_name() << ": "
                  << location.line() 
		  << '\n'
		  ;
    }


    template <typename InputIterator>
    void printIterable(InputIterator first, InputIterator last)
    {
        using value_type = std::decay_t<decltype(*first)>;
        std::cout << '[';
        if constexpr (std::is_same_v<std::uint8_t, value_type>) {
            std::copy(first, std::prev(last), std::ostream_iterator<std::uint16_t>(std::cout, ", "));
            std::cout << static_cast<std::uint16_t>(*std::prev(last)) << "]\n";
        }
        else
        {
            std::copy(first, std::prev(last), std::ostream_iterator<value_type>(std::cout, ", "));
            std::cout << *std::prev(last) << "]\n";
        }
    }

    template <typename Container>
    void printContainer(const Container& container)
    {
        printIterable(std::cbegin(container), std::cend(container));
    }
} // namespace


class [[nodiscard]] AudioDataResult final
{
    public:
        class promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        // Predefined interface that has to be specify in order to implement
        // coroutine's state-machine transitions
        class promise_type
        {

            public:

                using value_type = std::vector<int>;

                AudioDataResult get_return_object()
                {
                    return AudioDataResult{handle_type::from_promise(*this)};
                }
                std::suspend_never initial_suspend() noexcept { return {}; }
                std::suspend_always final_suspend() noexcept { return {}; }
                void return_void() {}
                void unhandled_exception()
                {
                    std::rethrow_exception(std::current_exception());
                }

                // Generates the value and suspend the "producer"
                template <typename Data>
                requires std::convertible_to<std::decay_t<Data>, value_type>
                std::suspend_always yield_value(Data&& value)
                {
                    data_ = std::forward<Data>(value);
                    data_ready_.store(true, std::memory_order::release);
                    return {};
                }

                // Awaiter interface: for consumer waiting on data being ready
                struct AudioDataAwaiter
                {
                    explicit AudioDataAwaiter(promise_type& promise) noexcept: promise_(promise) {log();}

                        bool await_ready() const { return promise_.data_ready_.load(std::memory_order::acquire);}

                    void await_suspend(handle_type) const
                    {
                        while(not promise_.data_ready_.exchange(false)) {
                             std::this_thread::yield();
                        }
                    }
                    // move assignment at client invocation side: const auto data = co_await audioDataResult;
                    // This requires that coroutine's result type provides the co_await unary operator
                    value_type&& await_resume() const
                    {
                        return std::move(promise_.data_);
                    }

                    private:
                        promise_type& promise_;
                };//Awaiter interface


            private:
                value_type data_;
                std::atomic<bool> data_ready_;
        }; //promise_type interface


        auto operator co_await() noexcept
        {
            return promise_type::AudioDataAwaiter{handle_.promise()};
        }

        // Make the result type move-only, due to ownership over the handle
        AudioDataResult(const AudioDataResult&) = delete;
        AudioDataResult& operator=(const AudioDataResult&) = delete;

        AudioDataResult(AudioDataResult&& other) noexcept: handle_(std::exchange(other.handle_, nullptr)) {log();}
        AudioDataResult& operator=(AudioDataResult&& other) noexcept
        {
            log();
            using namespace std;
            AudioDataResult tmp = std::move(other);
            swap(*this, tmp);
            return *this;
        }

        AudioDataResult() {log();}

        // d-tor: RAII
        ~AudioDataResult() { if (handle_) {log(); handle_.destroy();}}

        // For resuming the producer - at the point when the data are consumed
        void resume() {if (not handle_.done()) { log(); handle_.resume();}}


    private:
        AudioDataResult(handle_type handle) noexcept : handle_(handle) {log();}

    private:
    handle_type handle_;
}; // AudioDataResult


using data_type = std::vector<int>;
AudioDataResult producer(const data_type& data)
{
    for (std::size_t i = 0; i < 5; ++i) {
        log();
        co_yield data;
    }
    co_yield data_type{}; // exit criteria

    co_return;
}

AudioDataResult consumer(AudioDataResult& audioDataResult)
{
    for(;;)
    {
        log();
        const auto data = co_await audioDataResult;
        if (data.empty()) {std::cout << "No data - exit!\n"; break;}
        std::cout << "  Data received:";
        printContainer(data);

        audioDataResult.resume(); // resume producer
    }
    co_return;
}

int main()
{
    {
        const data_type data = {1, 2, 3, 4};
        auto audioDataProducer = producer(data);
        std::thread t ([&]{auto audioRecorded = consumer(audioDataProducer);});
        t.join();
    }

    std::cout << "bye-bye!\n";
    return 0;
}

