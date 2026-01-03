#pragma once
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

class Profiler{
    public:
        Profiler();
        ~Profiler();

        // Record a named tick and reset timer
        void tick(const std::string& name);

        // Reset timer without recording anything
        void tock();
    private:

        struct Entry {
            std::string name;
            double ms;
        };

        std::vector<Entry> entries_;
        std::chrono::steady_clock::time_point last_;

        static double elapsed_ms(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b);

        void print() const;
    
};