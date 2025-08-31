#include "utility/Constants.h"
#include <iostream>

int main() {
    // Verify hand relationship arrays work
    std::cout << "LHO of North (0): " << lho[0] << std::endl;  // Should be 1 (East)
    std::cout << "RHO of North (0): " << rho[0] << std::endl;  // Should be 3 (West)
    std::cout << "Partner of North (0): " << partner[0] << std::endl;  // Should be 2 (South)
    
    // Verify card arrays work
    std::cout << "Card rank for rank 14: " << cardRank[14] << std::endl;  // Should be 'A'
    std::cout << "Card suit for spades (0): " << cardSuit[0] << std::endl;  // Should be 'S'
    
    return 0;
}
