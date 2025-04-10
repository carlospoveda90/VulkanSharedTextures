// benchmark.cpp
#include "tools/benchmark.hpp"
#include <iomanip>
#include <sstream>

namespace vst
{

    void Timer::start()
    {
        startTime = std::chrono::high_resolution_clock::now();
    }

    void Timer::stop()
    {
        endTime = std::chrono::high_resolution_clock::now();
    }

    double Timer::elapsedMilliseconds() const
    {
        return std::chrono::duration<double, std::milli>(endTime - startTime).count();
    }

    BenchmarkLogger::BenchmarkLogger(const std::string &filename)
    {
        file.open(filename, std::ios::out);
        if (file.is_open())
        {
            file << "Label,Time(ms)\n";
        }
    }

    void BenchmarkLogger::log(const std::string &label, double milliseconds)
    {
        std::ostringstream entry;
        entry << label << "," << std::fixed << std::setprecision(3) << milliseconds;
        entries.push_back(entry.str());
    }

    void BenchmarkLogger::flush()
    {
        if (!file.is_open())
            return;
        for (const auto &entry : entries)
        {
            file << entry << "\n";
        }
        file.flush();
        entries.clear();
    }

} // namespace vst
