#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

#include <sstream>
#include <iostream>
#include <cassert>

namespace ex = stdexec;

int main() {
    //auto sched = ex::get_parallel_scheduler(); //  crash
    exec::static_thread_pool pool(6);
    auto                     sched = pool.get_scheduler();

    auto fun   = [](int i) { 
        std::ostringstream oss;
        oss << std::hex << std::this_thread::get_id() << ": " << i << '\n'; 
    std::cout << oss.str();
        return i * i; 
    };

    {
        // Build a lazy pipeline: three squares, computed in parallel.
        auto work = ex::when_all(ex::on(sched, ex::just(0) | ex::then(fun) | ex::then(fun)),
                                 ex::on(sched, ex::just(1) | ex::then(fun) | ex::then(fun)),
                                 ex::on(sched, ex::just(2) | ex::then(fun) | ex::then(fun)) //16
                                );

        // Launch the work and wait for the result.
        auto [i, j, k] = ex::sync_wait(std::move(work)).value();
        std::cout << std::dec << i << ' ' <<  j << ' ' << k << "\n"; 
        assert(i == 0 && j == 1 && k ==16);
    }
#if 0 // no working split
    {
        // distribute strategy 1
        // 1. A predecessor sender that produces our unique value
        auto predecessorSender = ex::just(42);

        // 2. Wrap it in 'split' so multiple children can safely read from it
        auto sharedVal = ex::split(std::move(predecessorSender));

        // 3. Use 'when_all' to fan out to children, passing the shared sender to each
        auto work = ex::when_all(
            sharedVal | ex::then([](int val) {
                std::cout << "Child A received: " << val << "\n";
            }),
            sharedVal | ex::then([](int val) {
                std::cout << "Child B received: " << val << "\n";
            })
        );

        // Run the pipeline
        ex::sync_wait(std::move(work));
    }
#endif
    {
        // distribute strategy 2
        auto work = ex::just(42)/*Predecessor*/ | ex::let_value(
        [](int& sharedVal/*42*/) {
            // Inside this lambda, 'sharedVal' is alive and stable.
            // We can pass copies or references of it to multiple children in when_all.
            return ex::when_all(
                ex::just() | ex::then([&sharedVal]() { std::cout << "A: " << sharedVal << "\n"; }),
                ex::just() | ex::then([&sharedVal]() { std::cout << "B: " << sharedVal << "\n"; })
            );
        });
    ex::sync_wait(std::move(work));
    }
}

