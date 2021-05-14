//
// compiler.hh
//
// Copyright (C) 2021 Jens Alfke. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once
#include "word.hh"
#include <optional>
#include <string>
#include <vector>


namespace tails {

    namespace core_words {
        extern const Word LITERAL;
    }


    /// A Forth word definition compiled at runtime.
    class CompiledWord : public Word {
    public:
        struct WordRef {
            WordRef(const Word &w)            :word(w), param((Op)0) {assert(!w.hasAnyParam());}
            WordRef(const Word &w, Instruction p):word(w), param(p) {assert(w.hasAnyParam());}
            WordRef(const Word &w, Value v)   :word(w), param(v) {assert(w.hasValParam());}
            WordRef(const Word &w, intptr_t o):word(w), param(o) {assert(w.hasIntParam());}

            WordRef(Value v)                  :WordRef(core_words::LITERAL, v) { }
            WordRef(double d)                  :WordRef(core_words::LITERAL, Value(d)) { }

            const Word& word;
            Instruction param;
        };

        /// Compiles Forth source code to an unnamed Word, but doesn't run it.
        static CompiledWord parse(const char *name = nullptr, bool allowParams =false);

        /// Creates a finished CompiledWord from a list of word references.
        CompiledWord(const char *name, std::initializer_list<WordRef> words);

        /// Creates a finished, anonymous CompiledWord from a list of word references.
        CompiledWord(std::initializer_list<WordRef> words)  :CompiledWord(nullptr, words) { }

        //---- Incrementally building words:

        /// Initializes a CompiledWord with a name (or none) but no instructions.
        /// \ref add and \ref finish need to be called before the word can be used.
        explicit CompiledWord(const char *name = nullptr);

        /// An opaque reference to an instruction written to a CompiledWord in progress.
        enum class InstructionPos : intptr_t { None = 0 };

        /// Adds an instruction to a word being compiled.
        /// @return  An opaque reference to this instruction, that can be used later to fix branches.
        InstructionPos add(const WordRef&);

        /// Returns the word at the given position.
        const WordRef& operator[] (InstructionPos);

        /// Returns an opaque reference to the _next_ instruction to be added,
        InstructionPos nextInstructionPos() const       {return InstructionPos(_instrs.size() + 1);}

        /// Updates the branch target of a previously-written `BRANCH` or `ZBRANCH` instruction.
        /// @param src  The branch instruction to update.
        /// @param dst  The instruction it should jump to.
        void fixBranch(InstructionPos src, InstructionPos dst);

        /// Finishes a word being compiled. Adds a RETURN instruction, and registers it with the
        /// global Vocabulary (unless it's unnamed.)
        void finish();

    private:
        using WordVec = std::vector<WordRef>;
        using EffectVec = std::vector<std::optional<StackEffect>>;

        void computeEffect();
        void computeEffect(intptr_t i,
                           StackEffect effect,
                           EffectVec &instrEffects,
                           std::optional<StackEffect> &finalEffect);

        std::string                 _nameStr;       // Backing store for inherited _name
        std::vector<Instruction>    _instrs {};     // Instructions; backing store for inherited _instr
        std::unique_ptr<WordVec>    _tempWords;     // used only during building, until `finish`
    };


    /// Looks up the word for an instruction and returns it as a WordRef.
    /// If the word is CALL, the next word (at `instr[1]`) is returned instead.
    /// If the word has a parameter (like LITERAL or BRANCH), it's read from `instr[1]`.
    std::optional<CompiledWord::WordRef> DisassembleInstruction(const Instruction *instr);

    /// Disassembles an entire interpreted word given its first instruction.
    std::vector<CompiledWord::WordRef> DisassembleWord(const Instruction *firstInstr);

}
