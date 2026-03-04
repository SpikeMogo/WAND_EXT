// StatTracker.h — Rolling rate tracker for EXP% and Mesos
// Ticked every frame from the ImGui loop.
// Bins deltas at 1-second and 1-minute intervals into rolling windows.
#pragma once

#include <cstdint>
#include <array>
#include <chrono>

// =====================================================================
// RollingWindow — fixed-size circular buffer that tracks sum of bins
// =====================================================================
template<int N>
class RollingWindow
{
public:
    void Push(double value)
    {
        sum_ -= bins_[head_];
        bins_[head_] = value;
        sum_ += value;
        head_ = (head_ + 1) % N;
        if (count_ < N) ++count_;
    }

    void Reset()
    {
        bins_.fill(0.0);
        sum_ = 0.0;
        head_ = 0;
        count_ = 0;
    }

    double Sum()    const { return sum_; }
    int    Count()  const { return count_; }

private:
    std::array<double, N> bins_{};
    double sum_   = 0.0;
    int    head_  = 0;
    int    count_ = 0;
};


// =====================================================================
// StatTracker — tracks EXP% and Mesos rate of change
//
// Window layout:
//   - perMinute:  60 bins × 1 second  → covers 1 minute
//   - perHour:    60 bins × 1 minute  → covers 1 hour
//
// Rates are rolling sums of the filled bins.
// Negative EXP deltas (level-up) are clamped to 0.
// =====================================================================
class StatTracker
{
public:
    struct Rates
    {
        double expPerMin  = 0.0;   // EXP% rate per minute
        double expPerHour = 0.0;   // EXP% rate per hour
        int64_t mesosPerMin  = 0;  // Mesos rate per minute
        int64_t mesosPerHour = 0;  // Mesos rate per hour
    };

    // Call every frame with current values.
    bool Tick(double expPer, int32_t mesos)
    {
        auto now = std::chrono::steady_clock::now();

        if (!initialized_)
        {
            prevExpPer_ = expPer;
            prevMesos_  = mesos;
            lastSecFlush_  = now;
            lastMinFlush_  = now;
            initialized_ = true;
            return false;
        }

        // Compute deltas since last tick
        double  dExp   = expPer - prevExpPer_;
        int64_t dMesos = static_cast<int64_t>(mesos) - static_cast<int64_t>(prevMesos_);

        prevExpPer_ = expPer;
        prevMesos_  = mesos;

        // Clamp negative EXP delta to 0 (level-up: 99% -> 0%)
        if (dExp < 0.0) dExp = 0.0;

        secAccumExp_   += dExp;
        secAccumMesos_ += dMesos;
        minAccumExp_   += dExp;
        minAccumMesos_ += dMesos;

        // Every 1 second → push into minute-window
        auto msElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSecFlush_).count();
        if (msElapsed >= 1000)
        {
            expMinWin_.Push(secAccumExp_);
            mesosMinWin_.Push(static_cast<double>(secAccumMesos_));

            secAccumExp_   = 0.0;
            secAccumMesos_ = 0;
            lastSecFlush_  = now;
        }

        // Every 1 minute → push into hour-window
        auto secElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastMinFlush_).count();
        if (secElapsed >= 60)
        {
            expHrWin_.Push(minAccumExp_);
            mesosHrWin_.Push(static_cast<double>(minAccumMesos_));

            minAccumExp_   = 0.0;
            minAccumMesos_ = 0;
            lastMinFlush_  = now;
        }

        // Rate = average per bin × window size (60)
        // This gives correct rates even with partially filled windows
        int sc = expMinWin_.Count();
        if (sc > 0) {
            rates_.expPerMin   = expMinWin_.Sum() / sc * 60;
            rates_.mesosPerMin = static_cast<int64_t>(mesosMinWin_.Sum() / sc * 60);
        }
        int mc = expHrWin_.Count();
        if (mc > 0) {
            rates_.expPerHour   = expHrWin_.Sum() / mc * 60;
            rates_.mesosPerHour = static_cast<int64_t>(mesosHrWin_.Sum() / mc * 60);
        }

        return true;
    }

    const Rates& GetRates() const { return rates_; }

    void Reset()
    {
        expMinWin_.Reset();
        mesosMinWin_.Reset();
        expHrWin_.Reset();
        mesosHrWin_.Reset();
        rates_ = {};
        initialized_ = false;
        secAccumExp_ = 0.0;
        secAccumMesos_ = 0;
        minAccumExp_ = 0.0;
        minAccumMesos_ = 0;
    }

    bool IsActive() const { return initialized_; }

private:
    bool initialized_ = false;

    double  prevExpPer_ = 0.0;
    int32_t prevMesos_  = 0;

    // Per-second accumulator
    double  secAccumExp_   = 0.0;
    int64_t secAccumMesos_ = 0;

    // Per-minute accumulator
    double  minAccumExp_   = 0.0;
    int64_t minAccumMesos_ = 0;

    std::chrono::steady_clock::time_point lastSecFlush_;
    std::chrono::steady_clock::time_point lastMinFlush_;

    // 60 × 1-second bins → 1-minute window
    RollingWindow<60> expMinWin_;
    RollingWindow<60> mesosMinWin_;

    // 60 × 1-minute bins → 1-hour window
    RollingWindow<60> expHrWin_;
    RollingWindow<60> mesosHrWin_;

    Rates rates_;
};
