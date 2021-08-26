#include "tests.h"
#define DLB_RAND_TEST
#include "dlb_rand.h"

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