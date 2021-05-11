//
// word.hh
//
// Copyright (C) 2020 Jens Alfke. All Rights Reserved.
//

#pragma once
#include "instruction.hh"
#include <stdexcept>


/// Describes the effect upon the stack of a word.
/// `in` is how many values it reads from the stack; `out` is how many it leaves behind.
/// Thus the net effect on stack depth is `out - in`.
class StackEffect {
public:
    constexpr StackEffect()
    :_in(0), _net(0), _max(0) { }

    constexpr StackEffect(uint8_t input, uint8_t output)
    :_in(input), _net(output - input), _max(std::max(input, output)) { }

    constexpr StackEffect(uint8_t input, uint8_t output, uint16_t max)
    :_in(input), _net(output - input), _max(max) { }

    constexpr int input() const     {return _in;}
    constexpr int output() const    {return _in + _net;}
    constexpr int net() const       {return _net;}
    constexpr int max() const       {return _max;}

    constexpr bool operator== (const StackEffect &other) const {
        return _in == other._in && _net == other._net && _max == other._max;
    }
    
    constexpr bool operator!= (const StackEffect &other) const {return !(*this == other);}

    /// Returns the cumulative effect of two StackEffects, first `this` and then `other`.
    /// This is complicated & confusing, since `other` gets offset by my `net`.
    constexpr StackEffect then(const StackEffect &other) const {
        int in = std::max(this->input(), other.input() - this->net());
        int net = this->net() + other.net();
        int max = in + std::max(this->max() - this->input(),
                                this->net() + other.max() - other.input());
        StackEffect result {uint8_t(in), uint8_t(in + net), uint16_t(max)};
        if (result._in != in || result._net != net || result._max != max)
            throw std::runtime_error("StackEffect overflow");
        return result;
    }

    /// Returns true if `merge` is legal, i.e. the two have the same net stack effect.
    bool canMerge(const StackEffect &other) const       {return this->net() == other.net();}

    /// Returns the effect of doing either `this` or `other` (which must have the same net.)
    constexpr StackEffect merge(const StackEffect &other) const {
        assert(canMerge(other));
        return (this->input() >= other.input()) ? *this : other;
    }

private:
    uint8_t  _in;       // Minimum stack depth on entry
    int8_t   _net;      // Change in stack depth on exit
    uint16_t _max;      // Maximum stack depth (relative to `_in`) while running
};


/// A Forth word definition: name, flags and code.
/// This base class itself is used for predefined words.
struct Word {
    enum Flags : uint8_t {
        None = 0,
        Native = 1,
        HasIntParam = 2 ///< This word is followed by an int parameter (LITERAL, BRANCH, 0BRANCH)
    };

    constexpr Word(const char *name, Op native, StackEffect effect, Flags flags =None)
    :_instr(native)
    ,_name(name)
    ,_effect(effect)
    ,_flags(Flags(flags | Native))
    { }

    constexpr Word(const char *name, StackEffect effect, const Instruction words[])
    :_instr(words)
    ,_name(name)
    ,_effect(effect)
    ,_flags(None)
    { }

    constexpr operator Instruction() const        {return _instr;}

    constexpr bool isNative() const     {return (_flags & Native) != 0;}
    constexpr bool hasParam() const     {return (_flags & HasIntParam) != 0;}
    constexpr StackEffect stackEffect() const   {return _effect;}

    Instruction _instr; // Instruction that calls it (either an Op or an Instruction*)
    const char* _name;  // Forth name, or NULL if anonymous
    StackEffect _effect;
    Flags       _flags; // Flags (see above)

protected:
    Word() :_instr {}, _name(nullptr), _effect(0, 0), _flags(None) { };
};


inline bool operator==(const Word &a, const Word &b)   {return a._instr.native == b._instr.native;}
inline bool operator!=(const Word &a, const Word &b)   {return !(a == b);}


// Shortcut for defining a native word (see examples in core_words.cc)
#define NATIVE_WORD(NAME, FORTHNAME, EFFECT, FLAGS) \
    extern "C" int* f_##NAME(int *sp, const Instruction *pc) noexcept; \
    constexpr Word NAME(FORTHNAME, f_##NAME, EFFECT, FLAGS); \
    extern "C" int* f_##NAME(int *sp, const Instruction *pc) noexcept


// Shortcut for defining a native word implementing a binary operator like `+` or `==`
#define BINARY_OP_WORD(NAME, FORTHNAME, INFIXOP) \
    NATIVE_WORD(NAME, FORTHNAME, StackEffect(2,1), Word::None) { \
        sp[1] = sp[1] INFIXOP sp[0];\
        ++sp;\
        NEXT(); \
    }


// Shortcut for defining an interpreted word (see examples in core_words.cc)
#define INTERP_WORD(NAME, FORTHNAME, EFFECT, ...) \
    static constexpr Instruction const i_##NAME[] { __VA_ARGS__, RETURN }; \
    constexpr Word NAME(FORTHNAME, EFFECT, i_##NAME)


