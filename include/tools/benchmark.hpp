// benchmark.hpp
#pragma once

#include <chrono>
#include <string>
#include <fstream>
#include <vector>

namespace vst
{

    class Timer
    {
    public:
        void start();
        void stop();
        double elapsedMilliseconds() const;

    private:
        std::chrono::high_resolution_clock::time_point startTime;
        std::chrono::high_resolution_clock::time_point endTime;
    };

    class BenchmarkLogger
    {
    public:
        explicit BenchmarkLogger(const std::string &filename);
        void log(const std::string &label, double milliseconds);
        void flush();

    private:
        std::ofstream file;
        std::vector<std::string> entries;
    };

} // namespace vst
