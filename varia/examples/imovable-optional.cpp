//
//
//

template <typename Factory>
struct deferred {
    Factory factory_;
    using result_t = std::invoke_result_t<Factory&>;
    operator result_t() {
        return factory_();
    }
};

template <typename Factory>
deferred<std::decay_t<Factory>> make_deferred(Factory&& f) {
    return {std::forward<Factory>(f)};
}


struct A {
    int m_i{5};
    A(int i = 13): m_i(i){
    }

    ~A() {
    }

    A(const A&) = delete;
    A& operator=(const A&) = delete;

    A(A&&) noexcept = delete;
    A& operator=(A&&) noexcept = delete;

    //A(const A&) {
    //}
    //A& operator=(const A&) {
    //    return *this;
    //}

    //A(A&&) noexcept {
    //}
    //A& operator=(A&&) noexcept {
    //    return *this;
    //}
};

A make_A() {return A{}; }

int main() {
    std::optional<A> oa0;
    //oa0.emplace(make_A()); // this is not working due to the removal of the move constructor

    oa0.emplace(ex::make_deferred([&] {
        return make_A();})
    ); // this is working due to the return copy ellision
}

