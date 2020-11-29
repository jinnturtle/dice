#ifndef RANDOMIZER_HPP
#define RANDOMIZER_HPP

#include <string>
#include <random>

class Randomizer final {
public:
    Randomizer();
    
    auto roll_dice(const std::string& dice_str) -> int;
    auto roll_range(int min, int max) -> int;
    
private:
    std::random_device dev;
    std::mt19937 rng;
};

#endif //ifndef RANDOMIZER_HPP
