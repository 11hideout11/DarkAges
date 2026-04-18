#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "memory/PooledAllocator.hpp"
#include <vector>
#include <list>
#include <string>
#include <memory>

using namespace DarkAges;
using namespace DarkAges::Memory;

// Test pool config for unit tests (small to verify behavior quickly)
using TestPooledAllocator = PooledAllocator<int, 64, 100>;

// Global destroy counter for destructor tracking
static int gDestroyCount = 0;

struct TrackedObject {
    int value;
    ~TrackedObject() { gDestroyCount++; }
    TrackedObject(int v = 0) : value(v) {}
};

// ============================================================================
// Basic Allocation Tests
// ============================================================================

TEST_CASE("[pooled-allocator] allocate single element", "[memory]") {
    TestPooledAllocator alloc;
    
    int* ptr = alloc.allocate(1);
    REQUIRE(ptr != nullptr);
    
    // Verify we can write to it
    *ptr = 42;
    REQUIRE(*ptr == 42);
    
    alloc.deallocate(ptr, 1);
}

TEST_CASE("[pooled-allocator] allocate multiple elements within block", "[memory]") {
    TestPooledAllocator alloc;
    
    // 64 bytes / 4 bytes per int = 16 ints max in 64-byte block
    int* ptr = alloc.allocate(10);
    REQUIRE(ptr != nullptr);
    
    // Write values
    for (int i = 0; i < 10; ++i) {
        ptr[i] = i;
    }
    
    for (int i = 0; i < 10; ++i) {
        REQUIRE(ptr[i] == i);
    }
    
    alloc.deallocate(ptr, 10);
}

TEST_CASE("[pooled-allocator] allocate exceeds block size falls back to heap", "[memory]") {
    // This allocator has 64-byte blocks, so requesting larger memory exceeds block size
    TestPooledAllocator alloc;
    
    // 20 ints = 80 bytes, exceeds 64-byte block
    int* ptr = alloc.allocate(20);
    REQUIRE(ptr != nullptr);
    
    ptr[0] = 123;
    REQUIRE(ptr[0] == 123);
    
    alloc.deallocate(ptr, 20);
}

// ============================================================================
// Construct and Destroy Tests
// ============================================================================

struct TestObject {
    int value;
    double dvalue;
    
    TestObject() : value(0), dvalue(0.0) {}
    TestObject(int v, double d) : value(v), dvalue(d) {}
    ~TestObject() = default;
};

TEST_CASE("[pooled-allocator] construct object", "[memory]") {
    using AllocType = PooledAllocator<TestObject, 64, 100>;
    AllocType alloc;
    
    TestObject* obj = alloc.allocate(1);
    alloc.construct(obj, 42, 3.14);
    
    REQUIRE(obj->value == 42);
    REQUIRE(obj->dvalue == Catch::Approx(3.14).margin(0.001));
    
    alloc.destroy(obj);
    alloc.deallocate(obj, 1);
}

TEST_CASE("[pooled-allocator] destroy object calls destructor", "[memory]") {
    gDestroyCount = 0;
    
    using TrackedAlloc = PooledAllocator<TrackedObject, 64, 100>;
    TrackedAlloc alloc;
    TrackedObject* obj = alloc.allocate(1);
    alloc.construct(obj, 42);
    
    alloc.destroy(obj);
    alloc.deallocate(obj, 1);
    
    REQUIRE(gDestroyCount == 1);
}

// ============================================================================
// Pool Stats Tests
// ============================================================================

TEST_CASE("[pooled-allocator] pool stats are accessible", "[memory]") {
    // These are static methods - should be callable
    auto total = TestPooledAllocator::getPoolTotalBlocks();
    auto free = TestPooledAllocator::getPoolFreeBlocks();
    auto used = TestPooledAllocator::getPoolUsedBlocks();
    
    REQUIRE(total > 0);
    REQUIRE(free >= 0);
    REQUIRE(used >= 0);
}

TEST_CASE("[pooled-allocator] pool stats reflect allocation", "[memory]") {
    using StatsAlloc = PooledAllocator<int, 64, 10>;
    
    auto beforeUsed = StatsAlloc::getPoolUsedBlocks();
    auto beforeFree = StatsAlloc::getPoolFreeBlocks();
    auto total = StatsAlloc::getPoolTotalBlocks();
    
    REQUIRE(total == 10);
    
    // Create allocator to track allocation
    StatsAlloc alloc;
    int* ptr1 = alloc.allocate(1);
    int* ptr2 = alloc.allocate(1);
    
    REQUIRE(StatsAlloc::getPoolUsedBlocks() == beforeUsed + 2);
    REQUIRE(StatsAlloc::getPoolFreeBlocks() == beforeFree - 2);
    
    alloc.deallocate(ptr1, 1);
    alloc.deallocate(ptr2, 1);
}

// ============================================================================
// Rebind Tests
// ============================================================================

TEST_CASE("[pooled-allocator] rebind to different type", "[memory]") {
    // Original allocator for int
    PooledAllocator<int, 64, 100> intAlloc;
    
    // Rebind to double
    using DoubleAlloc = PooledAllocator<int, 64, 100>::rebind<double>::other;
    DoubleAlloc doubleAlloc(intAlloc);  // Construct from int allocator
    
    // Allocate double - should work
    double* dptr = doubleAlloc.allocate(1);
    REQUIRE(dptr != nullptr);
    
    *dptr = 2.718;
    REQUIRE(*dptr == Catch::Approx(2.718).margin(0.001));
    
    doubleAlloc.deallocate(dptr, 1);
}

// ============================================================================
// Comparison Operators Tests
// ============================================================================

TEST_CASE("[pooled-allocator] equality operator returns true", "[memory]") {
    PooledAllocator<int, 64, 100> alloc1;
    PooledAllocator<int, 64, 100> alloc2;
    
    REQUIRE(alloc1 == alloc2);
}

TEST_CASE("[pooled-allocator] inequality operator returns false", "[memory]") {
    PooledAllocator<int, 64, 100> alloc1;
    PooledAllocator<int, 64, 100> alloc2;
    
    REQUIRE_FALSE(alloc1 != alloc2);
}

TEST_CASE("[pooled-allocator] different types equal", "[memory]") {
    PooledAllocator<int, 64, 100> intAlloc;
    PooledAllocator<double, 64, 100> dblAlloc;
    
    // Pooled allocators of same BlockSize/BlockCount are always equal
    REQUIRE(intAlloc == dblAlloc);
}

// ============================================================================
// Exception Tests
// ============================================================================

TEST_CASE("[pooled-allocator] allocate zero returns valid", "[memory]") {
    TestPooledAllocator alloc;
    
    // allocate(0) should return valid but not consume pool
    int* ptr = alloc.allocate(0);
    REQUIRE(ptr != nullptr);
    
    alloc.deallocate(ptr, 0);
}

TEST_CASE("[pooled-allocator] deallocate null is safe", "[memory]") {
    TestPooledAllocator alloc;
    
    // Should not throw
    alloc.deallocate(nullptr, 1);
}

// ============================================================================
// STL Container Integration Tests
// ============================================================================

TEST_CASE("[pooled-allocator] vector with pooled allocator", "[memory]") {
    using Vec = std::vector<int, TestPooledAllocator>;
    Vec vec(TestPooledAllocator{});
    
    // Push some values
    for (int i = 0; i < 20; ++i) {
        vec.push_back(i);
    }
    
    REQUIRE(vec.size() == 20);
    REQUIRE(vec[10] == 10);
}

TEST_CASE("[pooled-allocator] list with pooled allocator", "[memory]") {
    using List = std::list<int, TestPooledAllocator>;
    List list(TestPooledAllocator{});
    
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    
    REQUIRE(list.size() == 3);
    
    auto it = list.begin();
    REQUIRE(*it == 1);
    ++it;
    REQUIRE(*it == 2);
}

TEST_CASE("[pooled-allocator] vector preserves values after reallocation", "[memory]") {
    using Vec = std::vector<int, TestPooledAllocator>;
    Vec vec(TestPooledAllocator{});
    
    // Add enough to force reallocation
    for (int i = 0; i < 100; ++i) {
        vec.push_back(i * 2);
    }
    
    REQUIRE(vec.size() == 100);
    REQUIRE(vec[99] == 198);
    
    // Verify all values
    for (int i = 0; i < 100; ++i) {
        REQUIRE(vec[i] == i * 2);
    }
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST_CASE("[pooled-allocator] move constructor", "[memory]") {
    TestPooledAllocator alloc1;
    
    // Move construct
    TestPooledAllocator alloc2(std::move(alloc1));
    
    // Both should be usable (pool is static)
    int* ptr = alloc2.allocate(1);
    REQUIRE(ptr != nullptr);
    alloc2.deallocate(ptr, 1);
}

TEST_CASE("[pooled-allocator] move assignment", "[memory]") {
    TestPooledAllocator alloc1;
    TestPooledAllocator alloc2;
    
    alloc2 = std::move(alloc1);
    
    // Should still work
    int* ptr = alloc1.allocate(1);
    REQUIRE(ptr != nullptr);
    alloc1.deallocate(ptr, 1);
}

// ============================================================================
// Convenience Alias Tests
// ============================================================================

TEST_CASE("[pooled-allocator] convenience aliases exist", "[memory]") {
    // These should compile
    SmallPooledAllocator smallAlloc;
    MediumPooledAllocator mediumAlloc;
    LargePooledAllocator largeAlloc;
    
    // Verify they work
    char* small = smallAlloc.allocate(1);
    REQUIRE(small != nullptr);
    smallAlloc.deallocate(small, 1);
    
    char* medium = mediumAlloc.allocate(1);
    REQUIRE(medium != nullptr);
    mediumAlloc.deallocate(medium, 1);
    
    char* large = largeAlloc.allocate(1);
    REQUIRE(large != nullptr);
    largeAlloc.deallocate(large, 1);
}

// ============================================================================
// Large Allocation Fallback Tests  
// ============================================================================

TEST_CASE("[pooled-allocator] large allocation uses heap not pool", "[memory]") {
    // PooledAllocator with 64-byte blocks
    using Pool = PooledAllocator<long long, 64, 5>;
    
    // Each long long is 8 bytes, so 10 = 80 bytes > 64-byte block
    // This should allocate from heap, not pool
    
    Pool alloc;
    auto usedBefore = Pool::getPoolUsedBlocks();
    
    long long* ptr = alloc.allocate(10);
    REQUIRE(ptr != nullptr);
    
    auto usedAfter = Pool::getPoolUsedBlocks();
    
    // Pool should NOT have changed - we used heap fallback
    REQUIRE(usedBefore == usedAfter);
    
    alloc.deallocate(ptr, 10);
}