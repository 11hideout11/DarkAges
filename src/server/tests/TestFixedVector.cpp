#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "memory/FixedVector.hpp"
#include <string>

using namespace DarkAges::Memory;

TEST_CASE("FixedVector basic operations", "[memory][FixedVector]") {
    FixedVector<int, 10> vec;

    REQUIRE(vec.empty());
    REQUIRE(vec.size() == 0);
    REQUIRE(vec.capacity() == 10);

    SECTION("push_back increases size") {
        vec.push_back(1);
        REQUIRE(vec.size() == 1);
        REQUIRE(vec[0] == 1);

        vec.push_back(2);
        vec.push_back(3);
        REQUIRE(vec.size() == 3);
    }

    SECTION("front and back") {
        vec.push_back(10);
        vec.push_back(20);
        REQUIRE(vec.front() == 10);
        REQUIRE(vec.back() == 20);
    }

    SECTION("pop_back decreases size") {
        vec.push_back(1);
        vec.push_back(2);
        vec.pop_back();
        REQUIRE(vec.size() == 1);
        REQUIRE(vec.back() == 1);
    }

    SECTION("clear resets size") {
        vec.push_back(1);
        vec.push_back(2);
        vec.clear();
        REQUIRE(vec.empty());
        REQUIRE(vec.size() == 0);
    }

    SECTION("resize") {
        vec.resize(3);
        REQUIRE(vec.size() == 3);
        vec.resize(1);
        REQUIRE(vec.size() == 1);
    }
}

TEST_CASE("FixedVector capacity exceeded", "[memory][FixedVector]") {
    FixedVector<int, 3> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    SECTION("push_back throws when full") {
        REQUIRE_THROWS_AS(vec.push_back(4), std::runtime_error);
    }

    SECTION("emplace_back throws when full") {
        REQUIRE_THROWS_AS(vec.emplace_back(4), std::runtime_error);
    }

    SECTION("resize throws when exceeding capacity") {
        REQUIRE_THROWS_AS(vec.resize(5), std::runtime_error);
    }
}

TEST_CASE("FixedVector emplace_back", "[memory][FixedVector]") {
    FixedVector<std::string, 5> vec;

    vec.emplace_back("hello");
    REQUIRE(vec.size() == 1);
    REQUIRE(vec[0] == "hello");

    vec.emplace_back("world");
    REQUIRE(vec.size() == 2);
    REQUIRE(vec.back() == "world");
}

TEST_CASE("FixedVector find", "[memory][FixedVector]") {
    FixedVector<int, 10> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    SECTION("find existing element") {
        auto it = vec.find(20);
        REQUIRE(it != vec.end());
        REQUIRE(*it == 20);
    }

    SECTION("find non-existing element") {
        auto it = vec.find(99);
        REQUIRE(it == vec.end());
    }

    SECTION("find after erase") {
        vec.erase_value(20);
        auto it = vec.find(20);
        REQUIRE(it == vec.end());
    }
}

TEST_CASE("FixedVector erase", "[memory][FixedVector]") {
    FixedVector<int, 10> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    SECTION("erase by iterator") {
        auto it = vec.begin() + 1;  // points to 2
        vec.erase(it);
        REQUIRE(vec.size() == 3);
        REQUIRE(vec[1] == 3);  // 3 shifts into position 1
    }

    SECTION("erase first element") {
        vec.erase(vec.begin());
        REQUIRE(vec.size() == 3);
        REQUIRE(vec[0] == 2);
    }

    SECTION("erase last element") {
        vec.erase(vec.begin() + 3);
        REQUIRE(vec.size() == 3);
        REQUIRE(vec.back() == 3);
    }

    SECTION("erase invalid position returns end") {
        auto it = vec.end();
        auto result = vec.erase(it);
        REQUIRE(result == vec.end());
    }
}

TEST_CASE("FixedVector erase_value", "[memory][FixedVector]") {
    FixedVector<int, 10> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    SECTION("erase existing value returns true") {
        bool result = vec.erase_value(2);
        REQUIRE(result);
        REQUIRE(vec.size() == 2);
        REQUIRE(vec[1] == 3);
    }

    SECTION("erase non-existing value returns false") {
        bool result = vec.erase_value(99);
        REQUIRE_FALSE(result);
        REQUIRE(vec.size() == 3);
    }
}

TEST_CASE("FixedVector move semantics", "[memory][FixedVector]") {
    FixedVector<std::string, 5> vec1;
    vec1.push_back("hello");
    vec1.push_back("world");

    FixedVector<std::string, 5> vec2 = std::move(vec1);

    REQUIRE(vec2.size() == 2);
    REQUIRE(vec2[0] == "hello");
    REQUIRE(vec2[1] == "world");
}

TEST_CASE("FixedVector iterators", "[memory][FixedVector]") {
    FixedVector<int, 10> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    int sum = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        sum += *it;
    }
    REQUIRE(sum == 6);

    sum = 0;
    for (int val : vec) {
        sum += val;
    }
    REQUIRE(sum == 6);
}

TEST_CASE("FixedVector data pointer", "[memory][FixedVector]") {
    FixedVector<int, 10> vec;
    vec.push_back(42);
    vec.push_back(99);

    int* ptr = vec.data();
    REQUIRE(ptr[0] == 42);
    REQUIRE(ptr[1] == 99);
}

TEST_CASE("FixedVector const correctness", "[memory][FixedVector]") {
    FixedVector<int, 10> vec;
    vec.push_back(1);
    vec.push_back(2);

    const FixedVector<int, 10>& constVec = vec;

    REQUIRE(constVec.size() == 2);
    REQUIRE(constVec[0] == 1);
    REQUIRE(constVec.front() == 1);
    REQUIRE(constVec.back() == 2);
    REQUIRE(constVec.capacity() == 10);
    REQUIRE(constVec.empty() == false);
    REQUIRE(constVec.data() != nullptr);

    for (const auto& val : constVec) {
        (void)val;  // iterate without modification
    }
}

TEST_CASE("FixedVector large type", "[memory][FixedVector]") {
    struct LargeData {
        int a, b, c, d;
    };

    FixedVector<LargeData, 5> vec;
    vec.push_back({1, 2, 3, 4});
    vec.push_back({5, 6, 7, 8});

    REQUIRE(vec.size() == 2);
    REQUIRE(vec[0].a == 1);
    REQUIRE(vec[1].d == 8);
}