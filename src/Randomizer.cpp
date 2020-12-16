#include "Randomizer.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <array>
#include <vector>
#include <iostream>
#include <functional>
#include <memory>
#include <sstream>

struct Element;

using Elements_t = std::vector<std::unique_ptr<Element>>;

/* TODO Encapsulating Element class and children and related functions might be
 * a good idea. */

// parses the dice notation string and returns array of operable elements
auto parse_dice_str(const std::string& dice_str) -> Elements_t;
// resolves the dice elements and replaces them with resulting random value
auto resolve_dice(Elements_t* const elements, std::mt19937* const rng) -> void;
/* Performs arithmetic operation with elements and returns result, the only
 * element types supported by this operation are numbers and arithmetic
 * operators.*/
auto get_total(Elements_t* const elements) -> int64_t;

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

enum Element_type {
    et_undefined = 0,
    et_dice,        // dice element e.g. 3d10
    et_op,          // operator element e.g. +, -
    et_num          // number element e.g. 5
};

// Element is an elemenet in a roll command e.g. a group of dice, arithmetic operation
struct Element {
    Element(Element_type type)
    :type{type}
    {};
    
    Element_type type;
};

class Element_dice final: public Element {
public:
    Element_dice(const std::string& str, size_t pos);
    Element_dice(size_t dice_n, size_t sides_n);

    ~Element_dice(){};
    
    size_t n; // ammount of dice
    size_t sides; // how many sides each die has
};

Element_dice::Element_dice(size_t n, size_t sides)
: Element(et_dice)
, n{n}
, sides{sides}
{}

struct Element_op: public Element {
    Element_op(const char symb)
    : Element(et_op)
    , symb{symb}
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
    char symb; // the char symbol that represents the operator (e.g. '+', '-')
};

struct Element_num: public Element {
    Element_num(int i)
    : Element(et_num), i{i}
    {};
    
    int i;
};

auto Randomizer::roll_dice(const std::string& dice_str) -> int64_t
{
    // TODO implement some error detection
    Elements_t elements = parse_dice_str(dice_str);
    resolve_dice(&elements, &this->rng);
    int64_t grand_total { get_total(&elements) };   
    
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

auto parse_dice_str(const std::string& dice_str) -> Elements_t
{
    Elements_t elements;

    const char* ch {&dice_str.front()};
    const char* end {&dice_str.back()};
    long num1 {0};
    long num2 {0};
    while (ch <= end) {
        // skip spaces
        if (*ch == ' ') {
            ++ch;
            continue;
        }
        
        // just read the number if it's a digit
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
            
            if (num1 == 0) { num1 = 1; } // default to 1
            elements.push_back(
                std::unique_ptr<Element_dice>(new Element_dice(num1, num2)));
        } else {
            elements.push_back(
                std::unique_ptr<Element_num>(new Element_num(num1)));
        }
        
        switch (*ch) {
        case sym_add:
        case sym_substr:
        case sym_mult:
        case sym_divis:
            elements.push_back(
                std::unique_ptr<Element_op>(new Element_op(*ch)));
            ++ch;
            break;
        }
    }

    return std::move(elements);
}

auto resolve_dice(Elements_t* const elements, std::mt19937* const rng) -> void
{
    if (elements == nullptr || rng == nullptr) {
        std::cerr << "ERROR: null arguments to " << __func__ << " function"
        << std::endl;
        return;
    }

    for (size_t i {0}; i < (*elements).size(); ++i) {
        if((*elements)[i]->type == et_dice) {
            std::stringstream buf;
            Element_dice* dice =
                static_cast<Element_dice*>((*elements)[i].get());
            
            // roll the dice
            std::uniform_int_distribution<std::mt19937::result_type> dist(
                1, dice->sides);
            int total {0};
            for (size_t j {0}; j < dice->n; ++j) {
                int res = dist(*rng);
                total += res;

                buf << res;
                if (j < dice->n - 1) { buf << " "; } // don't put trailing space
            }

            std::cout << dice->n << "d" << dice->sides << " = " << total
            << " (" << buf.str() << ")" << std::endl;
            
            // replacing diceroll element with result numeric value
            (*elements)[i] = 
                std::unique_ptr<Element_num>(new Element_num(total));
        }
    }
}

/* TODO Here multiplication and division does not adhere to the correct
 * mathematical precedence, consider removing as these operation are not all
 * that common in tabletop games. */
auto get_total(Elements_t* const elements) -> int64_t
{
    int64_t res {0};

    Element_op* op {nullptr};
    int buf {0};
    std::stringstream ss;
    for (size_t i {0}; i < (*elements).size(); ++i) {
        Element_type type = (*elements)[i]->type;
        
        if(type == et_num) {
            buf = static_cast<Element_num*>((*elements)[i].get())->i;
            ss << buf;
            
            if (op) {
                res = op->f(res, buf);
                op = nullptr;
            } else {
                res += buf;
            }
        } else if (type == et_op) {
            op = static_cast<Element_op*>((*elements)[i].get());
            ss << op->symb;
        } else {
            std::cerr << "ERROR: function " << __func__
            << " does not support element type " << type << " at this moment"
            << std::endl;
        }

        ss << " "; // putting space between elements for readability

        // print string if last element
        if (i+1 >= (*elements).size()) {
            ss << "= " << res;
            std::cout << ss.str() << std::endl;
        }
    }

    return res;
}