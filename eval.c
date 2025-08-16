#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint64_t Card;

enum Cards { NULL_CARD };
const uint64_t SPADE_OFFSET = 0;
const uint64_t HEART_OFFSET = 16;
const uint64_t DIAMOND_OFFSET = 32;
const uint64_t CLUB_OFFSET = 48;

const uint64_t SPADE_BITMASK = 0xFFFF;
const uint64_t HEART_BITMASK = SPADE_BITMASK << HEART_OFFSET;
const uint64_t DIAMOND_BITMASK = SPADE_BITMASK << DIAMOND_OFFSET;
const uint64_t CLUB_BITMASK = SPADE_BITMASK << CLUB_OFFSET;

const uint64_t DEUCE_BITMASK = 0x0001000100010001;
const uint64_t TREY_BITMASK = DEUCE_BITMASK << 1;
const uint64_t FOUR_BITMASK = DEUCE_BITMASK << 2;
const uint64_t FIVE_BITMASK = DEUCE_BITMASK << 3;
const uint64_t SIX_BITMASK = DEUCE_BITMASK << 4;
const uint64_t SEVEN_BITMASK = DEUCE_BITMASK << 5;
const uint64_t EIGHT_BITMASK = DEUCE_BITMASK << 6;
const uint64_t NINE_BITMASK = DEUCE_BITMASK << 7;
const uint64_t TEN_BITMASK = DEUCE_BITMASK << 8;
const uint64_t JACK_BITMASK = DEUCE_BITMASK << 9;
const uint64_t QUEEN_BITMASK = DEUCE_BITMASK << 10;
const uint64_t KING_BITMASK = DEUCE_BITMASK << 11;
const uint64_t ACE_BITMASK = DEUCE_BITMASK << 12;

// http:// suffe.cool/poker/evaluator.html
// https://en.wikipedia.org/wiki/Poker_probability
#define NUM_HIGH_CARD_HANDS 1277
#define NUM_STRAIGHT_FLUSHES 10
#define NUM_FOUR_OF_A_KINDS 156
#define NUM_FULL_HOUSES 156
#define NUM_FLUSHES NUM_HIGH_CARD_HANDS
#define NUM_STRAIGHTS 10
#define NUM_TRIPS 858
#define NUM_TWO_PAIRS 858
#define NUM_PAIRS 2860

const uint32_t STRAIGHT_FLUSH_INDEX = 0;
const uint32_t FOUR_OF_A_KIND_INDEX = STRAIGHT_FLUSH_INDEX + NUM_STRAIGHT_FLUSHES;
const uint32_t FULL_HOUSE_INDEX = FOUR_OF_A_KIND_INDEX + NUM_FOUR_OF_A_KINDS;
const uint32_t FLUSH_INDEX = FULL_HOUSE_INDEX + NUM_FULL_HOUSES;
const uint32_t STRAIGHT_INDEX = FLUSH_INDEX + NUM_FLUSHES;
const uint32_t TRIPS_INDEX = STRAIGHT_INDEX + NUM_STRAIGHTS;
const uint32_t TWO_PAIR_INDEX = TRIPS_INDEX + NUM_TRIPS;
const uint32_t PAIR_INDEX = TWO_PAIR_INDEX + NUM_TWO_PAIRS;
const uint32_t HIGH_CARD_INDEX = PAIR_INDEX + NUM_PAIRS;

uint64_t unique_five_lookup_table[NUM_HIGH_CARD_HANDS];

Card create_card(const char *representation) {
        if (strnlen(representation, 3) != 2) {
                return NULL_CARD;
        }

        Card card = 0;
        char rank_str = representation[0];
        char suit_str = representation[1];

        uint64_t suit = 0;
        switch (suit_str) {
        case 'c':
                card = CLUB_BITMASK;
                break;
        case 'd':
                card = DIAMOND_BITMASK;
                break;
        case 'h':
                card = HEART_BITMASK;
                break;
        case 's':
                card = SPADE_BITMASK;
                break;
        default:
                return NULL_CARD;
        }

        uint64_t rank = 0;
        switch (rank_str) {
        case '2':
                card &= DEUCE_BITMASK;
                break;
        case '3':
                card &= TREY_BITMASK;
                break;
        case '4':
                card &= FOUR_BITMASK;
                break;
        case '5':
                card &= FIVE_BITMASK;
                break;
        case '6':
                card &= SIX_BITMASK;
                break;
        case '7':
                card &= SEVEN_BITMASK;
                break;
        case '8':
                card &= EIGHT_BITMASK;
                break;
        case '9':
                card &= NINE_BITMASK;
                break;
        case 'T':
                card &= TEN_BITMASK;
                break;
        case 'J':
                card &= JACK_BITMASK;
                break;
        case 'Q':
                card &= QUEEN_BITMASK;
                break;
        case 'K':
                card &= KING_BITMASK;
                break;
        case 'A':
                card &= ACE_BITMASK;
                break;
        }

        return card;
}

uint32_t card_index(uint64_t card) {
        uint32_t index = __builtin_ctzll(card);

        return index - 3 * (index / 16);
}

/**
 * Returns the number of bits set, for at most 14-bit values
 */
uint32_t count_bits(uint64_t n) { return (n * 0x200040008001ULL & 0x111111111111111ULL) % 0xf; }

/**
 * Returns the rank of the straight, or 0 if none
 */
uint32_t straight_rank(uint64_t hand) {
        const uint64_t straight_bitmask = 0x1F00;
        uint64_t acc = hand;

        uint32_t rank = 14; // Ace
        do {
                // 11011 & 11111 -> 11011 -> 00100
                uint64_t mask = ~(acc & straight_bitmask) & straight_bitmask;
                if (!mask) {
                        // mask == 0 -> all 5 originally set
                        return rank;
                } else {
                        uint32_t lsb = 13 - __builtin_ctzll(mask);
                        rank -= lsb;
                        acc = (acc << lsb) & 0x1FFF;
                }
        } while (count_bits(acc) >= 5);

        // Is it a wheel?
        return (hand & 0x100F) == 0x100F ? 5 : 0;
}

uint64_t get_flushed_cards(uint64_t hand) {
        uint64_t club_bits = (hand & CLUB_BITMASK) >> CLUB_OFFSET;
        uint64_t heart_bits = (hand & HEART_BITMASK) >> HEART_OFFSET;
        uint64_t diamond_bits = (hand & DIAMOND_BITMASK) >> DIAMOND_OFFSET;
        uint64_t spade_bits = (hand & SPADE_BITMASK) >> SPADE_OFFSET;

        return (count_bits(club_bits) >= 5 ? club_bits : 0) |
               (count_bits(heart_bits) >= 5 ? heart_bits : 0) |
               (count_bits(diamond_bits) >= 5 ? diamond_bits : 0) |
               (count_bits(spade_bits) >= 5 ? spade_bits : 0);
}

/**
 * Attempts to evaluate the score of a hand as straight flush, or returns UINT64_MAX if not a
 * straight flush
 */
uint32_t eval_straight_flush(uint64_t hand) {
        uint64_t flush = get_flushed_cards(hand);
        if (!flush) {
                return UINT32_MAX;
        } else {
                uint32_t rank = straight_rank(flush);
                return rank == 0 ? UINT32_MAX : 14 - rank;
        }
}

/**
 * Attempts to evaluate the score of a hand as straight flush, or returns UINT64_MAX if not a
 * straight flush
 */
uint32_t eval_flush(uint64_t hand) {
        uint64_t flush = get_flushed_cards(hand);

        if (!flush) {
                return UINT32_MAX;
        }

        // Make sure we only have 5 bits set
        for (uint32_t i = count_bits(flush); i > 5; i -= 1) {
                // Flip the least significant 1-bit to 0
                flush = flush ^ (flush & -flush);
        }

        uint32_t l = 0;
        uint32_t r = 1277;
        uint32_t index = UINT32_MAX;
        while (l != r) {
                uint32_t mid = (l + r) / 2;
                if (unique_five_lookup_table[mid] > flush) {
                        r = mid;
                } else if (unique_five_lookup_table[mid] < flush) {
                        l = mid;
                } else {
                        index = mid;
                        break;
                }
        }

        if (index != UINT32_MAX) { // found index
                return FLUSH_INDEX + NUM_FLUSHES - index - 1;
        } else {
                return UINT32_MAX;
        }
}

uint32_t eval_high_card(uint64_t hand) {
        uint64_t hc =
            ((hand & CLUB_BITMASK) >> CLUB_OFFSET) | ((hand & HEART_BITMASK) >> HEART_OFFSET) |
            ((hand & DIAMOND_BITMASK) >> DIAMOND_OFFSET) | ((hand & SPADE_BITMASK) >> SPADE_OFFSET);

        for (uint32_t i = count_bits(hc); i > 5; i -= 1) {
                // Flip the least significant 1-bit to 0
                hc = hc ^ (hc & -hc);
        }

        uint32_t l = 0;
        uint32_t r = 1277;
        uint32_t index = UINT32_MAX;
        while (l != r) {
                uint32_t mid = (l + r) / 2;
                if (unique_five_lookup_table[mid] > hc) {
                        r = mid;
                } else if (unique_five_lookup_table[mid] < hc) {
                        l = mid;
                } else {
                        index = mid;
                        break;
                }
        }

        if (index != UINT32_MAX) { // found index
                return HIGH_CARD_INDEX + NUM_HIGH_CARD_HANDS - index - 1;
        } else {
                return UINT32_MAX;
        }
}

uint32_t eval_quads(uint64_t hand, uint64_t c, uint64_t h, uint64_t d, uint64_t s) {
        uint64_t quads = c & h & d & s;

        if (!quads) {
                return UINT32_MAX;
        }
        uint32_t rank = __builtin_ctzll(quads);
        uint32_t kicker = 63 - __builtin_clzll(hand & ~quads);
        return FOUR_OF_A_KIND_INDEX + (12 - rank) * 12 +
               ((kicker > rank) ? 12 - kicker : 11 - kicker);
}

uint32_t eval_full_house(uint64_t trips, uint64_t pairs) {
        if (trips && pairs) {
                uint32_t trips_rank = 63 - __builtin_clzll(trips);
                uint32_t pair_rank = 63 - __builtin_clzll(pairs);
                return FULL_HOUSE_INDEX + (12 - trips_rank) * 12 +
                       ((pair_rank > trips_rank) ? 12 - pair_rank : 11 - pair_rank);
        } else {
                return UINT32_MAX;
        }
}

uint32_t eval_trips(uint64_t trips, uint64_t hand) {
        if (!trips) {
                return UINT32_MAX;
        } else {
                uint64_t kickers = hand & ~trips;
                uint32_t trips_rank = 63 - __builtin_clzll(trips);
                uint32_t kicker1 = 63 - __builtin_clzll(kickers);
                uint32_t k1_adjusted = kicker1 - (kicker1 > trips_rank ? 1 : 0);
                uint32_t kicker2 = 63 - __builtin_clzll(kickers & ~(1 << kicker1));
                uint32_t k2_adjusted = kicker2 - (kicker2 > trips_rank ? 1 : 0);
                // printf("%d, %d\n", k1_adjusted, k2_adjusted);
                return TRIPS_INDEX + ((12 - trips_rank) * 66) +
                       (66 - k1_adjusted * (1 + k1_adjusted) / 2) + (k1_adjusted - k2_adjusted - 1);
        }
}

uint32_t eval_pair(uint64_t pairs, uint64_t hand) {
        // Called after we verified 1 pair
        uint32_t pair = 63 - __builtin_clzll(pairs);
        hand = hand & ~(1 << pair);
        uint32_t kicker1 = 63 - __builtin_clzll(hand);
        uint32_t kicker2 = 63 - __builtin_clzll(hand & ~(1 << kicker1));
        uint32_t kicker3 = 63 - __builtin_clzll(hand & ~(1 << kicker1) & ~(1 << kicker2));
        kicker1 -= kicker1 > pair ? 2 : 1;
        kicker2 -= kicker2 > pair ? 1 : 0;
        kicker3 -= kicker3 > pair ? 1 : 0;

        // printf("%d, %d, %d, %d\n", pair, kicker1, kicker2, kicker3);

        uint32_t p = 220 * pair;
        uint32_t h = (kicker1 * kicker1 * kicker1 - kicker1) / 6;
        uint32_t m = (kicker2 * (kicker2 - 1) / 2);
        uint32_t l = kicker3;

        return PAIR_INDEX + NUM_PAIRS - p - h - m - l - 1;
}

uint32_t eval_two_pair(uint64_t pairs, uint64_t hand) {
        // Called after we verified 2+ pairs
        uint32_t pair1 = 63 - __builtin_clzll(pairs);
        pairs = pairs & ~(1 << pair1);
        uint32_t pair2 = 63 - __builtin_clzll(pairs);
        uint32_t kicker = 63 - __builtin_clzll(hand & ~(1 << pair1) & ~(1 << pair2));
        kicker -= (kicker > pair2 ? (kicker > pair1 ? 2 : 1) : 0);

        return TWO_PAIR_INDEX + NUM_TWO_PAIRS - (pair1 * (pair1 - 1) / 2 * 11) - kicker -
               (pair2 * 11) - 1;
}

/**
 * Evaluates the value of the best possible 5-card hand given a 7 cards
 */
uint32_t eval_hand(uint64_t hand) {

        uint32_t straight_flush_eval = eval_straight_flush(hand);
        if (straight_flush_eval != UINT32_MAX) {
                return straight_flush_eval;
        }

        // Does hand contain a flush?
        uint32_t flush_eval = eval_flush(hand);
        if (flush_eval != UINT32_MAX) {
                return flush_eval;
        }

        // With 7 cards, a flush is mutually exclusive with four of a kind and full house, so we can
        // check these after
        uint64_t c = ((hand & CLUB_BITMASK) >> CLUB_OFFSET);
        uint64_t h = ((hand & HEART_BITMASK) >> HEART_OFFSET);
        uint64_t d = ((hand & DIAMOND_BITMASK) >> DIAMOND_OFFSET);
        uint64_t s = ((hand & SPADE_BITMASK) >> SPADE_OFFSET);
        uint64_t flattened = c | h | d | s;

        uint32_t quads_eval = eval_quads(flattened, c, h, d, s);
        if (quads_eval != UINT32_MAX) {
                return quads_eval;
        }

        // Trips bit will be set iff card is quads or trips, but since we already handled quads,
        // this represents exclusively trips
        uint32_t trips = (d & h & s) | (c & h & s) | (c & d & s) | (c & d & h);

        // Likewise, pairs are all combinations of 2+, and no 3+
        uint32_t pairs = ((d & s) | (h & s) | (h & d) | (c & s) | (c & d) | (c & h)) & ~trips;

        uint32_t full_house_eval = eval_full_house(trips, pairs);
        if (full_house_eval != UINT32_MAX) {
                return full_house_eval;
        }

        // Next, check for straight
        uint32_t straight_eval = straight_rank(flattened);
        if (straight_eval != 0) {
                return STRAIGHT_INDEX + 14 - straight_eval;
        }

        uint32_t trips_eval = eval_trips(trips, flattened);
        if (trips_eval != UINT32_MAX) {
                return trips_eval;
        }

        switch (count_bits(pairs)) {
        case 0:
                return eval_high_card(flattened);
        case 1:
                return eval_pair(pairs, flattened);
        default:
                return eval_two_pair(pairs, flattened);
        }
}

uint32_t eval_hand_strings(const char *c1, const char *c2, const char *c3, const char *c4,
                           const char *c5, const char *c6, const char *c7) {
        uint64_t hand = create_card(c1) | create_card(c2) | create_card(c3) | create_card(c4) |
                        create_card(c5) | create_card(c6) | create_card(c7);

        return eval_hand(hand);
}

void init_high_cards() {
        uint64_t index = 0;
        for (int i = 0; i < 1277; i += 1) {
                while (count_bits(index) != 5 || straight_rank(index)) {
                        index += 1;
                }

                unique_five_lookup_table[i] = index;
                index += 1;
        }
}
