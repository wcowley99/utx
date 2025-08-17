#include "eval.c"

#include <stdlib.h>

const double ANTE = 1.0;
const double BLIND = 1.0;

uint64_t lookup_table[2118760];
double flop_scoring_table[2118760];
double river_scoring_table[2118760];

double max(double a, double b) { return a > b ? a : b; }

// https://stackoverflow.com/questions/506807/creating-multiple-numbers-with-certain-number-of-bits-set
uint64_t next_combination(uint64_t x, uint64_t deadzones) {
        do {
                uint64_t smallest = (x & -x);
                uint64_t ripple = x + smallest;
                uint64_t new_smallest = ripple & -ripple;
                x = ripple | ((new_smallest / smallest) >> 1) - 1;
        } while (x & deadzones);

        return x;
}

double blind_payout(uint32_t score) {
        if (score == 0) { // royal flush
                return 500.0;
        } else if (score < STRAIGHT_FLUSH_INDEX + NUM_STRAIGHT_FLUSHES) {
                return 50.0;
        } else if (score < FOUR_OF_A_KIND_INDEX + NUM_FOUR_OF_A_KINDS) {
                return 10.0;
        } else if (score < FLUSH_INDEX + NUM_FLUSHES) {
                return 1.5;
        } else if (score < STRAIGHT_INDEX + NUM_STRAIGHTS) {
                return 1.0;
        } else {
                // did not qualify
                return 0.0;
        }
}

double get_payout(uint64_t hand, uint64_t board, uint64_t dealer, double bet) {
        uint32_t player_score = eval_hand(board | hand);
        uint32_t dealer_score = eval_hand(board | dealer);

        if (player_score < dealer_score) { // player wins
                double ante = dealer_score < HIGH_CARD_INDEX ? ANTE : 0;
                double blind = blind_payout(player_score);

                return ante + blind + bet;
        } else if (player_score > dealer_score) {
                return -(ANTE + BLIND + bet);
        } else {
                // push
                return 0.0;
        }
}

double simulate_runout(uint64_t hand, uint64_t deck) {
        printf("Simulating runout\n");
        uint64_t board = 0x1F;
        double total = 0.0;

        // problem space reductions:
        // # clubs > # spades -> skip
        // # clubs < # spades -> x2

        const uint64_t deadzones = 0xE000E000E000E000;
        const uint64_t runouts = 2118760;  // 50 choose 5
        const uint64_t dealer_cards = 990; // 45 choose 2

        if (hand & board) {
                board = next_combination(board, deadzones | hand);
        }

        uint32_t count = 0;
        for (int i = 0; i < runouts; i += 1) {
                uint64_t dealer = 0x3;
                double reduction_scalar = 1.0;
                double subtotal = 0.0;
                double flop_total = 0.0;
                double river_total = 0.0;
                bool skip = false;

                uint64_t clubs = (board & CLUB_BITMASK) >> CLUB_OFFSET;
                uint64_t diamonds = (board & DIAMOND_BITMASK) >> DIAMOND_OFFSET;
                uint64_t hearts = (board & HEART_BITMASK) >> HEART_OFFSET;
                uint64_t spades = (board & SPADE_BITMASK) >> SPADE_OFFSET;
                bool rainbow = clubs != 0 && hearts != 0 && diamonds != 0 && spades != 0;

                if (dealer & board) {
                        dealer = next_combination(dealer, deadzones | hand | board);
                }

                if (clubs > spades) {
                        skip = true;
                } else if (clubs < spades) {
                        reduction_scalar *= 2;
                }

                if (!skip) {
                        count += 1;
                        for (int k = 0; k < dealer_cards; k += 1) {
                                // printf("dealer: %lx\n", dealer);
                                subtotal += get_payout(hand, board, dealer, 4.0);
                                flop_total += get_payout(hand, board, dealer, 2.0);
                                river_total += get_payout(hand, board, dealer, 1.0);

                                if (k != dealer_cards - 1) {
                                        dealer = next_combination(dealer, deadzones | hand | board);
                                }
                        }
                }

                total += reduction_scalar * subtotal;
                lookup_table[i] = board;
                flop_scoring_table[i] = flop_total;
                river_scoring_table[i] = river_total;

                if (i != runouts - 1) {
                        board = next_combination(board, deadzones | hand);
                }
        }

        printf("total (count %d): %f\n", count, total / (runouts * dealer_cards));
        return total / (runouts * dealer_cards);
}

double simulate_river(uint64_t hand, uint64_t board, uint64_t deck, bool flop_bet) {
        uint64_t river = 0x3;
        double total = 0.0;

        const uint64_t deadzones = 0xE000E000E000E000;
        const uint32_t runouts = 1081;     // 47 choose 3
        const uint32_t dealer_cards = 990; // 45 choose 2

        if ((hand | board) & river) {
                river = next_combination(river, deadzones | hand | board);
        }

        for (int i = 0; i < runouts; i += 1) {
                uint32_t l = 0;
                uint32_t r = 2118760;
                uint32_t index = UINT32_MAX;
                uint64_t cc = board | river;
                while (l != r) {
                        uint32_t mid = (l + r) / 2;
                        // printf("%lx => [%d, %d], mid=%lx\n", cc, l, r, lookup_table[mid]);
                        if (lookup_table[mid] > cc) {
                                r = mid;
                        } else if (lookup_table[mid] < cc) {
                                l = mid;
                        } else {
                                index = mid;
                                break;
                        }
                }

                total += flop_bet ? flop_scoring_table[index]
                                  : max(river_scoring_table[index], -ANTE - BLIND);

                if (i != runouts - 1) {
                        river = next_combination(river, deadzones | hand | board);
                }
        }

        return total / (runouts * dealer_cards);
}

double simulate_flop(uint64_t hand, uint64_t deck) {
        uint64_t board = 0x7;
        double total = 0.0;

        const uint64_t deadzones = 0xE000E000E000E000;
        const uint64_t runouts = 19600; // 50 choose 3

        if (hand & board) {
                board = next_combination(board, deadzones | hand);
        }

        for (int i = 0; i < runouts; i += 1) {

                total += max(simulate_river(hand, board, deck, false),
                             simulate_river(hand, board, deck, true));
                printf("Evaluated flop %d/19600\n", i);

                if (i != runouts - 1) {
                        board = next_combination(board, deadzones | hand);
                }
        }

        return total / runouts;
}

double simulate(char first_rank, char second_rank, bool suited) {
        int ante = 1;
        int blind = ante;
        double bet = 0.0;

        // Hole cards
        char card[3] = "_h\0";
        card[0] = first_rank;
        uint64_t first_card = create_card(card);

        card[0] = second_rank;
        card[1] = suited ? 'h' : 'd';
        uint64_t second_card = create_card(card);

        uint64_t deck = 0x1FFF1FFF1FFF1FFF ^ (first_card | second_card);

        double maxbet_ev = simulate_runout(first_card | second_card, deck);
        double flop_ev = simulate_flop(first_card | second_card, deck);

        printf("hold ev: %f, bet ev: %f", flop_ev, maxbet_ev);

        return max(maxbet_ev, flop_ev);
}

int main(int argc, char **argv) {
        init_high_cards();
        bool suited = false;
        char r1 = 'A';
        char r2 = 'K';
        double score = simulate(r1, r2, suited);

        printf("Score for %s %c%c: %f\n", suited ? "suited" : "unsuited", r1, r2, score);

        return 0;
}
