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

template <typename Container, typename Element>
concept ConstIterable = requires(Container a, Element b) {
    { a.cbegin() } -> std::same_as<typename Container::const_iterator>;
    { a.cend() } -> std::same_as<typename Container::const_iterator>;
    typename Container::const_reference;
};

template <typename Container, typename Element>
concept Iterable = requires(Container a, Element b) {
    { a.begin() } -> std::same_as<typename Container::iterator>;
    { a.end() } -> std::same_as<typename Container::iterator>;
    typename Container::reference;
};

template <typename ConstIterableContainer>
requires ConstIterable<ConstIterableContainer, typename ConstIterableContainer::value_type>
inline bool none_of(ConstIterableContainer &container, std::function<bool(typename ConstIterableContainer::const_reference)> func)
{
    return std::none_of(container.cbegin(), container.cend(), func);
}

#endif // STL_EXTRA_H_INCLUDED
