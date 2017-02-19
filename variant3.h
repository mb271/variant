#ifndef _INCLUDE_VARIANT_H_
#define _INCLUDE_VARIANT_H_

#include <cstddef>
#include <type_traits>
#include <utility>
//#include "max_sizeof2.h"
#include <cassert> // todo remove

namespace mb
{

template <std::size_t C, typename T, typename... Ts>
struct variant_alternative
{
    using type = typename variant_alternative<C-1, Ts...>::type;
};

template <typename T, typename... Ts>
struct variant_alternative<0, T, Ts...>
{
    using type = T;
};

template <std::size_t I, typename... Types>
using variant_alternative_t = typename variant_alternative<I, Types...>::type;

template <typename T>
struct is_destructable
{
    static constexpr bool value = std::is_class<T>::value || std::is_union<T>::value;
};


template <typename T, typename... Ts>
struct variant_index
{
private:
    template <typename SOURCE_T, typename TARGET_T>
    struct convert_type
    {
    public:
        static constexpr unsigned SAME = 1;
        static constexpr unsigned CONVERTIBLE = 3;
        static constexpr unsigned NOT_CONVERTIBLE = 4;

    private:
        struct ONE {char a[1];};
        struct TWO {char a[3];};
        struct THREE {char a[4];};

        template <typename So, typename Ta>
        struct helper
        {
            static constexpr TWO func(Ta);
            static constexpr THREE func(...);

            static constexpr unsigned value = sizeof(func(So{}));
        };

        template <typename So>
        struct helper<So, So>
        {
            static constexpr unsigned value = SAME;
        };

    public:
        static constexpr unsigned value = helper<SOURCE_T, TARGET_T>::value;
    };

    template <typename SOURCE_T, typename TARGET_T>
    static constexpr unsigned convert_type_v = convert_type<SOURCE_T, TARGET_T>::value;


    template <
        typename U,
        std::size_t POS,
        std::size_t SEL_POS,
        unsigned TYPE,
        typename V,
        typename... Vs>
    struct get_index_helper
    {
        static constexpr unsigned cur_convert_type = convert_type_v<U, V>;
        static_assert(TYPE != convert_type<U, V>::SAME
                || cur_convert_type != convert_type<U, V>::SAME, "Ambiguous types!");
        static constexpr unsigned new_type =
            cur_convert_type < TYPE ? cur_convert_type :
            cur_convert_type == TYPE && TYPE == convert_type<U, V>::CONVERTIBLE ? 2 : TYPE;
        static constexpr std::size_t new_pos = new_type < TYPE ? POS : SEL_POS;
        static constexpr std::size_t value =
            get_index_helper<U, POS+1, new_pos, new_type, Vs...>::value;
    };

    template <typename U, std::size_t POS, std::size_t SEL_POS, unsigned TYPE, typename V>
    struct get_index_helper<U, POS, SEL_POS, TYPE, V>
    {
        static constexpr unsigned cur_convert_type = convert_type_v<U, V>;
        static_assert(TYPE != convert_type<U, V>::SAME
                || cur_convert_type != convert_type<U, V>::SAME, "Ambiguous types!");
        static constexpr unsigned new_type =
            cur_convert_type < TYPE ? cur_convert_type :
            cur_convert_type == TYPE && TYPE == convert_type<U, V>::CONVERTIBLE ? 2 : TYPE;
        static_assert(new_type != 2, "Ambiguous types!");
        static_assert(new_type < convert_type<U, V>::NOT_CONVERTIBLE, "No matching type!");
        static constexpr std::size_t value = new_type < TYPE ? POS : SEL_POS;
    };
public:
    static constexpr std::size_t value =
        get_index_helper<
            std::remove_reference_t<T>,
            0,
            0,
            convert_type<T, T>::NOT_CONVERTIBLE,
            Ts...>::value;
};

template <typename T, typename... Ts>
constexpr std::size_t variant_index<T, Ts...>::value;

template <typename T, typename... Ts>
static constexpr std::size_t variant_index_v = variant_index<T, Ts...>::value;

std::size_t variant_npos = -1;

template <typename T, typename U>
struct CaptureObject
{
    CaptureObject(std::remove_reference_t<U> &&o)
    : o(std::move(o))
    {}

    constexpr U get()
    {
        return std::forward<U>(o);
    }

    std::remove_reference_t<U> &&o;
};


template <typename... Ts>
union variant_data;

template <typename T, typename... Rest>
union variant_data<T, Rest...>
{
    using DummyT = typename variant_data<Rest...>::DummyT;

    variant_data()
    : val{}
    {}

    variant_data(const DummyT *dummy)
    : rest(dummy)
    {}

    variant_data(const T &val)
    : val(val)
    {}

    variant_data(const variant_data<Rest...> &other_rest, std::size_t i)
    : rest(other_rest, i)
    {}

//    variant_data(const variant_data &other, std::size_t i)
//    : variant_data(i==0 ? variant_data(other.val) : variant_data(other.rest, i-1))
//    {}

    template <typename U>
    constexpr variant_data(CaptureObject<T, U> &&v)
    : val(v.get())
    {}

    template <typename U, typename V>
    constexpr variant_data(CaptureObject<U, V> &&v)
    : rest(std::move(v))
    {}

    ~variant_data()
    {}

    void operator=(const T &v)
    {
        val = v;
    }

    template <typename U>
    void operator=(const U &v)
    {
        rest = v;
    }

    void destroy(const T*)
    {
        if (is_destructable<T>::value)
        {
            val.~T();
        }
    }

    template <typename U>
    void destroy(const U*)
    {
        rest.destroy(static_cast<U*>(nullptr));
    }

    void copy_construct(const T &other)
    {
        new (&val) T(other);
    }

    template <typename U>
    void copy_construct(const U &other)
    {
        rest.copy_construct(other);
    }

    T val;
    variant_data<Rest...> rest;
};

template <typename T>
union variant_data<T>
{
    class DummyT {};

    variant_data()
    : val{}
    {}

    variant_data(const DummyT*)
    : dummy{}
    {}

    variant_data(const variant_data &other, std::size_t)
    : val(other.val)
    {}

    template <typename U>
    constexpr variant_data(CaptureObject<T, U> &&v)
    : val(v.get())
    {}

    ~variant_data()
    {}

    void operator=(const T &v)
    {
        val = v;
    }


    void destroy(const T*)
    {
        if (is_destructable<T>::value)
        {
            val.~T();
        }
    }

    void copy_construct(const T &other)
    {
        new (&val) T(other);
    }

    T val;
    DummyT dummy;
};

template <typename... Ts> class variant;

template <typename T, typename... Ts> constexpr T& get(variant<Ts...> &v);

template <typename... Ts>
class variant
{
private:
    typedef void (*DestructorT)(variant<Ts...>&);
    typedef void (*CopyConstructorT)(variant<Ts...>&, const variant<Ts...>&);

    static const DestructorT destructors[sizeof...(Ts)];
    static const CopyConstructorT constructors[sizeof...(Ts)];

    template <typename T>
    static void destroy(variant<Ts...> &v)
    {
        if (is_destructable<T>::value)
        {
            v.data.destroy(static_cast<T*>(nullptr));
            // get<T>(v).~T();
        }
    }

    template <typename T>
    static void copy_construct(variant<Ts...> &target, const variant<Ts...> &source)
    {
        target.data.copy_construct(get<T>(source));
    }

    template <std::size_t pos, typename U, typename... Us>
    struct get_helper_pos
    {
        static constexpr variant_alternative_t<pos, U, Us...> const&
            get(const variant_data<U, Us...> &a)
        {
            return get_helper_pos<pos-1, Us...>::get(a.rest);
        }

        static constexpr variant_alternative_t<pos, U, Us...>&
            get(variant_data<U, Us...> &a)
        {
            return get_helper_pos<pos-1, Us...>::get(a.rest);
        }
    };

    template <typename U, typename... Us>
    struct get_helper_pos<0, U, Us...>
    {
        static constexpr U const& get(const variant_data<U, Us...> &a)
        {
            return a.val;
        }

        static constexpr U& get(variant_data<U, Us...> &a)
        {
            return a.val;
        }
    };

public:
    variant()
    {
        _index = 0;
    }

    variant(variant &other)
    : variant(static_cast<const variant&>(other))
    {}

    variant(const variant &other)
    : data(static_cast<const typename variant_data<Ts...>::DummyT*>(nullptr))
    {
        _index = variant_npos;
        constructors[other.index()](*this, other);
        _index = other.index();
    }

    template <typename U>
    constexpr variant(U &&v)
    : data(CaptureObject<variant_alternative_t<variant_index_v<U, Ts...>, Ts...>,
                         U>(std::move(v)))
    {
        _index = variant_index_v<std::remove_reference_t<U>, Ts...>;
    }

    ~variant()
    {
        if (_index < sizeof...(Ts))
            destructors[_index](*this);
    }

    template <typename U>
    variant<Ts...>& operator=(const U &v)
    {
        const auto old_index = _index;
        _index = variant_npos;

        if (old_index < sizeof...(Ts))
            destructors[old_index](*this);

        data = v;
        _index = variant_index_v<U, Ts...>;
        return *this;
    }

    constexpr std::size_t index() const noexcept
    {
        return _index;
    }

private:
    template <std::size_t pos, typename... Us>
    friend constexpr variant_alternative_t<pos, Us...> const& get(const variant<Us...>&);

    template <std::size_t pos, typename... Us>
    friend constexpr variant_alternative_t<pos, Us...>& get(variant<Us...>&);

    std::size_t _index = variant_npos;
    variant_data<Ts...> data;
};

template <typename... Ts>
const typename variant<Ts...>::DestructorT variant<Ts...>::destructors[sizeof...(Ts)] =
    {variant<Ts...>::destroy<Ts>...};

template <typename... Ts>
const typename variant<Ts...>::CopyConstructorT variant<Ts...>::constructors[sizeof...(Ts)]
    {variant<Ts...>::copy_construct<Ts>...};


template <std::size_t pos, typename... Ts>
constexpr variant_alternative_t<pos, Ts...> const& get(const variant<Ts...> &v)
{
    return variant<Ts...>::template get_helper_pos<pos, Ts...>::get(v.data);
}

template <std::size_t pos, typename... Ts>
constexpr variant_alternative_t<pos, Ts...>& get(variant<Ts...> &v)
{
    return variant<Ts...>::template get_helper_pos<pos, Ts...>::get(v.data);
}

template <typename T, typename... Ts>
constexpr T const& get(const variant<Ts...> &v)
{
    return get<variant_index_v<T, Ts...>>(v);
}

template <typename T, typename... Ts>
constexpr T& get(variant<Ts...> &v)
{
    return get<variant_index_v<T, Ts...>>(v);
}


} //  namesapce mb

#endif
