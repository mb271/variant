#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "variant3.h"
#include "../doctest/doctest/doctest.h"
#include <iostream>

using namespace mb;

enum class CID
{
    INVALID,
    DEFAULT,
    VALUE,
    LVALUE_COPY,
    RVALUE_Copy
};

std::ostream& operator<<(std::ostream &s, const CID &v)
{
    switch(v)
    {
        case CID::INVALID: s << "CID::INVALID"; break;
        case CID::DEFAULT: s << "CID::DEFAULT"; break;
        case CID::VALUE: s << "CID::VALUE"; break;
        case CID::LVALUE_COPY: s << "CID::LVALUE_COPY"; break;
        case CID::RVALUE_Copy: s << "CID::RVALUE_Copy"; break;
    }

    return s;
}

struct TestClass
{
    TestClass()
    : constructorid{CID::DEFAULT}
    {}

    TestClass(int a)
    : a(a)
    , constructorid{CID::VALUE}
    {}

    TestClass(const TestClass &other)
    : a(other.a)
    , constructorid{CID::LVALUE_COPY}
    {}

    TestClass(TestClass &&other)
    : a(other.a)
    , constructorid(CID::RVALUE_Copy)
    {}

    ~TestClass()
    {}

    bool operator==(const TestClass &rhs) const
    {
        return a == rhs.a;
    }

    int a = 10;

    // statistic members
    CID constructorid = CID::INVALID;
};

std::ostream& operator<<(std::ostream &s, const TestClass &t)
{
    s << "TestClass{" << t.a << "}";
    return s;
}

struct TestClass2
{
    operator TestClass&&()
    {
        return std::move(TestClass{42});
    }
};

template <typename T>
void printer1(T&)
{
    std::cout << "printer1 - lvalue" << std::endl;
}

template <typename T>
void printer1(const T&)
{
    std::cout << "printer1 - const lvalue" << std::endl;
}

template <typename T>
void printer1(T &&)
{
    std::cout << "printer1 - rvalue" << std::endl;
}

template <typename T>
void printer2(T &&v)
{
    std::cout << "printer 2 - ";
    printer1(std::forward<T>(v));
}

template <typename T>
struct Test
{
    Test(T&)
    {
        std::cout << "Test - lvalue" << std::endl;
    }

    Test(const T&)
    {
        std::cout << "Test - const lvalue" << std::endl;
    }

    Test(T&&)
    {
        std::cout << "Test - rvalue" << std::endl;
    }

    template <typename U>
    Test(const U&)
    {
        std::cout << "template Test - const lvalue" << std::endl;
    }

    template <typename U>
    Test(U&&)
    {
        std::cout << "template Test - rvalue" << std::endl;
    }
};


struct TestNoDefConstructor
{
    TestNoDefConstructor() = delete;
};



/*TEST_CASE("Playing around")
{
    variant<double, int> a;
    std::cout << "get<0>(a): " << get<0>(a) << std::endl;
    CHECK(get<double>(a) == 0.);

    int b = 10;
    printer1(b);
    printer1(static_cast<const int&>(b));
    printer1(int{});
    printer2(int{});

    Test<int>{b};
    Test<int>{static_cast<const int&>(b)};
    Test<int>{int{} };

    double d = 3.;
    Test<int>{d};
    Test<int>{static_cast<const double &>(d)};
    Test<int>{double{}};

}*/


TEST_CASE("testing variant default constructor")
{
    variant<int, TestClass> a;
    CHECK(a.index() == 0u);
    CHECK(get<0>(a) == int{});
    CHECK(get<int>(a) == int{});

    variant<TestClass, int> b;
    CHECK(a.index() == 0u);
    CHECK(get<0>(b) == TestClass{});
    CHECK(get<TestClass>(b) == TestClass{});
    CHECK(get<TestClass>(b).constructorid == CID::DEFAULT);
}


TEST_CASE("testing const lvalue constructor")
{
    int lval_a{12};
    variant<int, TestClass> a(lval_a);
    CHECK(a.index() == 0u);
    CHECK(get<0u>(a) == lval_a);

    double lval_b{6.};
    variant<int, TestClass, double> b(lval_b);
    CHECK(b.index() == 2);
    CHECK(get<2>(b) == lval_b);

    TestClass lval_c{42};
    variant<int, TestClass, double> c(lval_c);
    CHECK(c.index() == 1);
    CHECK(get<1>(c) == lval_c);
    CHECK(get<TestClass>(c).constructorid == CID::LVALUE_COPY);
}


TEST_CASE("testing rvalue constructor")
{
    variant<int, TestClass> a(TestClass{42});
    CHECK(a.index() == 1u);
    CHECK(get<1>(a) == TestClass{42});
    CHECK(get<1>(a).constructorid == CID::RVALUE_Copy);

    variant<TestClass, int> b(TestClass{42});
    CHECK(b.index() == 0u);
    CHECK(get<0>(b) == TestClass{42});
    CHECK(get<0>(b).constructorid == CID::RVALUE_Copy);

    variant<TestClass, double> c(TestClass2{});
    CHECK(c.index() == 0u);
    CHECK(get<0>(c).a == 42);
    CHECK(get<0>(c).constructorid == CID::RVALUE_Copy);

    variant<int, TestClass, double> d(TestClass2{});
    CHECK(d.index() == 1u);
    CHECK(get<1u>(d).a == 42);
    CHECK(get<1u>(d).constructorid == CID::RVALUE_Copy);

    variant<int, double, TestClass> e(TestClass2{});
    CHECK(e.index() == 2u);
    CHECK(get<2u>(e).a == 42);
    CHECK(get<2u>(e).constructorid == CID::RVALUE_Copy);

    variant<double, TestClass, int> f(TestClass{42});
    CHECK(f.index() == 1u);
    CHECK(get<1>(f) == TestClass{42});
    CHECK(get<1>(f).constructorid == CID::RVALUE_Copy);

    variant<double, int, TestClass> g(TestClass{42});
    CHECK(g.index() == 2u);
    CHECK(get<2>(g) == TestClass{42});
    CHECK(get<2>(g).constructorid == CID::RVALUE_Copy);
}


TEST_CASE("testing variant_index")
{
    struct NotConvertible{};
    CHECK((variant_index_v<int, int>) == 0);
    CHECK((variant_index_v<int, double, int>) == 1);
    CHECK((variant_index_v<int, int, double>) == 0);
    CHECK((variant_index_v<int, double, NotConvertible>) == 0);
    CHECK((variant_index_v<int, NotConvertible, double>) == 1);
    CHECK((variant_index_v<int, int, double, unsigned>) == 0);
    CHECK((variant_index_v<int, double, int, unsigned>) == 1);
    CHECK((variant_index_v<int, double, unsigned, int>) == 2);

    CHECK((variant_index_v<TestClass2, double, TestClass>) == 1);

    CHECK((variant_index_v<int&, double, unsigned, int>) == 2);
    CHECK((variant_index_v<double&, double, unsigned, int>) == 0);
    CHECK((variant_index_v<unsigned&, double, unsigned, int>) == 1);
};

template <typename T, typename U>
bool is_same_v = std::is_same<T, U>::value;

TEST_CASE("testing CaptureObject")
{
    struct DummyT {};

    using ct1 = decltype(CaptureObject<DummyT, int>{0}.get());
    CHECK((is_same_v<ct1, int>));

    using ct2 = decltype(CaptureObject<DummyT, int&>{0}.get());
    CHECK((is_same_v<ct2, int&>));

    using ct3 = decltype(CaptureObject<DummyT, int&&>{0}.get());
    CHECK((is_same_v<ct3, int&&>));

    using ct4 = decltype(CaptureObject<DummyT, const int>{0}.get());
    CHECK((is_same_v<ct4, int>)); // that's ok

    using ct5 = decltype(CaptureObject<DummyT, const int&>{0}.get());
    CHECK((is_same_v<ct5, const int&>));

    using ct6 = decltype(CaptureObject<DummyT, const int&&>{0}.get());
    CHECK((is_same_v<ct6, const int&&>));

    using ct7 = decltype(CaptureObject<DummyT, int*>{nullptr}.get());
    CHECK((is_same_v<ct7, int*>));

    using ct8 = decltype(CaptureObject<DummyT, const int*>{nullptr}.get());
    CHECK((is_same_v<ct8, const int*>));

    using ct9 = decltype(CaptureObject<DummyT, int const*>{nullptr}.get());
    CHECK((is_same_v<ct9, const int*>));
}

TEST_CASE("testing variant_alternative")
{
    CHECK((is_same_v<variant_alternative_t<0, double>, double>));
    CHECK((is_same_v<variant_alternative_t<0, double, int>, double>));
    CHECK((is_same_v<variant_alternative_t<1, double, int>, int>));
    CHECK((is_same_v<variant_alternative_t<0, double, int, unsigned>, double>));
    CHECK((is_same_v<variant_alternative_t<1, double, int, unsigned>, int>));
    CHECK((is_same_v<variant_alternative_t<2, double, int, unsigned>, unsigned>));
}

struct CheckDestructorCall
{
    CheckDestructorCall()
    {
        constructor_called = true;
    }

    ~CheckDestructorCall()
    {
        destructor_called = true;
    }

    static bool constructor_called;
    static bool destructor_called;
};

bool CheckDestructorCall::constructor_called = false;
bool CheckDestructorCall::destructor_called = false;

TEST_CASE("testing variant copy constructor")
{
    variant<int, double> as{3.3};
    auto at = as;
    CHECK(at.index() == 1u);
    CHECK(get<1u>(at) == 3.3);

    variant<TestClass, int, double> bs{TestClass(31)};
    auto bt = bs;
    CHECK(bt.index() == 0u);
    CHECK(get<0u>(bt).a == 31);
    CHECK(get<0u>(bt).constructorid == CID::LVALUE_COPY);

    // check that destructor of default initialized value is called
    CheckDestructorCall::constructor_called = false;
    CheckDestructorCall::destructor_called = false;
    variant<CheckDestructorCall, int> cs{32};
    CHECK(!CheckDestructorCall::constructor_called);
    CHECK(!CheckDestructorCall::destructor_called);
    auto ct = cs;
    CHECK(ct.index() == 1u);
    CHECK(get<1u>(ct) == 32);
    CHECK(CheckDestructorCall::constructor_called);
    CHECK(CheckDestructorCall::destructor_called);
}


