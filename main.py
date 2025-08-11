from phevaluator import evaluate_cards
import random

suits = ['d', 's', 'c', 'h']
ranks = ['A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K']

cards = []
for r in ranks:
    for s in suits:
        cards.append(r+s)

pair_eval = evaluate_cards('2d', '2s', '3c', '4h', '5d') # worst pair
straight_eval = evaluate_cards('Ad', '2s', '3c', '4h', '5d') # worst straight
flush_eval = evaluate_cards('2s', '3s', '4s', '5s', '7s') # worst flush
full_house_eval = evaluate_cards('2s', '2d', '2h', '3s', '3d') # worst full house
quads_eval = evaluate_cards('2s', '2d', '2h', '2s', '3d') # worst quads
straight_flush_eval = evaluate_cards('Ad', '2d', '3d', '4d', '5d') # worst straight flush

# The specific suit does not matter, only whether it is suited or unsuited
player_hands = []
for c1 in range(len(ranks)):
    for c2 in range(c1+1):
        r1 = ranks[c1]
        r2 = ranks[c2]
        player_hands.append((r1+'d', r2+'h'))
        # Can't have suited pairs
        if r1 != r2:
            player_hands.append((r1+'d', r2+'d'))

maxbet_evs = dict.fromkeys(player_hands, 0)
flop_evs = dict.fromkeys(player_hands, 0)
river_evs = dict.fromkeys(player_hands, 0)

def is_pair(hand):
    return hand[0][0] == hand[1][0]

def is_suited(hand):
    return hand[0][1] == hand[1][1]

def hand_ev(hand) -> float:
    return max(maxbet_evs[hand], flop_evs[hand], river_evs[river], -2)

def blind_payout(dealer_eval) -> float:
    if dealer_eval <= straight_flush_eval:
        return 50
    if dealer_eval <= quads_eval:
        return 10
    if dealer_eval <= full_house_eval:
        return 3
    if dealer_eval <= flush_eval:
        return 1.5
    if dealer_eval <= straight_eval:
        return 1
    else:
        return 0

def evaluate(player, dealer, board, ante, bet) -> float:
    player_eval = evaluate_cards(player[0], player[1], board[0], board[1], board[2], board[3], board[4])
    dealer_eval  = evaluate_cards(dealer[0], dealer[1], board[0], board[1], board[2], board[3], board[4])
    if dealer_eval < player_eval:
        return -2 * ante - bet
    if dealer_eval == player_eval:
        return 0
    else:
        # player won
        if pair_eval < dealer_eval:
            # dealer did not make a pair
            return bet
        else:
            return ante + bet + blind_payout(dealer_eval)

def flop_rule(hand, board) -> bool:
    # Bet if hand is a pair or better on the flop
    flop_eval = evaluate_cards(hand[0], hand[1], board[0], board[1], board[2])
    return flop_eval < pair_eval

def river_rule(hand, board) -> bool:
    # Bet if hand is a pair or better on the river
    river_eval = evaluate_cards(hand[0], hand[1], board[0], board[1], board[2], board[3], board[4])
    return river_eval < pair_eval

def simulate(hand, others=[]):
    ante = 1
    bet = 0
    deck = random.sample(cards, len(cards))
    deck = [x for x in deck if x not in hand]

    dealer = deck[:2]
    deck = deck[2:]

    flop = deck[:3]
    deck = deck[3:]
    river = deck[:2]

    board = flop + river

    maxbet_evs[hand] += evaluate(hand, dealer, board, ante, 4)
    if flop_rule(hand, flop):
        flop_evs[hand] += evaluate(hand, dealer, board, ante, 2)
    elif river_rule(hand, board):
        ev = evaluate(hand, dealer, board, ante, 1)
        flop_evs[hand] += ev
        river_evs[hand] += ev
    else:
        flop_evs[hand] -= 2
        river_evs[hand] -= 2

rounds = 1000
    
for i in range(rounds):
    for hand in player_hands:
        simulate(hand)

for hand in player_hands:
    maxbet_evs[hand] = maxbet_evs[hand] / rounds
    flop_evs[hand] = flop_evs[hand] / rounds
    river_evs[hand] = river_evs[hand] / rounds

total_ev = 0
for hand in player_hands:
    ev = -2
    play = "fold"
    if river_evs[hand] >  ev:
        ev = river_evs[hand]
        play = "bet on river"
    if flop_evs[hand] > ev:
        ev = flop_evs[hand]
        play = "bet on flop"
    if maxbet_evs[hand] > ev:
        ev = maxbet_evs[hand]
        play = "maxbet hand"

    if is_pair(hand):
        total_ev += 6 * ev
    elif is_suited(hand):
        total_ev += 4 * ev
    else:
        total_ev += 12 * ev

    print(f"{hand} => {play} for {ev} ev")

total_ev = total_ev / 1326
print(f"total ev = {total_ev}")
