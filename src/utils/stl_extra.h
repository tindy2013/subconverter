#ifndef STL_EXTRA_H_INCLUDED
#define STL_EXTRA_H_INCLUDED

#include <vector>

template <typename T> inline void eraseElements(std::vector<T> &target)
{
    target.clear();
    target.shrink_to_fit();
}

template <typename T> inline void eraseElements(T &target)
{
    T().swap(target);
}

#if __cpp_concepts >= 201907L // C++20 concepts supported  (g++-10 or clang++-10)

template <typename Container>
concept ConstIterable = requires(Container a) {
    { a.cbegin() } -> std::same_as<typename Container::const_iterator>;
    { a.cend() } -> std::same_as<typename Container::const_iterator>;
    typename Container::const_reference;
};

template <typename Container>
concept Iterable = requires(Container a) {
    { a.begin() } -> std::same_as<typename Container::iterator>;
    { a.end() } -> std::same_as<typename Container::iterator>;
    typename Container::reference;
};

template <typename ConstIterableContainer>
requires ConstIterable<ConstIterableContainer>
inline bool none_of(const ConstIterableContainer &container, std::function<bool(typename ConstIterableContainer::const_reference)> func)
{
    return std::none_of(container.cbegin(), container.cend(), func);
}

#else // __cpp_concepts >= 201907L

template <typename Container>
inline bool none_of(const Container &container, std::function<bool(typename Container::const_reference)> func)
{
    return std::none_of(container.cbegin(), container.cend(), func);
}

#endif // __cpp_concepts >= 201907L

#endif // STL_EXTRA_H_INCLUDED
