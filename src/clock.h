#pragma once

struct Clock {
    //uint64_t tick;
    //double frameDt;
    bool server;
    double now;     // game clock, not the same as glfwGetTime(), can be paused
};

extern thread_local Clock g_clock;