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

