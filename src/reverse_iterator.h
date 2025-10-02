// Taken from https://gist.github.com/arvidsson/7231973

#ifndef SOTERIA_REVERSE_ITERATOR_H
#define SOTERIA_REVERSE_ITERATOR_H

template <typename T>
class reverse_range
{
    T &m_x;
    
public:
    explicit reverse_range(T &x) : m_x(x) {}
    
    auto begin() const -> decltype(this->m_x.rbegin())
    {
        return m_x.rbegin();
    }
    
    auto end() const -> decltype(this->m_x.rend())
    {
        return m_x.rend();
    }
};
 
template <typename T>
reverse_range<T> reverse_iterate(T &x)
{
    return reverse_range<T>(x);
}

#endif // SOTERIA_REVERSE_ITERATOR_H
