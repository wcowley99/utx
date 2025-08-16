#include "eval.c"

#include <stdlib.h>

const double ANTE = 1.0;
const double BLIND = 1.0;

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

double simulate_runout(uint64_t hand, uint64_t deck, double bet) {
        uint64_t board = 0x1F;
        double total = 0.0;

        const uint64_t deadzones = 0x0000E000E000E000;
        const uint64_t runouts = 2118760;  // 50 choose 5
        const uint64_t dealer_cards = 990; // 45 choose 2

        for (int i = 0; i < runouts; i += 1) {
                uint64_t dealer = 0x3;

                for (int k = 0; k < dealer_cards; k += 1) {
                        total += get_payout(hand, board, dealer, bet);
                        dealer = next_combination(dealer, deadzones | hand | board);
                }
                board = next_combination(board, deadzones | hand);
        }

        printf("total: %f\n", total / (runouts * dealer_cards));
        return total / (runouts * dealer_cards);
}

double simulate_flop(uint64_t hand, uint64_t deck, double bet) { return 0.0f; }

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

        return max(simulate_flop(first_card | second_card, deck, 0.0),
                   simulate_runout(first_card | second_card, deck, 4.0));
}

int main(int argc, char **argv) {
        init_high_cards();
        bool suited = false;
        char r1 = 'A';
        char r2 = 'A';
        double score = simulate(r1, r2, suited);

        printf("Score for %s %c%c: %f\n", suited ? "suited" : "unsuited", r1, r2, score);

        return 0;
}
