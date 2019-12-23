#pragma once

template <typename E> // E must should be an enum class
class Set
// Utility class that makes bitfield-based sets out of enums.
// TODO: implement on the basis of std::bitset?
{
public:
    constexpr Set() : bits{0} {}

    template <typename... Args>
    constexpr Set(E elem1, Args... otherElems)
    {
        // static_assert((int)elem1 < 32);
        Set(otherElems...);
        bits |= 1 << (int)elem1;
    }

    constexpr bool contains(E elem) const { return (bits & (1 << (int)elem)) != 0; }

    constexpr bool contains_all(const Set &other) const { return (bits & other.bits) == other.bits; }

    constexpr Set &operator+=(E elem)
    {
        set(elem);
        return *this;
    }
    constexpr Set &operator-=(E elem)
    {
        remove(elem);
        return *this;
    }

    constexpr bool operator&(E elem) const { return bits & (1 << (int)elem) != 0; }

    constexpr void set(E elem) { bits |= 1 << (int)elem; }
    constexpr void remove(E elem) { bits &= ~(1 << elem); }

private:
    unsigned int bits = 0;
};

