// Stub implementation for PerfettoProfiler
#include "profiling/PerfettoProfiler.hpp"
#include <mutex>

namespace DarkAges::Profiling {

PerfettoProfiler& PerfettoProfiler::instance() {
    static PerfettoProfiler inst;
    return inst;
}

bool PerfettoProfiler::initialize(const std::string&) { 
    active_ = true;
    return true; 
}

void PerfettoProfiler::shutdown() { 
    active_ = false; 
}

void PerfettoProfiler::clearCounters() {
    std::lock_guard<std::mutex> lock(counterMutex_);
    counters_.clear();
}

const PerformanceCounter* PerfettoProfiler::getCounter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(counterMutex_);
    auto it = counters_.find(name);
    if (it != counters_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> PerfettoProfiler::getCounterNames() const {
    std::lock_guard<std::mutex> lock(counterMutex_);
    std::vector<std::string> names;
    for (const auto& [name, _] : counters_) {
        names.push_back(name);
    }
    return names;
}

void PerfettoProfiler::resetCounters() {
    std::lock_guard<std::mutex> lock(counterMutex_);
    for (auto& [name, counter] : counters_) {
        counter.reset();
    }
}

void PerfettoProfiler::setActive(bool active) { active_ = active; }
void PerfettoProfiler::beginEvent(const char*, TraceCategory) {}
void PerfettoProfiler::endEvent(const char*, TraceCategory) {}
void PerfettoProfiler::instantEvent(const char*, TraceCategory) {}
void PerfettoProfiler::counterEvent(const char* name, int64_t value) {
    std::lock_guard<std::mutex> lock(counterMutex_);
    auto& counter = counters_[name];
    counter.name = name;
    counter.count++;
    auto uv = static_cast<uint64_t>(value >= 0 ? value : 0);
    counter.totalTimeUs += uv;
    if (counter.count == 1 || uv > counter.maxTimeUs) counter.maxTimeUs = uv;
    if (counter.count == 1 || uv < counter.minTimeUs) counter.minTimeUs = uv;
}
ScopedTraceEvent PerfettoProfiler::scopedEvent(const char* name, TraceCategory category) { return ScopedTraceEvent(name, category); }

// ScopedTraceEvent methods
ScopedTraceEvent::ScopedTraceEvent(const char* name, TraceCategory category) 
    : name_(name), category_(category) {}
ScopedTraceEvent::~ScopedTraceEvent() {}
ScopedTraceEvent::ScopedTraceEvent(ScopedTraceEvent&& other) noexcept
    : name_(other.name_), category_(other.category_) { other.name_ = nullptr; }
ScopedTraceEvent& ScopedTraceEvent::operator=(ScopedTraceEvent&& other) noexcept {
    name_ = other.name_; category_ = other.category_; other.name_ = nullptr; return *this;
}

} // namespace DarkAges::Profiling
