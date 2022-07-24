#pragma once

struct Clock {
    //uint64_t tick;
    //double frameDt;
    bool   server       {};  // true if server thread, false if client thread
    double now          {};  // game clock, not the same as glfwGetTime(), can be paused
    double nowPrev      {};  // previous now time
    double accum        {};  // time accumulator, for fixed-step updates (only server-side atm)
    double timeOfDay    {};  // 0 = start of day, 1 = end of day
    double daylightPerc {};  // how bright the daylight currently is
};

extern thread_local Clock g_clock;