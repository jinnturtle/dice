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


// Element is an elemenet in a roll command e.g. a group of dice, arithmetic operation
enum Element_type {
    et_undefined = 0,
    et_dice,    // dice element e.g. 3d10
    et_op,          // operator element e.g. +, -
    et_num          // number element e.g. 5
};

struct Element {
    Element(Element_type type)
    :type{type}
    {};
    
    Element_type type;
};


class Element_dice final: public Element{
public:
    Element_dice(const std::string& str, size_t pos);
    Element_dice(size_t dice_n, size_t sides_n);

    ~Element_dice(){};
    
    size_t n; // ammount of dice
    size_t sides; // how many sides each die has
};

#endif //ifndef RANDOMIZER_HPP
