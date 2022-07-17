#pragma once

struct Clock {
    //uint64_t tick;
    //double frameDt;
    bool   server       {};  // true if server thread, false if client thread
    double now          {};  // game clock, not the same as glfwGetTime(), can be paused
    //double lastTickedAt {};  // used to check if next tick is due
};

extern thread_local Clock g_clock;