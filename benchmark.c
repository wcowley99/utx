#include "eval.c"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
        init_high_cards();

        const uint64_t suits[4] = {
            CLUB_BITMASK,
            DIAMOND_BITMASK,
            HEART_BITMASK,
            SPADE_BITMASK,
        };
        const uint64_t ranks[13] = {
            DEUCE_BITMASK, TREY_BITMASK,  FOUR_BITMASK, FIVE_BITMASK, SIX_BITMASK,
            SEVEN_BITMASK, EIGHT_BITMASK, NINE_BITMASK, TEN_BITMASK,  JACK_BITMASK,
            QUEEN_BITMASK, KING_BITMASK,  ACE_BITMASK,
        };

        uint64_t cards[52];

        for (int i = 0; i < 52; i += 1) {
                cards[i] = suits[i % 4] & ranks[i % 13];
        }

        uint32_t count = 0;
        clock_t begin = clock();
        for (int c1 = 0; c1 < 52 - 6; c1 += 1) {
                for (int c2 = c1 + 1; c2 < 52 - 5; c2 += 1) {
                        for (int c3 = c2 + 1; c3 < 52 - 4; c3 += 1) {
                                for (int c4 = c3 + 1; c4 < 52 - 3; c4 += 1) {
                                        for (int c5 = c4 + 1; c5 < 52 - 2; c5 += 1) {
                                                for (int c6 = c5 + 1; c6 < 52 - 1; c6 += 1) {
                                                        for (int c7 = c6 + 1; c7 < 52; c7 += 1) {
                                                                uint64_t hand =
                                                                    cards[c1] | cards[c2] |
                                                                    cards[c3] | cards[c4] |
                                                                    cards[c5] | cards[c6] |
                                                                    cards[c7];
                                                                count += 1;
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }
        clock_t end = clock();
        clock_t ms = 1000 * (end - begin) / CLOCKS_PER_SEC;
        printf("Time to enumerate all %d possible hands: %ldms\n", count, ms);

        count = 0;
        begin = clock();
        for (int c1 = 0; c1 < 52 - 6; c1 += 1) {
                for (int c2 = c1 + 1; c2 < 52 - 5; c2 += 1) {
                        for (int c3 = c2 + 1; c3 < 52 - 4; c3 += 1) {
                                for (int c4 = c3 + 1; c4 < 52 - 3; c4 += 1) {
                                        for (int c5 = c4 + 1; c5 < 52 - 2; c5 += 1) {
                                                for (int c6 = c5 + 1; c6 < 52 - 1; c6 += 1) {
                                                        for (int c7 = c6 + 1; c7 < 52; c7 += 1) {
                                                                uint64_t hand =
                                                                    cards[c1] | cards[c2] |
                                                                    cards[c3] | cards[c4] |
                                                                    cards[c5] | cards[c6] |
                                                                    cards[c7];
                                                                eval_hand(hand);
                                                                count += 1;
                                                        }
                                                }
                                        }
                                }
                        }
                }
        }
        end = clock();
        clock_t diff = 1000 * (end - begin) / CLOCKS_PER_SEC;
        printf("Time to evaluate all %d possible hands minus loop times: %ldms\n", count,
               diff - ms);
}
