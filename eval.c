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

#define NUM_HIGH_CARD_HANDS 1277

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

        uint32_t rank = 13; // Ace
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

/**
 * Attempts to evaluate the score of a hand as straight flush, or returns UINT64_MAX if not a
 * straight flush
 */
uint32_t eval_straight_flush(uint64_t hand) {
        // Does hand contain a straight flush?
        const uint64_t sf_bitmask = 0x1111100000000;
        uint32_t count = 0;

        while (count < 9) {
                uint64_t mask = hand & sf_bitmask;
                if (mask == sf_bitmask) {
                        return count;
                }
        }

        return UINT32_MAX;
}

/**
 * Attempts to evaluate the score of a hand as straight flush, or returns UINT64_MAX if not a
 * straight flush
 */
uint32_t eval_flush(uint64_t hand) {
        uint64_t club_bits = (hand & CLUB_BITMASK) >> CLUB_OFFSET;
        uint64_t heart_bits = (hand & HEART_BITMASK) >> HEART_OFFSET;
        uint64_t diamond_bits = (hand & DIAMOND_BITMASK) >> DIAMOND_OFFSET;
        uint64_t spade_bits = (hand & SPADE_BITMASK) >> SPADE_OFFSET;

        uint64_t flush = (count_bits(club_bits) >= 5 ? club_bits : 0) |
                         (count_bits(heart_bits) >= 5 ? heart_bits : 0) |
                         (count_bits(diamond_bits) >= 5 ? diamond_bits : 0) |
                         (count_bits(spade_bits) >= 5 ? spade_bits : 0);

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
                return 1599 - index;
        } else {
                return UINT32_MAX;
        }
}

/**
 * Evaluates the value of the best possible 5-card hand given a 7 cards
 */
uint32_t eval_hand(uint64_t hand) {
        // Does hand contain a flush?
        uint32_t flush_eval = eval_flush(hand);
        if (flush_eval != UINT32_MAX) {
                return flush_eval;
        }
        return UINT32_MAX;
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

        printf("smallest: %lx | biggest: %lx\n", unique_five_lookup_table[0],
               unique_five_lookup_table[1276]);
}
