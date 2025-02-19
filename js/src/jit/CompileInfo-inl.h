/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 sts=2 et sw=2 tw=80:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jit_CompileInfo_inl_h
#define jit_CompileInfo_inl_h

#include "jit/CompileInfo.h"
#include "jit/JitAllocPolicy.h"

#include "vm/JSScript-inl.h"

namespace js {
namespace jit {

inline RegExpObject* CompileInfo::getRegExp(jsbytecode* pc) const {
  return script_->getRegExp(pc);
}

inline JSFunction* CompileInfo::getFunction(jsbytecode* pc) const {
  return script_->getFunction(pc);
}

InlineScriptTree* InlineScriptTree::New(TempAllocator* allocator,
                                        InlineScriptTree* callerTree,
                                        jsbytecode* callerPc,
                                        JSScript* script) {
  MOZ_ASSERT_IF(!callerTree, !callerPc);
  MOZ_ASSERT_IF(callerTree, callerTree->script()->containsPC(callerPc));

  // Allocate a new InlineScriptTree
  void* treeMem = allocator->allocate(sizeof(InlineScriptTree));
  if (!treeMem) {
    return nullptr;
  }

  // Initialize it.
  return new (treeMem) InlineScriptTree(callerTree, callerPc, script);
}

InlineScriptTree* InlineScriptTree::addCallee(TempAllocator* allocator,
                                              jsbytecode* callerPc,
                                              JSScript* calleeScript) {
  MOZ_ASSERT(script_ && script_->containsPC(callerPc));
  InlineScriptTree* calleeTree = New(allocator, this, callerPc, calleeScript);
  if (!calleeTree) {
    return nullptr;
  }

  calleeTree->nextCallee_ = children_;
  children_ = calleeTree;
  return calleeTree;
}

void InlineScriptTree::removeCallee(InlineScriptTree* callee) {
  InlineScriptTree** prevPtr = &children_;
  for (InlineScriptTree* child = children_; child; child = child->nextCallee_) {
    if (child == callee) {
      *prevPtr = child->nextCallee_;
      return;
    }
    prevPtr = &child->nextCallee_;
  }
  MOZ_CRASH("Callee not found");
}

static inline const char* AnalysisModeString(AnalysisMode mode) {
  switch (mode) {
    case Analysis_None:
      return "Analysis_None";
    case Analysis_DefiniteProperties:
      return "Analysis_DefiniteProperties";
    case Analysis_ArgumentsUsage:
      return "Analysis_ArgumentsUsage";
    default:
      MOZ_CRASH("Invalid AnalysisMode");
  }
}

}  // namespace jit
}  // namespace js

#endif /* jit_CompileInfo_inl_h */
