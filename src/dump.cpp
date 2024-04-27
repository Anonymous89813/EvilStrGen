//
// Created by 黄鸿 on 2023/7/19.
//

// Copyright 2006 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Dump the regexp into a string showing structure.
// Tested by parse_unittest.cc

// This function traverses the regexp recursively,
// meaning that on inputs like Regexp::Simplify of
// a{100}{100}{100}{100}{100}{100}{100}{100}{100}{100},
// it takes time and space exponential in the size of the
// original regular expression.  It can also use stack space
// linear in the size of the regular expression for inputs
// like ((((((((((((((((a*)*)*)*)*)*)*)*)*)*)*)*)*)*)*)*)*.
// IT IS NOT SAFE TO CALL FROM PRODUCTION CODE.
// As a result, Dump is provided only in the testing
// library (see BUILD).

#include "iostream"
#include <string>
#include "deque"
#include "util/test.h"
#include "util/logging.h"
#include "util/strutil.h"
#include "util/utf.h"
#include "src/stringpiece.h"
#include "src/regexp.h"
#include "vector"
namespace re2 {

    static const char* kOpcodeNames[] = {
            "bad",
            "no",
            "emp",
            "lit",
            "str",
            "cat",
            "alt",
            "star",
            "plus",
            "que",
            "rep",
            "cap",
            "dot",
            "byte",
            "bol",
            "eol",
            "wb",   // kRegexpWordBoundary
            "nwb",  // kRegexpNoWordBoundary
            "bot",
            "eot",
            "cc",
            "match",
    };


    static std::vector<std::string> All_Sub_Regex(Regexp* re) {
        std::vector<std::string> sub_regex_set;
        sub_regex_set.emplace_back("");
        if (re->op() < 0 || re->op() >= arraysize(kOpcodeNames)) {
            std::cout << "unknown option" << std::endl;
            exit(100);
        } else {
            switch (re->op()) {
                default:
                    break;
                case kRegexpStar:
                case kRegexpPlus:
                case kRegexpQuest:
                case kRegexpRepeat:
                    if (re->parse_flags() & Regexp::NonGreedy)
                        //s->append("n");
                        break;
            }
            //s->append(kOpcodeNames[re->op()]);
            if (re->op() == kRegexpLiteral && (re->parse_flags() & Regexp::FoldCase)) {
                Rune r = re->rune();
                if ('a' <= r && r <= 'z');
                //s->append("fold");
            }
            if (re->op() == kRegexpLiteralString && (re->parse_flags() & Regexp::FoldCase)) {
                for (int i = 0; i < re->nrunes(); i++) {
                    Rune r = re->runes()[i];
                    if ('a' <= r && r <= 'z') {
                        //s->append("fold");
                        break;
                    }
                }
            }
        }
        //s->append("{");
        switch (re->op()) {
            default:
                break;
            case kRegexpEndText:
                if (!(re->parse_flags() & Regexp::WasDollar)) {
                    //s->append("\\z");
                }
                break;
            case kRegexpLiteral: {
                Rune r = re->rune();
                char buf[UTFmax + 1];
                buf[runetochar(buf, &r)] = 0;
                for (auto string = sub_regex_set.begin(); string != sub_regex_set.end(); string++) {
                    *string += buf;
                }
                break;
            }
            case kRegexpLiteralString:{
                for (int i = 0; i < re->nrunes(); i++) {
                    Rune r = re->runes()[i];
                    char buf[UTFmax + 1];
                    buf[runetochar(buf, &r)] = 0;
                    for (auto string = sub_regex_set.begin(); string != sub_regex_set.end(); string++) {
                        *string += buf;
                    }
                }
                break;
            }
            case kRegexpConcat:{
                for (int i = 0; i < re->nsub(); i++) {
                    auto sub_regex_vec = All_Sub_Regex(re->sub()[i]);
                    std::vector<std::string> mid_vec;
                    for (auto it = sub_regex_set.begin(); it != sub_regex_set.end(); it++) {
                        for (auto it_vec = sub_regex_vec.begin(); it_vec != sub_regex_vec.end(); it_vec++)
                            mid_vec.push_back(*it + *it_vec);
                    }
                    sub_regex_set = mid_vec;
                }
                break;
            }

            case kRegexpAlternate: {
                std::vector<std::string> Alternate_Sub_set;
                for (int i = 0; i < re->nsub(); i++) {
                    auto sub_regex_vec = All_Sub_Regex(re->sub()[i]);
                    for (auto it = sub_regex_set.begin(); it != sub_regex_set.end(); it++) {
                        for (auto it_vec = sub_regex_vec.begin(); it_vec != sub_regex_vec.end(); it_vec++) {
                            Alternate_Sub_set.push_back(*it + *it_vec);
                        }
                    }
                }
                sub_regex_set = Alternate_Sub_set;
                break;
            }
            case kRegexpStar: {
                std::string Star_Sub_string;
                std::vector<std::string> mid_vec;
                auto sub_regex_vec = All_Sub_Regex(re->sub()[0]);
                for (auto &it_vec: sub_regex_vec)
                    Star_Sub_string.append(it_vec + "|");
                Star_Sub_string.pop_back();
                Star_Sub_string = "(?:" + Star_Sub_string + ")*";
                for (auto it = sub_regex_set.begin(); it != sub_regex_set.end(); it++) {
                    for (auto &it_vec: sub_regex_vec) {
                        mid_vec.emplace_back(*it += (Star_Sub_string + it_vec));
                    }
                }
                sub_regex_set = mid_vec;
                break;
            }
            case kRegexpPlus: {
                std::string Star_Sub_string;
                std::vector<std::string> mid_vec;
                auto sub_regex_vec = All_Sub_Regex(re->sub()[0]);
                for (auto &it_vec: sub_regex_vec)
                    Star_Sub_string.append(it_vec + "|");
                Star_Sub_string.pop_back();
                Star_Sub_string = "(?:" + Star_Sub_string + ")+";
                for (auto it = sub_regex_set.begin(); it != sub_regex_set.end(); it++) {
                    for (auto &it_vec: sub_regex_vec) {
                        mid_vec.emplace_back(*it += (Star_Sub_string + it_vec));
                    }
                }
                sub_regex_set = mid_vec;
                break;
            }
            case kRegexpQuest: {
                auto sub_regex_vec = All_Sub_Regex(re->sub()[0]);
                std::vector<std::string> mid_vec;
                for (auto it = sub_regex_set.begin(); it != sub_regex_set.end(); it++) {
                    for (auto &it_vec: sub_regex_vec)
                        mid_vec.push_back(*it + it_vec);
                }
                for (auto &it_vec : mid_vec)
                    sub_regex_set.push_back(it_vec);
                break;
            }
            case kRegexpCapture: {
                auto sub_regex_vec = All_Sub_Regex(re->sub()[0]);
                std::vector<std::string> mid_vec;
                for (auto it = sub_regex_set.begin(); it != sub_regex_set.end(); it++) {
                    for (auto &it_vec: sub_regex_vec)
                        mid_vec.push_back(*it + it_vec);
                }
                sub_regex_set = mid_vec;
                break;
            }
            case kRegexpRepeat: {
                std::string Repeat_Sub_set;
                std::vector<std::string> mid_vec;
                auto sub_regex_vec = All_Sub_Regex(re->sub()[0]);
                for (auto &it_vec: sub_regex_vec) {
                    Repeat_Sub_set.append(it_vec + '|');
                }
                Repeat_Sub_set.pop_back();
                Repeat_Sub_set = "(?:" + Repeat_Sub_set + ")" + "{" + std::to_string(re->max()) + "}";
                for (auto it = sub_regex_set.begin(); it != sub_regex_set.end(); it++) {
                    for (auto &it_vec: sub_regex_vec)
                        mid_vec.push_back(*it + Repeat_Sub_set + it_vec);
                }
                sub_regex_set = mid_vec;
                break;
            }
            case kRegexpCharClass: {
                std::string sep;
                sep.append("[");
                for (CharClass::iterator it = re->cc()->begin();
                     it != re->cc()->end(); ++it) {
                    RuneRange rr = *it;
                    if (rr.lo == rr.hi)
                        sep.append("\\x{" + StringPrintf("%x", rr.lo) + "}");
                    else
                        sep.append("\\x{" + StringPrintf("%x", rr.lo) + "}-\\x{" + StringPrintf("%x", rr.hi) + "}");
                }
                sep.append("]");
                for (auto &it : sub_regex_set){
                    it.append(sep);
                }
                break;
            }
        }
        return sub_regex_set;
    }




// Create string representation of regexp with explicit structure.
// Nothing pretty, just for testing.
    static void DumpRegexpAppending(Regexp* re, std::string* s) {
        if (re->op() < 0 || re->op() >= arraysize(kOpcodeNames)) {
            *s += StringPrintf("op%d", re->op());
        } else {
            switch (re->op()) {
                default:
                    break;
                case kRegexpStar:
                case kRegexpPlus:
                case kRegexpQuest:
                case kRegexpRepeat:
                    if (re->parse_flags() & Regexp::NonGreedy)
                        s->append("n");
                    break;
            }
            s->append(kOpcodeNames[re->op()]);
            if (re->op() == kRegexpLiteral && (re->parse_flags() & Regexp::FoldCase)) {
                Rune r = re->rune();
                if ('a' <= r && r <= 'z')
                    s->append("fold");
            }
            if (re->op() == kRegexpLiteralString && (re->parse_flags() & Regexp::FoldCase)) {
                for (int i = 0; i < re->nrunes(); i++) {
                    Rune r = re->runes()[i];
                    if ('a' <= r && r <= 'z') {
                        s->append("fold");
                        break;
                    }
                }
            }
        }
        s->append("{");
        switch (re->op()) {
            default:
                break;
            case kRegexpEndText:
                if (!(re->parse_flags() & Regexp::WasDollar)) {
                    s->append("\\z");
                }
                break;
            case kRegexpLiteral: {
                Rune r = re->rune();
                char buf[UTFmax+1];
                buf[runetochar(buf, &r)] = 0;
                s->append(buf);
                break;
            }
            case kRegexpLiteralString:
                for (int i = 0; i < re->nrunes(); i++) {
                    Rune r = re->runes()[i];
                    char buf[UTFmax+1];
                    buf[runetochar(buf, &r)] = 0;
                    s->append(buf);
                }
                break;
            case kRegexpConcat:
            case kRegexpAlternate:
                for (int i = 0; i < re->nsub(); i++)
                    DumpRegexpAppending(re->sub()[i], s);
                break;
            case kRegexpStar:
            case kRegexpPlus:
            case kRegexpQuest:
                DumpRegexpAppending(re->sub()[0], s);
                break;
            case kRegexpCapture:
                if (re->cap() == 0)
                    LOG(DFATAL) << "kRegexpCapture cap() == 0";
                if (re->name()) {
                    s->append(*re->name());
                    s->append(":");
                }
                DumpRegexpAppending(re->sub()[0], s);
                break;
            case kRegexpRepeat:
                s->append(StringPrintf("%d,%d ", re->min(), re->max()));
                DumpRegexpAppending(re->sub()[0], s);
                break;
            case kRegexpCharClass: {
                std::string sep;
                for (CharClass::iterator it = re->cc()->begin();
                     it != re->cc()->end(); ++it) {
                    RuneRange rr = *it;
                    s->append(sep);
                    if (rr.lo == rr.hi)
                        s->append(StringPrintf("%#x", rr.lo));
                    else
                        s->append(StringPrintf("%#x-%#x", rr.lo, rr.hi));
                    sep = " ";
                }
                break;
            }
        }
        s->append("}");
    }




    std::string Regexp::Dump() {
        // Make sure that we are being called from a unit test.
        // Should cause a link error if used outside of testing.
        CHECK(!::testing::TempDir().empty());

        std::string s;
        DumpRegexpAppending(this, &s);
        return s;
    }


    std::vector<std::string> Regexp::AllSubString() {
        // Make sure that we are being called from a unit test.
        // Should cause a link error if used outside of testing.

        return All_Sub_Regex(this);
    }
}  // namespace src
