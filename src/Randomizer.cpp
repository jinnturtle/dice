#include "Randomizer.hpp"

#include <array>
#include <vector>
#include <iostream>
#include <cctype>
#include <cmath>

// TODO move to utils.cpp
auto mirror(size_t n) -> size_t {
    size_t res {0};
    while (n != 0) {
        size_t digit = n % 10;
        n /= 10;
        res *= 10;
        res += digit;
    }
    
    return res;
}

Randomizer::Randomizer()
: rng{std::mt19937(this->dev())}
{}

enum Symbol : char {
    dice = 'd',
    add = '+',
    substr = '-',
    mult = '*',
    divis = '/'
};

struct Dice {
    size_t n; // ammount of dice
    size_t sides; // how many sides each die has
};

// TODO organise, encapsulate
auto parse_dice_element(const std::string& str, size_t pos) -> Dice
{
    size_t amt {0};
    // reading backwards
    for (size_t j {1}; j <= pos; ++j) {
        const char* ch {&str[pos-j]};
        if (isdigit(*ch)) {
            amt = amt*10 + (*ch - '0');
        } else {
            break;
        }
    }
    amt = mirror(amt);
    if (amt == 0) { amt = 1; } // default to 1
    
    // find sides of dice
    size_t sides {0};
    for (size_t j {pos+1}; j < str.size(); ++j) {
        const char* ch = &str[j];
        if (isdigit(*ch)) {
            sides *= 10;
            sides += *ch - '0';
        }
    }
    
    return Dice{.n = amt, .sides = sides};
}

auto Randomizer::roll_dice(const std::string& dice_str) -> int
{    
    std::vector<Dice> dice_pool;
    
    for (size_t i {0}; i < dice_str.size(); ++i) {
        switch (dice_str[i]) {
        case dice:
            // find ammount of dice
            dice_pool.push_back(parse_dice_element(dice_str, i));
            
            break;
        }
    }
    
    std::cout << "*** dice pool ***" << std::endl;
    int sum {0};
    for (size_t i {0}; i < dice_pool.size(); ++i) {
        std::cout << i+1 << ": " << dice_pool[i].n << "d" << dice_pool[i].sides
        << std::endl;
        
        // roll the dice
        std::uniform_int_distribution<std::mt19937::result_type> dist(
            1, dice_pool[i].sides);
        for (size_t j {0}; j < dice_pool[i].n; ++j) {
            int res = dist(this->rng);
            std::cout << "d#" << j+1 << ": " << res << std::endl;
            sum += res;
        }
    }
    
    return sum;
}

auto Randomizer::roll_range(int min, int max) -> int
{
    // TODO implement a swap function that does not use an intermediary buffer
    if(min > max) {
        int sav_min = min;
        min = max;
        max = sav_min;
    }
    std::uniform_int_distribution<std::mt19937::result_type> dist(min, max);
    
    return dist(this->rng);
}
