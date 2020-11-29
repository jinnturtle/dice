#include <iostream>
#include <string>

#include "Randomizer.hpp"

auto main(int argc, char** argv) -> int
{
    Randomizer rand;
    
    std::string dice_str;
    
    for (int i {1}; i < argc; ++i) {
        std::cout << "arg " << i << ": " << argv[i] << std::endl;
        if (argv[i][0] == '-') {
            std::cout << "sorry, args processing not implemented yet"
            << std::endl;
        } else {
            dice_str = argv[i];
        }
    }
    
    int result {0};
    if (dice_str.size() > 0) {
        result = rand.roll_dice(dice_str);
    } else {
        std::cout << "no dice to roll" << std::endl;
    }
    
    std::cout
    << "----------------------------------------\n"
    << "total: " << result << std::endl;
    
    return 0;
}
