/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 sts=2 et sw=2 tw=80:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef frontend_FoldConstants_h
#define frontend_FoldConstants_h

#include "frontend/SyntaxParseHandler.h"

namespace js {
namespace frontend {

struct CompilationInfo;

class FullParseHandler;
template <class ParseHandler>
class PerHandlerParser;

// Perform constant folding on the given AST. For example, the program
// `print(2 + 2)` would become `print(4)`.
//
// pnp is the address of a pointer variable that points to the root node of the
// AST. On success, *pnp points to the root node of the new tree, which may be
// the same node (unchanged or modified in place) or a new node.
//
// Usage:
//    pn = parser->statement();
//    if (!pn) {
//        return false;
//    }
//    if (!FoldConstants(cx, &pn, parser)) {
//        return false;
//    }
extern MOZ_MUST_USE bool FoldConstants(JSContext* cx,
                                       CompilationInfo& compilationInfo,
                                       ParseNode** pnp,
                                       FullParseHandler* handler);

inline MOZ_MUST_USE bool FoldConstants(JSContext* cx,
                                       CompilationInfo& compilationInfo,
                                       typename SyntaxParseHandler::Node* pnp,
                                       SyntaxParseHandler* handler) {
  return true;
}

} /* namespace frontend */
} /* namespace js */

#endif /* frontend_FoldConstants_h */
