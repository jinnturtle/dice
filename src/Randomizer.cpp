#include "Randomizer.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <array>
#include <vector>
#include <iostream>
#include <functional>
#include <memory>

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
    sym_dice = 'd',
    sym_add = '+',
    sym_substr = '-',
    sym_mult = '*',
    sym_divis = '/'
};

// Creates Element_dice from dice element position in a string
Element_dice::Element_dice(const std::string& str, size_t pos)
: Element(et_dice)
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
        } else {
            break;
        }
    }
    
    this->n = amt;
    this->sides = sides;
}

Element_dice::Element_dice(size_t n, size_t sides)
: Element(et_dice)
, n{n}
, sides{sides}
{}

struct Element_op: public Element {
    Element_op(const char symb)
    : Element(et_op)
    {
        switch(symb){
        case sym_add:
            this->f = [](int a, int b) -> int {return a+b;};
            break;
        case sym_substr:
            this->f = [](int a, int b) -> int {return a-b;};
            break;
        case sym_mult:
            this->f = [](int a, int b) -> int {return a*b;};
            break;
        case sym_divis:
            this->f = [](int a, int b) -> int {return a/b;};
            break;
        }
    };
    
    std::function<int(int, int)> f;
};

struct Element_num: public Element {
    Element_num(int i)
    : Element(et_num), i{i}
    {};
    
    int i;
};

// TODO seems there is a bug whene there are spaces
auto Randomizer::roll_dice(const std::string& dice_str) -> int
{    
    std::vector<std::unique_ptr<Element>> elements;
    
    const char* ch {&dice_str.front()};
    const char* end {&dice_str.back()};
    long num1 {0};
    long num2 {0};
    while (ch <= end) {
        // skip spaces
//         if (*ch == ' ') {
//             ++ch;
//             continue;
//         }
        
        if (isdigit(*ch)) {
            char* num_end;
            num1 = strtol(ch, &num_end, 10);
            
            if (ch == num_end) { break; }
            ch = num_end;
        }
        
        if (*ch == sym_dice) {
            ++ch;
            
            char* num_end;
            num2 = strtol(ch, &num_end, 10);
            
            if (ch == num_end) { break; }
            ch = num_end;
            
            elements.push_back(
                std::unique_ptr<Element_dice>(new Element_dice(num1, num2)));
            
            std::cout << "add dice "
            << static_cast<Element_dice*>(elements.back().get())->n
            << "d"
            << static_cast<Element_dice*>(elements.back().get())->sides
            << std::endl;
        } else {
            elements.push_back(
                std::unique_ptr<Element_num>(new Element_num(num1)));
            
            std::cout << "add number '" << num1 << "'" << std::endl;
        }
        
        switch (*ch) {
        case sym_add:
        case sym_substr:
        case sym_mult:
        case sym_divis:
            elements.push_back(
                std::unique_ptr<Element_op>(new Element_op(*ch)));
            
            std::cout << "add operator '" << *ch << "'" << std::endl;
            
            ++ch;
            break;
        }
    }
    
    // resolve the dice
    std::cout << "*** rolling the dice ***" << std::endl;
    for (size_t i {0}; i < elements.size(); ++i) {
        if(elements[i]->type == et_dice) {
            Element_dice* dice = static_cast<Element_dice*>(elements[i].get());
            std::cout << i+1 << ": " << dice->n << "d" << dice->sides
            << std::endl;
            
            // roll the dice
            std::uniform_int_distribution<std::mt19937::result_type> dist(
                1, dice->sides);
            int total {0};
            for (size_t j {0}; j < dice->n; ++j) {
                int res = dist(this->rng);
                std::cout << "d#" << j+1 << ": " << res << std::endl;
                total += res;
            }
            std::cout << "total: " << total << "\n"
            << "----------------------------------------" << std::endl;
            
            // replacing diceroll with it's result, for next pass
            elements[i] = std::unique_ptr<Element_num>(new Element_num(total));
        }
    }
    
    int grand_total {0};
    
    // perform arithmetic operations
    Element_op* op {nullptr};
    for (size_t i {0}; i < elements.size(); ++i) {
        Element_type type = elements[i]->type;
        
        if(type == et_num) {
            num1 = static_cast<Element_num*>(elements[i].get())->i;
            
            if (op) {
                grand_total = op->f(grand_total, num1);
                op = nullptr;
            } else {
                grand_total += num1;
            }
        } else if (type == et_op) {
            op = static_cast<Element_op*>(elements[i].get());
        }
    }
    
    return grand_total;
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
