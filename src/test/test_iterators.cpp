/*
    Copyright (c) 2017-2018 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.




*/

#include "tbb/tbb_config.h"

#if __TBB_CPP11_PRESENT && __TBB_CPP11_DECLTYPE_PRESENT

#include "tbb/iterators.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <numeric>

#include "harness.h"

//common checks of a random access iterator functionality
template <typename RandomIt>
void test_random_iterator(const RandomIt& it) {
    ASSERT(  it == it,      "== returned false negative");
    ASSERT(!(it == it + 1), "== returned false positive");
    ASSERT(  it != it + 1,  "!= returned false negative");
    ASSERT(!(it != it),     "!= returned false positive");

    ASSERT(*it == *it, "wrong result with operator*");

    RandomIt it1 = it;
    ASSERT(it1 == it, "iterator is not copy constructible");
    RandomIt it2 = RandomIt(it);
    ASSERT(it2 == it, "iterator is not move constructible");

    ++it1;
    ASSERT(it1 == it + 1, "wrong result with prefix operator++");

    using std::swap;
    swap(it1, it2);
    ASSERT((it1 == it) && (it2 == it + 1), "iterator is not swappable");

    it2 = it;
    ASSERT(it2 == it, "iterator is not copy assignable");

    ++it2;
    it2 = RandomIt(it);
    ASSERT(it2 == it, "iterator is not move assignable");

    it1 = it;
    ASSERT((it1++ == it) && (it1 == it + 1), "wrong result with postfix operator++");

    it1 = it + 1;
    ASSERT(--it1 == it, "wrong result with prefix operator--");

    it1 = it + 1;
    ASSERT((it1-- == it + 1) && (it1 == it), "wrong result with postfix operator--");

    it1 += 1;
    ASSERT(it1 == it + 1, "wrong result with operator+=");

    it1 -= 1;
    ASSERT(it1 == it, "wrong result with operator-=");

    ASSERT(1 + it == it + 1, "n + iterator != iterator + n");

    ASSERT((it + 1) - 1 == it, "wrong result with operator-(difference_type)");

    ASSERT((it + 1) - it == 1, "wrong result with iterator subtraction");

    ASSERT(it[1] == *(it + 1), "wrong result with operator[]");

    ASSERT(it < it + 1, "operator< returned false negative");
    ASSERT(!(it < it),  "operator< returned false positive");

    ASSERT(it + 1 > it, "operator> returned false negative");
    ASSERT(!(it > it),  "operator> returned false positive");

    ASSERT(it <= it + 1,    "operator<= returned false negative");
    ASSERT(it <= it,        "operator<= returned false negative");
    ASSERT(!(it + 1 <= it), "operator<= returned false positive");

    ASSERT(1 + it >= it,    "operator>= returned false negative");
    ASSERT(    it >= it,    "operator>= returned false negative");
    ASSERT(!(it >= it + 1), "operator>= returned false positive");
}

struct test_counting_iterator {
    template <typename T, typename IntType>
    void operator()( std::vector<T>& in, IntType begin, IntType end, const T& value) {
        ASSERT((0 <= begin) && (begin <= end) && (end <= IntType(in.size())),
        "incorrect test_counting_iterator 'begin' and/or 'end' argument values");

        auto b = tbb::counting_iterator<IntType>(begin);
        auto e = tbb::counting_iterator<IntType>(end);

        //checks in using
        std::for_each(b, e, [&in, &value](IntType i) { in[i] = value; });

        auto res = std::all_of(in.begin(), in.begin() + begin, [&value](const T& a) {return a!=value;});
        ASSERT(res, "wrong result with counting_iterator in vector's begin portion");

        res = std::all_of(in.begin() + begin, in.begin() + end, [&value](const T& a) {return a==value;});
        ASSERT(res, "wrong result with counting_iterator in vector's main portion");

        res = std::all_of(in.begin() + end, in.end(), [&value](const T& a) {return a!=value;});
        ASSERT(res, "wrong result with counting_iterator in vector's end portion");

        //explicit checks of the counting iterator specific
        ASSERT(b[0]==begin, "wrong result with operator[] for an iterator");
        ASSERT(*(b + 1) == begin+1, "wrong result with operator+ for an iterator");
        ASSERT(*(b+=1) == begin+1, "wrong result with operator+= for an iterator");
    }
};

struct test_zip_iterator {
    template <typename T1, typename T2>
    void operator()(std::vector<T1>& in1, std::vector<T2>& in2) {
        auto b = tbb::make_zip_iterator(in1.begin(), in2.begin());
        auto e = tbb::make_zip_iterator(in1.end(), in2.end());

        //checks in using
        std::for_each(b, e, [](const std::tuple<T1&, T2&>& a) { std::get<0>(a) = 1, std::get<1>(a) = 1;});
        auto res = std::all_of(b, e, [](const std::tuple<T1&, T2&>& a) {return std::get<0>(a) == 1 && std::get<1>(a) == 1;});
        ASSERT(res, "wrong result with zip_iterator iterator");

        test_random_iterator(b);
    }
};

template <typename VecIt1, typename VecIt2>
void test_transform_effect(VecIt1 first1, VecIt1 last1, VecIt2 first2) {
    auto triple = [](typename std::iterator_traits<VecIt1>::value_type const& val) {
        return typename std::iterator_traits<VecIt2>::value_type (3 * val);
    };

    std::copy(
        tbb::make_transform_iterator(first1, triple),
        tbb::make_transform_iterator(last1,  triple),
        first2
    );

    for (typename std::iterator_traits<VecIt1>::difference_type i = 0; i < last1 - first1; ++i)
        if ( first2[i] != (typename std::iterator_traits<VecIt2>::value_type) triple(first1[i]) ) {
            std::cout << "wrong effect with transform iterator" << std::endl;
            exit(1);
        }
}

struct test_transform_iterator {
    template <typename T1, typename T2>
    void operator()(std::vector<T1>& in1, std::vector<T2>& in2) {
        std::iota(in1.begin(), in1.end(), T1(0));

        test_transform_effect(in1.begin(),  in1.end(),  in2.begin());
        test_transform_effect(in1.cbegin(), in1.cend(), in2.begin());

        auto new_transform_iterator = tbb::make_transform_iterator(in2.begin(), [](T2& x) { return x + 1; });
        test_random_iterator(new_transform_iterator);
    }
};

template <typename T, typename IntType>
void test_iterator_by_type(IntType n) {

    const IntType beg = 0;
    const IntType end = n;

    std::vector<T> in(n, T(0));
    std::vector<IntType> in2(n, IntType(0));

    test_counting_iterator()(in, beg,     end,     /*value*/ T(-1));
    test_counting_iterator()(in, beg+123, end-321, /*value*/ T(42));
    test_random_iterator(tbb::counting_iterator<IntType>(beg));

    test_zip_iterator()(in, in2);
    test_transform_iterator()(in, in2);
}

int TestMain() {

    const auto n1 = 1000;
    const auto n2 = 100000;

    test_iterator_by_type<int16_t, int16_t>(n1);
    test_iterator_by_type<int16_t, int64_t>(n2);

    test_iterator_by_type<double, int16_t>(n1);
    test_iterator_by_type<double, int64_t>(n2);

    return Harness::Done;
}

#else

#include "harness.h"

int TestMain () {
    return Harness::Skipped;
}

#endif /* __TBB_CPP11_PRESENT && __TBB_CPP11_DECLTYPE_PRESENT */
