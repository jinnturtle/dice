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

// parses the dice notation string and returns array of operable elements
auto parse_dice_str(const std::string& dice_str) -> Elements_t;
// resolves the dice elements and replaces them with resulting random value
auto resolve_dice(Elements_t* const elements, std::mt19937* const rng) -> void;
/* Performs arithmetic operation with elements and returns result, the only
 * element types supported by this operation are numbers and arithmetic
 * operators.*/
auto get_total(Elements_t* const elements) -> int64_t;
// roll n amount of sided dice and return result total
auto roll_dice(int n, int sides,
    std::mt19937* const rng, std::stringstream& buf) -> int;

Randomizer::Randomizer()
: rng{std::mt19937(this->dev())}
{}

enum Symbol : char {
    sym_dice = 'd',
    sym_mult = '*',
    sym_divis = '/',
    sym_add = '+',
    sym_substr = '-'
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
    int64_t grand_total { get_total(&elements) };

    return grand_total;
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
        case sym_divis:
        case sym_add:
        case sym_substr:
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

auto roll_dice(
    int n,
    int sides,
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

/* TODO Here multiplication and division does not adhere to the correct
 * mathematical precedence, consider removing as these operation are not all
 * that common in tabletop games. */
auto get_total(Elements_t* const elements) -> int64_t
{
    int64_t res {0};

    Element_op start_op('+');
    Element_op* op {&start_op};
    int buf {0};
    std::stringstream ss;
    for (auto it {elements->begin()}; it != elements->end(); it++) {
        Element_type type = (*it)->type;

        if(type == et_num) {
            buf = static_cast<Element_num*>(it->get())->i;
            ss << buf;

            if (op) {
                switch(op->symb) {
                    case sym_add: res += buf; break;
                    case sym_substr: res -= buf; break;
                    case sym_mult: res *= buf; break;
                    case sym_divis: res /= buf; break;
                    default:
                        std::cerr << "ERROR: op " << op->symb
                        << " not supported in " << __func__ << std::endl;
                        return res;
                }
                op = nullptr;
            } else {
                std::cerr << "ERROR: operator should precede operand"
                << std::endl;
                return res;
            }
        } else if (type == et_op) {
            op = static_cast<Element_op*>(it->get());
            ss << op->symb;
        } else {
            std::cerr << "ERROR: function " << __func__
            << " does not support element type " << type << " at this moment"
            << std::endl;
        }

        ss << " "; // putting space between elements for readability

        // print string if last element
        if (std::next(it) == elements->end()) {
            ss << "= " << res;
            std::cout << ss.str() << std::endl;
        }
    }

    return res;
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
