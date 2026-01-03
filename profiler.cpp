#include "profiler.h"


    Profiler::Profiler() {
        last_ = std::chrono::steady_clock::now();
    }

    Profiler::~Profiler() {
        print();
    }

    // Record a named tick and reset timer
    void Profiler::tick(const std::string& name) {
        auto now = std::chrono::steady_clock::now();
        double ms = elapsed_ms(last_, now);
        entries_.push_back({name, ms});
        last_ = now;
    }

    // Reset timer without recording anything
    void Profiler::tock() {
        last_ = std::chrono::steady_clock::now();
    }

    double Profiler::elapsed_ms(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b) {
        return std::chrono::duration<double, std::milli>(b - a).count();
    }

 void Profiler::print() const {
    if (entries_.empty()) return;
    double total = 0.0;
    for (const auto& e : entries_) total += e.ms;
    std::cout << "\n=== Profiling Results ===\n";
    std::cout << std::left << std::setw(30) << "Tick"
    << std::right << std::setw(12) << "Time (ms)"
    << std::right << std::setw(10) << "%"
    << "\n";
    std::cout << std::string(52, '-') << "\n";
    for (const auto& e : entries_) {
        double pct = (e.ms / total) * 100.0;
        std::cout << std::left << std::setw(30) << e.name
        << std::right << std::setw(12) << std::fixed << std::setprecision(3)
        << e.ms << std::right << std::setw(10) << std::setprecision(1) << pct
        << "\n"; } std::cout << std::string(52, '-') << "\n";
        // Total row
        std::cout << std::left << std::setw(30) << "Total"
        << std::right << std::setw(12) << std::fixed << std::setprecision(3)
        << total << std::right << std::setw(10) << "100.0"
        << "\n\n";
    }
