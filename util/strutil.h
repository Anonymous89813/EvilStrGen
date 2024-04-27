//
// Created by hhhhuang on 7/13/23.
//

#ifndef RE2_CLION_STRUTIL_H
#define RE2_CLION_STRUTIL_H

#endif //RE2_CLION_STRUTIL_H
// Copyright 2016 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_STRUTIL_H_
#define UTIL_STRUTIL_H_

#include <string>

#include "src/stringpiece.h"
#include "util/util.h"

namespace re2 {

    std::string CEscape(const StringPiece& src);
    void PrefixSuccessor(std::string* prefix);
    std::string StringPrintf(const char* format, ...);

}  // namespace src

#endif  // UTIL_STRUTIL_H_
