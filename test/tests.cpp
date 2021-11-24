#include "tests.h"

void maths_test();
void dlb_rand_test();
void bit_stream_test();
void net_message_test();

void run_tests()
{
    maths_test();
    dlb_rand_test();
    bit_stream_test();
    net_message_test();
}

#include "maths_test.cpp"
#include "bitstream_test.cpp"
#include "net_message_test.cpp"