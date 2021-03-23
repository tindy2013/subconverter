#ifndef DEFER_H_INCLUDED
#define DEFER_H_INCLUDED

#define CONCAT(a,b) a ## b
#define DO_CONCAT(a,b) CONCAT(a,b)
template <typename T> class __defer_struct final {private: T fn; bool __cancelled = false; public: explicit __defer_struct(T func) : fn(std::move(func)) {} ~__defer_struct() {if(!__cancelled) fn();} void cancel() {__cancelled = true;} };
//#define defer(x) std::unique_ptr<void> DO_CONCAT(__defer_deleter_,__LINE__) (nullptr, [&](...){x});
#define defer(x) __defer_struct DO_CONCAT(__defer_deleter,__LINE__) ([&](...){x;});

#endif // DEFER_H_INCLUDED
