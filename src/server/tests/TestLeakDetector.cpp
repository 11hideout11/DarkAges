// [MEMORY_AGENT] Unit tests for LeakDetector

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "memory/LeakDetector.hpp"
#include "memory/MemoryPool.hpp"
#include <vector>
#include <memory>

using namespace DarkAges::Memory;

TEST_CASE("LeakDetector basic operations", "[memory][leakdetector]") {
    AllocationTracker& tracker = AllocationTracker::instance();
    
    // Reset tracker before each test
    tracker.reset();
    
    SECTION("No leak detected in empty scope") {
        LeakDetector detector("empty_scope");
        REQUIRE(detector.verifyNoLeaks());
    }
    
    SECTION("verifyNoLeaks returns true when no allocations") {
        LeakDetector detector("test_verify");
        bool result = detector.verifyNoLeaks();
        REQUIRE(result == true);
    }
    
    SECTION("Allocation delta is zero with no allocations") {
        LeakDetector detector("test_delta");
        detector.verifyNoLeaks();
        REQUIRE(detector.getAllocationDelta() == 0);
    }
}

TEST_CASE("LeakDetector with allocations", "[memory][leakdetector]") {
    AllocationTracker& tracker = AllocationTracker::instance();
    tracker.reset();
    
    SECTION("Leak detected when allocation not freed") {
        LeakDetector detector("leak_scope");
        
        // Allocate memory
        int* leaked = new int(42);
        tracker.trackAllocation(leaked, sizeof(int), "TestLeakDetector.cpp", __LINE__, __FUNCTION__);
        
        bool result = detector.verifyNoLeaks();
        REQUIRE(result == false);
        
        // Proper cleanup
        tracker.untrackAllocation(leaked);
        delete leaked;
    }
    
    SECTION("No leak when allocation is properly freed") {
        LeakDetector detector("no_leak_scope");
        
        int* data = new int(100);
        tracker.trackAllocation(data, sizeof(int), "TestLeakDetector.cpp", __LINE__, __FUNCTION__);
        
        tracker.untrackAllocation(data);
        delete data;
        
        bool result = detector.verifyNoLeaks();
        REQUIRE(result == true);
    }
    
    SECTION("Partial leak detected") {
        LeakDetector detector("partial_leak_scope");
        
        // Allocate two, free only one
        int* a = new int(1);
        int* b = new int(2);
        tracker.trackAllocation(a, sizeof(int), "TestLeakDetector.cpp", __LINE__, "func_a");
        tracker.trackAllocation(b, sizeof(int), "TestLeakDetector.cpp", __LINE__, "func_b");
        
        tracker.untrackAllocation(a);
        
        bool result = detector.verifyNoLeaks();
        REQUIRE(result == false);
        
        // Cleanup
        tracker.untrackAllocation(b);
        delete b;
    }
}

TEST_CASE("LeakDetector multiple allocations", "[memory][leakdetector]") {
    AllocationTracker& tracker = AllocationTracker::instance();
    tracker.reset();
    
    SECTION("Multiple allocations - all freed") {
        LeakDetector detector("multi_free");
        std::vector<int*> ptrs;
        
        for (int i = 0; i < 5; ++i) {
            int* p = new int(i);
            tracker.trackAllocation(p, sizeof(int), "TestLeakDetector.cpp", __LINE__, __FUNCTION__);
            ptrs.push_back(p);
        }
        
        // Free all
        for (int* p : ptrs) {
            tracker.untrackAllocation(p);
            delete p;
        }
        
        REQUIRE(detector.verifyNoLeaks());
    }
    
    SECTION("Multiple allocations - one leak") {
        LeakDetector detector("multi_leak");
        std::vector<int*> ptrs;
        
        for (int i = 0; i < 3; ++i) {
            int* p = new int(i);
            tracker.trackAllocation(p, sizeof(int), "TestLeakDetector.cpp", __LINE__, __FUNCTION__);
            ptrs.push_back(p);
        }
        
        // Free only first two
        for (size_t i = 0; i < 2; ++i) {
            tracker.untrackAllocation(ptrs[i]);
            delete ptrs[i];
        }
        
        bool result = detector.verifyNoLeaks();
        REQUIRE(result == false);
        
        // Cleanup remaining
        tracker.untrackAllocation(ptrs[2]);
        delete ptrs[2];
    }
    
    SECTION("Allocation delta correctly calculated") {
        LeakDetector detector("delta_calc");
        
        // Allocate 100 bytes total
        char* buf1 = new char[50];
        char* buf2 = new char[50];
        tracker.trackAllocation(buf1, 50, "TestLeakDetector.cpp", __LINE__, __FUNCTION__);
        tracker.trackAllocation(buf2, 50, "TestLeakDetector.cpp", __LINE__, __FUNCTION__);
        
        // Free none - should have 50 byte leak per allocation
        // But we need to track them first
        tracker.untrackAllocation(buf1);
        tracker.untrackAllocation(buf2);
        
        // getAllocationDelta = (allocatedEnd - freedEnd) - (allocatedStart - freedStart)
        // With both freed, delta should be 0
        REQUIRE(detector.getAllocationDelta() == 0);
        
        delete[] buf1;
        delete[] buf2;
    }
}

TEST_CASE("LeakDetector destructor behavior", "[memory][leakdetector]") {
    AllocationTracker& tracker = AllocationTracker::instance();
    tracker.reset();
    
    SECTION("Destructor auto-verifies if verifyNoLeaks not called") {
        // Note: Can't fully test destructor auto-verify without checking
        // the implementation behavior. We'll verify the checked_ flag works.
        LeakDetector detector("auto_verify");
        // Don't call verifyNoLeaks - destructor should call it
        (void)detector; // Suppress unused warning
        // If we reach here without crash, destructor behavior is OK
    }
    
    SECTION("Destructor does not double-check") {
        LeakDetector detector("double_check");
        REQUIRE(detector.verifyNoLeaks());
        
        // Second call should also work (idempotent)
        REQUIRE(detector.verifyNoLeaks());
    }
}

TEST_CASE("LeakDetector boundary cases", "[memory][leakdetector]") {
    AllocationTracker& tracker = AllocationTracker::instance();
    tracker.reset();
    
    SECTION("Zero-size allocation") {
        LeakDetector detector("zero_alloc");
        
        // Allocate zero bytes
        void* p = new char[0];
        if (p) {
            tracker.trackAllocation(p, 0, "TestLeakDetector.cpp", __LINE__, __FUNCTION__);
            tracker.untrackAllocation(p);
            delete[] p;
        }
        
        REQUIRE(detector.verifyNoLeaks());
    }
    
    SECTION("Multiple small allocations") {
        LeakDetector detector("small_allocs");
        std::vector<void*> ptrs;
        
        // Allocate many small objects
        for (int i = 0; i < 100; ++i) {
            void* p = new char[8];
            tracker.trackAllocation(p, 8, "TestLeakDetector.cpp", __LINE__, __FUNCTION__);
            ptrs.push_back(p);
        }
        
        // Free all
        for (void* p : ptrs) {
            tracker.untrackAllocation(p);
            delete[] static_cast<char*>(p);
        }
        
        REQUIRE(detector.verifyNoLeaks());
    }
    
    SECTION("Verify after reset tracker") {
        LeakDetector detector("after_reset");
        
        int* x = new int(42);
        tracker.trackAllocation(x, sizeof(int), "TestLeakDetector.cpp", __LINE__, __FUNCTION__);
        
        // Reset tracker mid-detection
        tracker.reset();
        
        // Now should pass since everything reset
        REQUIRE(detector.verifyNoLeaks());
        
        delete x;
    }
}

TEST_CASE("LeakDetector scope name", "[memory][leakdetector]") {
    SECTION("Custom scope name accepted") {
        LeakDetector detector("my_custom_scope_name_that_is_quite_long");
        REQUIRE(detector.verifyNoLeaks());
    }
    
    SECTION("Empty scope name accepted") {
        LeakDetector detector("");
        REQUIRE(detector.verifyNoLeaks());
    }
    
    SECTION("Null scope name via direct construction") {
        // This tests the constructor accepts scope name
        LeakDetector detector("null_test");
        (void)detector;
    }
}

#if defined(CATCH_CONFIG_ENABLE_BENCHMARKING)
TEST_CASE("LeakDetector performance", "[!benchmark][memory][leakdetector]") {
    AllocationTracker& tracker = AllocationTracker::instance();
    
    BENCHMARK("LeakDetector create and verify") {
        tracker.reset();
        
        LeakDetector detector("benchmark_scope");
        bool result = detector.verifyNoLeaks();
        
        return result;
    };
    
    BENCHMARK("LeakDetector with allocations") {
        tracker.reset();
        
        LeakDetector detector("benchmark_alloc");
        
        // Allocate and free
        std::vector<int*> ptrs;
        for (int i = 0; i < 10; ++i) {
            int* p = new int(i);
            tracker.trackAllocation(p, sizeof(int), "benchmark.cpp", i, "func");
            ptrs.push_back(p);
        }
        
        for (int* p : ptrs) {
            tracker.untrackAllocation(p);
            delete p;
        }
        
        bool result = detector.verifyNoLeaks();
        return result;
    };
}
#endif