#include "eval.c"

#include <stdio.h>
#include <stdlib.h>

#define ASSERT_EQ(x, y)                                                                            \
        do {                                                                                       \
                if (x != y) {                                                                      \
                        printf("[%s:%d]: %s is not equal to %s.\n", __FILE__, __LINE__, #x, #y);   \
                        return -1;                                                                 \
                }                                                                                  \
        } while (0)

#define ASSERT_EQ2(x, y)                                                                           \
        do {                                                                                       \
                if (x != y) {                                                                      \
                        printf("[%s:%d]: %d is not equal to %d.\n", __FILE__, __LINE__, x, y);     \
                        return -1;                                                                 \
                }                                                                                  \
        } while (0)

#define ASSERT(x)                                                                                  \
        do {                                                                                       \
                if (!(x)) {                                                                        \
                        printf("[%s:%d]: Expected '%s' to be true, was false.\n", __FILE__,        \
                               __LINE__, #x);                                                      \
                }                                                                                  \
        } while (0)

int test_suite(const char *filename) {
        FILE *file;
        char *line;
        size_t len = 0;
        ssize_t read;

        file = fopen(filename, "r");

        uint32_t count = 0;
        do {
                printf("count: %d\n", count);
                read = getline(&line, &len, file);

                for (int j = 2; j < 15; j += 3) {
                        line[j] = '\0';
                }

                for (int j = 0; j < 13; j += 3) {
                        printf("%s ", &line[j]);
                }
                printf("\n");

                ASSERT_EQ2(eval_hand_strings(line, line + 3, line + 6, line + 9, line + 12, "", ""),
                           count);

                count += 1;
        } while (read != -1);

        if (line) {
                free(line);
        }
        fclose(file);

        return 0;
}

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

                ASSERT_EQ(straight_rank(0x1F00), 14); // Ace
                ASSERT_EQ(straight_rank(0x0F80), 13); // King
                ASSERT_EQ(straight_rank(0x07C0), 12); // Queen
                ASSERT_EQ(straight_rank(0x100F), 5);  // Wheel
        }

        {
                printf("Testing Straight Flush Evaluations\n");

                ASSERT_EQ(eval_hand_strings("Ad", "Kd", "Qd", "Jd", "Td", "2s", "3s"), 0);
                ASSERT_EQ(eval_hand_strings("Ad", "5d", "4d", "3d", "2d", "2s", "3s"), 9);

                // Straight Flushes of different suits are equal
                ASSERT_EQ(eval_hand_strings("6d", "5d", "4d", "3d", "2d", "2s", "3s"),
                          eval_hand_strings("6c", "5c", "4c", "3c", "2c", "2s", "3s"));

                // 6th and 7th cards don't score
                ASSERT_EQ(eval_hand_strings("Ad", "Kd", "Qd", "Jd", "Td", "2s", "3s"),
                          eval_hand_strings("Ad", "Kd", "Qd", "Jd", "Td", "9d", "8d"));
        }

        {
                printf("Testing Flush Evaluations\n");

                ASSERT_EQ(eval_hand_strings("Ad", "Kd", "Qd", "Jd", "9d", "2s", "3s"), 322);
                ASSERT_EQ(eval_hand_strings("7d", "5d", "4d", "3d", "2d", "2s", "3s"), 1598);

                // Flushes of different suits are equal
                ASSERT_EQ(eval_hand_strings("7d", "5d", "4d", "3d", "2d", "2s", "3s"),
                          eval_hand_strings("7c", "5c", "4c", "3c", "2c", "2s", "3s"));

                // 6th and 7th cards don't score
                ASSERT_EQ(eval_hand_strings("Ad", "Kd", "Qd", "Jd", "9d", "2s", "3s"),
                          eval_hand_strings("Ad", "Kd", "Qd", "Jd", "9d", "8d", "3d"));
        }

        {
                printf("Testing Straight Evaluations\n");

                ASSERT_EQ(eval_hand_strings("Ac", "Kh", "Qd", "Jc", "Td", "2s", "3s"), 1599);
                ASSERT_EQ(eval_hand_strings("Ad", "5h", "4d", "3c", "2d", "2s", "3s"), 1608);

                // Straight with different suits are equal
                ASSERT_EQ(eval_hand_strings("6h", "5d", "4c", "3d", "2d", "2s", "3s"),
                          eval_hand_strings("6d", "5s", "4d", "3c", "2c", "2s", "3s"));

                // 6th and 7th cards don't score
                ASSERT_EQ(eval_hand_strings("Ac", "Kd", "Qd", "Jh", "Td", "2s", "3s"),
                          eval_hand_strings("Ah", "Ks", "Qd", "Jc", "Td", "9d", "8d"));
        }

        // {
        //         printf("Testing High Card Evaluations\n");
        //
        //         ASSERT_EQ(eval_hand_strings("Ad", "Kh", "Qc", "Jd", "9c", "2s", "3s"), 6185);
        //         ASSERT_EQ(eval_hand_strings("7h", "5h", "4c", "3d", "2d", "", ""), 7461);
        //
        //         // 5th kicker wins
        //         ASSERT(eval_hand_strings("8d", "6d", "5d", "4c", "2c", "", "") >
        //                eval_hand_strings("8c", "6h", "5c", "4d", "3d", "", ""));
        //
        //         // 6th and 7th cards don't score
        //         ASSERT_EQ(eval_hand_strings("Ah", "Kc", "Qc", "Js", "9d", "2s", "3s"),
        //                   eval_hand_strings("Ad", "Kd", "Qs", "Jh", "9d", "8s", "3d"));
        // }

        {
                printf("Testing Four of a Kind Evaluations\n");

                printf("%d\n", eval_hand_strings("Ad", "Ah", "Ac", "As", "Kc", "2s", "3s"));
                printf("%d\n", eval_hand_strings("2d", "2h", "2c", "2s", "3d", "3c", "3s"));
                ASSERT_EQ(eval_hand_strings("Ad", "Ah", "Ac", "As", "Kc", "2s", "3s"), 10);
                ASSERT_EQ(eval_hand_strings("2d", "2h", "2c", "2s", "3d", "3c", "3s"), 165);

                // kicker wins
                ASSERT(eval_hand_strings("8d", "8s", "8c", "8h", "2c", "3c", "4c") >
                       eval_hand_strings("8c", "8h", "8s", "8d", "Ad", "3c", "4c"));

                // Ace-high kicker beats next hand
                ASSERT(eval_hand_strings("8h", "8d", "8c", "8s", "Ad", "2s", "3s") >
                       eval_hand_strings("9d", "9c", "9s", "9h", "2d", "2s", "2h"));
        }

        {
                printf("Testing Evaluation Function\n");

                // Two royal flushes should have equal values
                ASSERT_EQ(eval_hand_strings("Ac", "Kc", "Qc", "Jc", "Tc", "Ad", "As"),
                          eval_hand_strings("Ah", "Kh", "Qh", "Jh", "Th", "Ad", "As"));
        }

        {
                printf("Testing vs. 5-card suite\n");

                return test_suite("test-suite-1.txt") || test_suite("test-suite-1.txt") ||
                       test_suite("test-suite-1.txt") || test_suite("test-suite-1.txt");
        }

        printf("All Tests Ran Successfully.\n");
        return 0;
}

int main(int argc, char **argv) { return test(); }
