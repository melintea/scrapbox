#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

#include <cstdio>

namespace ex = stdexec;

int main() {
    //auto sched = ex::get_parallel_scheduler(); //  crash
    exec::static_thread_pool pool(3);
    auto                     sched = pool.get_scheduler();

    auto fun   = [](int i) { 
        std::printf("  %d\n", i);
        return i * i; 
    };

    // Build a lazy pipeline: three squares, computed in parallel.
    auto work = ex::when_all(ex::on(sched, ex::just(0) | ex::then(fun)),
                             ex::on(sched, ex::just(1) | ex::then(fun)),
                             ex::on(sched, ex::just(2) | ex::then(fun)));

    // Launch the work and wait for the result.
    auto [i, j, k] = ex::sync_wait(std::move(work)).value();
    std::printf("%d %d %d\n", i, j, k); // prints "0 1 4"
}

