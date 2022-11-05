#pragma once
#include "helpers.h"
#include "dlb_types.h"
#include <cmath>
#include <cstdio>

struct Clock {
    //uint64_t tick;
    //double frameDt;
    bool   server       {};  // true if server thread, false if client thread
    double now          {};  // local game clock, not the same as glfwGetTime(), can be paused
    double nowPrev      {};  // previous now time
    double accum        {};  // time accumulator, for fixed-step updates (only server-side atm)

    double serverNow    {};  // approximate time on server, sync'd each time we receive a snapshot
    double timeOfDay    {};  // 0 = start of day, 1 = end of day
    double daylightPerc {};  // how bright the daylight currently is

    // NOTE: Do not hold returned pointer, it's a shared temp buffer
    const char *timeOfDayStr() {
        thread_local static char buf[16]{};
        const double hours = 1.0 + timeOfDay * 24.0;
        const double mins = fmod(hours, 1.0) * 60.0;
        const int hh = (int)fmod(hours, 12.0);
        const int mm = (int)mins;
        int len = snprintf(buf, sizeof(buf), "%02d:%02d %s", hh, mm, hours < 12 ? "am" : "pm");
        if (len > 10) {
            DLB_ASSERT(len);
        }
        return buf;
    }

    double update(double glfwNow) {
        // Limit delta time to prevent spiraling for after long hitches (e.g. hitting a breakpoint)
        double frameDt = MIN(glfwNow - nowPrev, CL_FRAME_DT_MAX);
        now += frameDt;
        nowPrev = glfwNow;

        // Daily clock
#if CL_DAY_NIGHT_CYCLE
        const double dayClockNow = server ? now : (serverNow ? serverNow : now);
#else
        const double dayClockNow = 0;
#endif
        const double dayClockScale = (1.0 / SV_TIME_SECONDS_IN_DAY);
        const double dayClockStartTime = SV_TIME_WHEN_GAME_STARTS;
        const double dayClockScaled = (dayClockStartTime + dayClockNow) * dayClockScale;
        timeOfDay = fmod(fmod(dayClockScaled, 1.0), 1.0);

        // Daylight
        const double midnightLightPerc = 0.2;
        daylightPerc = 1.0 + (0.5 * (1.0 - midnightLightPerc)) * (sin(2 * (double)PI * dayClockScaled - 0.5 * (double)PI) - 1.0);

        return frameDt;
    }
};

thread_local static Clock g_clock{};
