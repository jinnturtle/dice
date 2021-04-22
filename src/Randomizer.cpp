#include "Randomizer.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <array>
#include <list>
#include <iterator>
#include <iostream>
#include <functional>
#include <memory>
#include <sstream>
#include <tuple>

struct Element;

using Elements_t = std::list<std::unique_ptr<Element>>;

/* TODO Encapsulating Element class and children and related functions might be
 * a good idea. */

auto get_surrounding_nums(Elements_t* const list, Elements_t::iterator it)
    -> std::tuple<Element*, Element*>;
auto print_elements(Elements_t* const elements) -> std::stringstream;

// parses the dice notation string and returns array of operable elements
auto parse_dice_str(const std::string& dice_str) -> Elements_t;
// resolves the dice operator and replaces them with resulting random value
auto resolve_dice(Elements_t* const elements, std::mt19937* const rng) -> void;
// resolves the division and multiplicaiton, and replaces them with result vals
auto resolve_divmult(Elements_t* const elements) -> void;
// resolves the addition and subtraction ops, and replaces them with result vals
auto resolve_addsub(Elements_t* const elements) -> void;
/* Performs arithmetic operation with elements and returns result, the only
 * element types supported by this operation are numbers and arithmetic
 * operators.*/
auto get_total(Elements_t* const elements) -> int64_t;
// roll n amount of sided dice and return result total
auto roll_dice(unsigned n, unsigned sides,
    std::mt19937* const rng, std::stringstream& buf) -> int;

Randomizer::Randomizer()
: rng{std::mt19937(this->dev())}
{}

enum Symbol : char {
    sym_dice = 'd',
    sym_mult = '*',
    sym_div = '/',
    sym_add = '+',
    sym_sub = '-'
};

enum Element_type {
    et_undefined = 0,
    et_op,          // operator element e.g. +, -
    et_num          // number element e.g. 5
};

// Element is an elemenet in a roll command e.g. a group of dice, arithmetic operation
struct Element {
    Element(Element_type type)
    :type{type}
    {};

    std::string get_type_str();

    Element_type type;
};

std::string Element::get_type_str() {
    switch(this->type) {
        case et_num:
            return "number"; // TODO "integer value" would prob make more sense
            break;
        case et_op:
            return "arithmetic operator";
            break;
        case et_undefined:
            return "undefined";
            break;
        default:
            return "unknown";
            break;
    }
}

struct Element_op: public Element {
    Element_op(const char symb)
    : Element(et_op)
    , symb{symb}
    {}

    char symb; // the char symbol that represents the operator (e.g. '+', '-')
};

struct Element_num: public Element {
    Element_num(int i)
    : Element(et_num), i{i}
    {};

    int i;
};

auto Randomizer::process(const std::string& dice_str) -> int64_t
{
    // TODO implement some error detection
    Elements_t elements = parse_dice_str(dice_str);
    resolve_dice(&elements, &this->rng);
    resolve_divmult(&elements);
    resolve_addsub(&elements);

    if (elements.size() != 1) {
        std::cerr << "ERROR: sequence not fully processed, " << elements.size()
        << " elements remaining" << std::endl;
        return 0;
    }

    return static_cast<Element_num*>(elements.front().get())->i;
}

auto Randomizer::roll_range(int min, int max) -> int
{
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
    long num {0};
    while (ch <= end) {
        // skip spaces
        if (*ch == ' ') {
            ++ch;
            continue;
        }

        // just read the number if it's a digit
        if (isdigit(*ch)) {
            char* num_end;
            num = strtol(ch, &num_end, 10);

            if (ch == num_end) { break; }
            ch = num_end;

            elements.push_back(
                std::unique_ptr<Element_num>(new Element_num(num)));
            continue;
        }

        switch (*ch) {
        case sym_dice:
        case sym_mult:
        case sym_div:
        case sym_add:
        case sym_sub:
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
    if (!elements || !rng) {
        std::cerr << "ERROR: null arguments to " << __func__ << " function"
        << std::endl;
        return;
    }

    for (auto it {(*elements).begin()}; it != (*elements).end(); ++it) {
        if((*it)->type != et_op) { continue; }

        Element_op* op =
            static_cast<Element_op*>((*it).get());
        if (op->symb != sym_dice) { continue; }

        // dice operator must have two integer elements surrounding it
        auto [prev_el, next_el] = get_surrounding_nums(elements, it);
        if (!prev_el || !next_el) {
            std::cerr << "ERROR: dice operator must be surrounded by ints"
            << std::endl;
            return;
        }

        int amt {static_cast<Element_num*>(prev_el)->i};
        int sides {static_cast<Element_num*>(next_el)->i};

        // roll the dice
        std::stringstream buf;
        int total {roll_dice(amt, sides, rng, buf)};

        std::cout << amt << "d" << sides << " = " << total
        << " (" << buf.str() << ")" << std::endl;

        // replacing dice operator and operands with result numeric value
        elements->erase(std::prev(it));
        (*it) = std::unique_ptr<Element_num>(new Element_num(total));
        elements->erase(std::next(it));
    }
}

auto resolve_divmult(Elements_t *const elements) -> void
{
    if (!elements) {
        std::cerr << "ERROR: null arguments to " << __func__ << " function"
        << std::endl;
        return;
    }

    for (auto it {(*elements).begin()}; it != (*elements).end(); ++it) {
        if((*it)->type != et_op) { continue; }

        Element_op* op =
            static_cast<Element_op*>((*it).get());
        if (op->symb != sym_div && op->symb != sym_mult) { continue; }

        // operator must have two integer elements surrounding it
        auto [prev_el, next_el] = get_surrounding_nums(elements, it);
        if (!prev_el || !next_el) {
            std::cerr << "ERROR: arith. operators must be surrounded by ints"
            << std::endl;
            return;
        }

        int a {static_cast<Element_num*>(prev_el)->i};
        int b {static_cast<Element_num*>(next_el)->i};
        int res {0};

        if (op->symb == sym_div) {
            res = a/b;
        } else if (op->symb == sym_mult) {
            res = a*b;
        } else {
            std::cerr << "ERROR: unexpected operator at " << __func__
            << std::endl;
        }

        // the operand and operators can be replaced by result now
        (*it) = std::unique_ptr<Element_num>(new Element_num(res));
        elements->erase(std::prev(it));
        elements->erase(std::next(it));
    }
}

auto resolve_addsub(Elements_t *const elements) -> void
{
    if (!elements) {
        std::cerr << "ERROR: null arguments to " << __func__ << " function"
        << std::endl;
        return;
    }

    for (auto it {(*elements).begin()}; it != (*elements).end(); ++it) {
        if((*it)->type != et_op) { continue; }

        Element_op* op =
            static_cast<Element_op*>((*it).get());
        if (op->symb != sym_add && op->symb != sym_sub) { continue; }

        // operator must have two integer elements surrounding it
        auto [prev_el, next_el] = get_surrounding_nums(elements, it);
        if (!prev_el || !next_el) {
            std::cerr << "ERROR: arith. operators must be surrounded by ints"
            << std::endl;
            return;
        }

        int a {static_cast<Element_num*>(prev_el)->i};
        int b {static_cast<Element_num*>(next_el)->i};

        // replacing dice operator and operands with result numeric value
        if (op->symb == sym_add) {
            (*it) = std::unique_ptr<Element_num>(new Element_num(a+b));
        } else if (op->symb == sym_sub) {
            (*it) = std::unique_ptr<Element_num>(new Element_num(a-b));
        } else {
            std::cerr << "ERROR: unexpected operator at " << __func__
            << std::endl;
        }
        elements->erase(std::prev(it));
        elements->erase(std::next(it));
    }
}

auto roll_dice(
    unsigned n,
    unsigned sides,
    std::mt19937* const rng,
    std::stringstream& buf) -> int
{
    std::uniform_int_distribution<std::mt19937::result_type> dist(1, sides);

    int total {0};
    for (size_t j {0}; j < n; ++j) {
        int res = dist(*rng);
        total += res;

        buf << res;
        if (j < n - 1) { buf << " "; } // don't put trailing space
    }

    return total;
}

auto get_surrounding_nums(Elements_t* const list, Elements_t::iterator it)
    -> std::tuple<Element*, Element*>
{
    if (it == list->begin() || std::next(it) == list->end()) {
        return {nullptr, nullptr};
    }

    Element* prev_el {std::prev(it)->get()};
    Element* next_el {std::next(it)->get()};
    if (prev_el->type != et_num || next_el->type != et_num) {
        return {nullptr, nullptr};
    }

    return {std::prev(it)->get(), std::next(it)->get()};
}

auto print_elements(Elements_t* const elements) -> std::stringstream
{
    std::stringstream buf;

    for (auto it {elements->begin()}; it != elements->end(); ++it) {
        Element* e {it->get()};
        switch (e->type) {
            case et_num:
                buf << static_cast<Element_num*>(e)->i;
                break;
            case et_op:
                buf << static_cast<Element_op*>(e)->symb;
                break;
            case et_undefined:
                buf << "[undefined]";
                break;
            default:
                buf << "?";
                break;
        }

        if (std::next(it) != elements->end()) {
            buf << " "; // separating from next element
        }
    }

    return buf;
}
