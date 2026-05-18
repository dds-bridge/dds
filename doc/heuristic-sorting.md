5.	The Move Ordering algorithm

The weight of a card in the move list is affected by the suit and the rank of the card and by the other cards in the same trick.  The weights of the cards in the move list are used to sort them, with the cards having the highest weight being sorted first in the list. 

If the hand-to-play is the trick-leading hand or is void in the suit played by leading hand, the card with the highest weight for each present suit will get a high additional bonus weight. After list resorting, those cards will occupy the first positions in the move list.

Two "best moves" are maintained for each searched depth, one for an alpha-beta cutoff and one at a Transposition Table entry match. At an alpha-beta cutoff, the move causing the cutoff overwrites the present "best move" for the current depth. When a Transposition Table entry is created, the current best move is stored in that entry if:

•	The target is met and the leading hand belongs to the player’s side, 
•	Or the target is not met and the leading hand belongs to the other side. 
•	Otherwise the best move is not stored in the Transposition Table entry. 

At a Transposition Table entry match, its stored best move will be best move for the current search depth.

By ”card move” in the following pseudo code is meant the card by the hand-to-play that is getting a weight in the move list. The ”card rank” is a value in the range 2 (deuce) to 14 (Ace). 

For the determination of the weight, it is calculated whether or not the current card move wins the current trick for the side of the hand-to-play, assuming that both sides play optimum cards. 

The following pseudo-code contains empirical weights that are used to obtain move orderings that tend to put optimum move early in the list of moves.  These may or may not be the exact weights and algorithms used in the current DDS version, but they give an idea of the important factors; the code is significantly more complex.  One aim is to move the likely candidates to the top of the list, and another aim is to have good mixture of moves (i.e. not all cards from the same suit first) in case the heuristic is not good for a particular set-up.

If the hand-to-play is void in the trick lead suit, the suit selected for the discard gets a bonus:

suitAdd = ((suit length) * 64)/36;

If the suit length is 2, and the hand-to-play has the next highest rank of the suit, the bonus is reduced by 2. 


Hand-to-play is trick-leading hand

The contribution of the suit to the weight is:
 
suitWeightDelta = suitBonus – ((countLH+countRH) * 32)/15

suitBonus has the initial value 0, changed if conditions below apply:

If it is a trump contract, and the suit is not trump, then there is a suitBonus change of  –10 if
 
LHO is void and LHO has trump card(s), or
RHO is void and RHO has trump card(s).
If RHO has either the highest rank of the suit played by hand-to-play or the next highest rank, then there is a suitBonus change of –18.  

If it is a trump contract, the suit is not trump, the own hand has a singleton, the own hand has at least one trump, partner has the highest rank in the suit and at least a suit length of 2, then there is a suitBonus change of +16.  Suits are thus favoured where the opponents have as few move alternatives as possible.

countLH = (suit length of LHO) * 4, if LHO is not void in the suit,
countLH = (depth + 4), if LHO is void in the suit
countRH = (suit length of RHO) * 4, if RHO is not void in the suit,
countRH = (depth + 4), if RHO is void in the suit

if (trick winning card move) {
    if (one of the opponents has a singleton highest rank in the suit)
        weight = suitWeightDelta + 40 – (rank of card move)
    else if (hand-to-play has highest rank in suit)  {
        if (partner has second highest rank in suit)
            weight = suitWeightDelta + 50 – (rank of card move)
        else if  (the card move is the card with highest rank in the suit)
            weight = suitWeightDelta + 31
        else
            weight = suitWeightDelta + 19 – (rank of card move)
    }
    else if  (partner  has highest rank in suit)  {
        if (hand-to-play has second highest rank in suit)
            weight = suitWeightDelta + 50 – (rank of card move)
        else
            weight = suitWeightDelta + 35 – (rank of card move)  
    }
    else if  (hand-to-play has second highest rank together with 
              equivalent card(s) in suit)
        weight = suitWeightDelta + 40 
    else
        weight = suitWeightDelta + 30 – (rank of card move)
    if  (the card move  is ”best move” as obtained at alpha-beta cutoff)
        weight = weight + 52;
    if  (the card move is ”best move” as obtained from the Transposition Table)
        weight = weight + 11;
}
else {	/* Not a trick winning move */
    if  (either LHO or RHO has singleton in suit which has highest rank)
        weight = suitWeightDelta + 29 – (rank of card move)
    else if (hand-to-play has highest rank in suit)  {
        if (partner has second highest rank in suit)
            weight = suitWeightDelta + 44 – (rank of card move)
        else if  (the card move is the card with highest rank in the suit)
            weight = suitWeightDelta + 25
        else
            weight = suitWeightDelta + 13 – (rank of card move)
    }
    else if  (partner  has highest rank in suit)  {
        if (hand-to-play has second highest rank in suit)
            weight = suitWeightDelta + 44 – (rank of card move)
        else
            weight = suitWeightDelta + 29 – (rank of card move)  
    }
    else if  (hand-to-play has second highest rank together with 
              equivalent card(s) in suit)
        weight = suitWeightDelta + 29 
    else
        weight = suitWeightDelta + 13 – (rank of card move)
    if  (the card move  is ”best move” as obtained at alpha-beta cutoff)
        weight = weight + 20;
    if  (the card move  is ”best move” as obtained from the Transposition Table)
        weight = weight + 9;
}

Hand-to-play is left hand opponent (LHO) to leading hand

if (trick winning card move) {
    if  (hand-to-play void in the suit played by the leading hand)  {
        if  (trump contract and trump is equal to card move suit)
            weight = 30 - (rank of card move) + suitAdd
        else
            weight = 60 - (rank of card move) + suitAdd
    }
    else if (lowest card for partner to leading hand is higher than LHO played card)
        weight = 45 - (rank of card move)
    else if (RHO has a card in the leading suit higher than the leading card             but lower than the highest rank of the leading hand)
        weight = 60 - (rank of card move)
    else if (LHO played card is higher than card played by the leading hand) {
        if (played card by LHO is lower than any card for RHO in the same suit)
            weight = 75 - (rank of card move)
        else if (LHO’s card by LHO beats any card in that suit for the leading hand)
            weight = 70 - (rank of card move)
        else  {
            if  (LHO move card has at least one equivalent card) {
                weight = 60 - (rank of card move) 
            else
                weight = 45 - (rank of card move)
        }
    }
    else if (RHO is not void in the suit played by the leading hand) {
        if  (LHO move card has at least one equivalent card)    
            weight = 50 - (rank of card move)
        else
            weight = 45 - (rank of card move)
    }
    else
        weight = 45 - (rank of card move)
}
else {	// card move is not trick winning
    if  (hand-to-play void in the suit played by the leading hand)  {
        if  (trump contract and trump is equal to card move suit)
            weight = 15 - (rank of card move) + suitAdd
        else
            weight = - (rank of card move) + suitAdd
    }
    else if (lowest card for leader’s partner or for RHO in the suit played is 
             higher than played card for LHO) 
        weight = - (rank of card move) 
    else if (LHO played card is higher than card played by the leading hand) { 
        if  (LHO move card has at least one equivalent card)
            weight = 20 - (rank of card move) 
         else 
             weight = 10 - (rank of card move)
     }  
     else 
         weight = - (rank of card move)
}      



Hand-to-play is partner to trick leading hand

if (trick winning card move) {
    if  (hand-to-play void in the suit played by the leading hand)  {
        if (card played by the leading hand is highest so far) {
            if (card by hand-to-play is trump and the suit played by the 
                leading hand is not trump) 
                weight = 30 - (rank of card move) + suitAdd
            else
                weight = 60 - (rank of card move) + suitAdd
        }
        else if (hand-to-play is on top by ruffing)
            weight = 70 - (rank of card move) + suitAdd
        else if (hand-to-play discards a trump but still loses)
            weight = 15 - (rank of card move) + suitAdd 
        else       
            weight = 30 - (rank of card move) + suitAdd
    }
    else 
        weight = 60 - (rank of card move) 
}
else {               // card move is not trick winning
    if  (hand-to-play void in the suit played by the leading hand)  {
        if (hand-to-play is on top by ruffing)
            weight = 40 - (rank of card move) + suitAdd
        else if (hand-to-play underruffs */
            weight = -15 - (rank of card move) + suitAdd
        else
            weight = - (rank of card move) + suitAdd
    }
    else {
         if (the card by hand-to-play is highest so far) {
             if (rank of played card is second highest in the suit)
                weight = 25  
             else if (hand-to-play card has at least one equivalent card)
                 weight = 20 - (rank of card move)
             else
                 weight = 10 - (rank of card move)
         }
         else
             weight = -10 - (rank of card move)
    }
}

Hand-to-play is right hand opponent (RHO) to leading hand

if  (hand-to-play is void in leading suit)  {
    if  (LHO has current highest rank of the trick)  {
       if  (card move ruffs)
          weight = 14- (rank of card move) + suitAdd
       else
          weight = 30- (rank of card move) + suitAdd 
     }
    else if  (hand-to-play ruffs and wins) 
         weight = 30- (rank of card move) + suitAdd
    else if  (card move suit is trump, but not winning)
        weight = - (rank of card move)
    else
        weight = 14- (rank of card move) + suitAdd
}
else if  (LHO has current winning move)  {
    if  (RHO ruffs LHO’s winner)
        weight = 24 - (rank of card move) 
    else
        weight = 30- (rank of card move) 
}
else if  (card move superior to present winning move not by LHO)  {        
     weight = 30- (rank of card move)
else  {
    if  (card move ruffs but still losing)
        weight = - (rank of card move)
    else
        weight = 14- (rank of card move)
}