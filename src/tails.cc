//
// tails.cc
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

#include "tails.hh"
#include "core_words.hh"
#include "vocabulary.hh"


/// Constructor for a native word.
Word::Word(const char *name, Op native, Flags flags)
:_name(name)
,_native(native)
,_flags(flags)
{
    gVocabulary.add(*this);
}


/// Constructor for an interpreted word.
Word::Word(const char *name, std::initializer_list<WordRef> words)
:_name(name)
{
    size_t count = 1;
    for (auto &ref : words)
        count += ref._count;
    _instrs.reset(new Instruction[count]);
    Instruction *dst = &_instrs[0];
    for (auto &ref : words) {
        std::copy(&ref._instrs[0], &ref._instrs[ref._count], dst);
        dst += ref._count;
    }
    *dst = RETURN._native;

    if (name)
        gVocabulary.add(*this);
}


WordRef::WordRef(const Word &word) {
    assert(!(word._flags & Word::HasIntParam));
    if (word._native) {
        _instrs[0] = word._native;
        _count = 1;
    } else {
        assert(!word._native);
        _instrs[0] = CALL._native;
        _instrs[1] = word._instrs.get();
    }
}


WordRef::WordRef(const Word &word, int param)
:_instrs{word._native, param}
{
    assert(word._native);
    assert(word._flags & Word::HasIntParam);
}


WordRef::WordRef(int i)
:WordRef{LITERAL, i}
{ }
