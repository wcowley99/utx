#include "eval.c"

#include <stdio.h>

#define ASSERT_EQ(x, y)                                                                            \
        do {                                                                                       \
                if (x != y) {                                                                      \
                        printf("[%s:%d]: %s is not equal to %s.\n", __FILE__, __LINE__, #x, #y);   \
                        return -1;                                                                 \
                }                                                                                  \
        } while (0);

int test() {
        printf("Running Tests...\n");

        init_high_cards();

        {
                printf("Testing Card Creation\n");

                ASSERT_EQ(create_card("Kd"), 1ull << 43);
                ASSERT_EQ(create_card("5s"), 1ull << 3);
                ASSERT_EQ(create_card("Jc"), 1ull << 57);
        }

        {
                printf("Testing Bit Twiddling Functions\n");

                ASSERT_EQ(count_bits(0b0110010), 3);
                ASSERT_EQ(count_bits(0xFF), 8);
                ASSERT_EQ(count_bits(0x3FFF), 14);
                ASSERT_EQ(count_bits(0b10101100101001), 7);
                ASSERT_EQ(count_bits(0), 0);

                ASSERT_EQ(straight_rank(0x1F00), 13); // Ace
                ASSERT_EQ(straight_rank(0x0F80), 12); // King
                ASSERT_EQ(straight_rank(0x07C0), 11); // Queen
                ASSERT_EQ(straight_rank(0x100F), 5);  // Wheel
        }

        {
                printf("Testing Flush Evalutaions\n");

                ASSERT_EQ(eval_hand_strings("Ad", "Kd", "Qd", "Jd", "9d", "2s", "3s"), 323);
                ASSERT_EQ(eval_hand_strings("7d", "5d", "4d", "3d", "2d", "2s", "3s"), 1599);
        }

        {
                printf("Testing Evaluation Function\n");

                // Two royal flushes should have equal values
                ASSERT_EQ(eval_hand_strings("Ac", "Kc", "Qc", "Jc", "10c", "Ad", "As"),
                          eval_hand_strings("Ah", "Kh", "Qh", "Jh", "10h", "Ad", "As"))
        }

        printf("All Tests Ran Successfully.\n");
        return 0;
}

int main(int argc, char **argv) { return test(); }
