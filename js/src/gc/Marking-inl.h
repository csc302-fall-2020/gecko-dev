/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 sts=2 et sw=2 tw=80:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef gc_Marking_inl_h
#define gc_Marking_inl_h

#include "gc/Marking.h"

#include "mozilla/Maybe.h"

#include <type_traits>

#include "gc/RelocationOverlay.h"
#include "vm/BigIntType.h"
#include "vm/RegExpShared.h"

#include "gc/Nursery-inl.h"

namespace js {
namespace gc {

// An abstraction to re-wrap any kind of typed pointer back to the tagged
// pointer it came from with |TaggedPtr<TargetType>::wrap(sourcePtr)|.
template <typename T>
struct TaggedPtr {};

template <>
struct TaggedPtr<JS::Value> {
  static JS::Value wrap(JSObject* obj) { return JS::ObjectOrNullValue(obj); }
  static JS::Value wrap(JSString* str) { return JS::StringValue(str); }
  static JS::Value wrap(JS::Symbol* sym) { return JS::SymbolValue(sym); }
  static JS::Value wrap(JS::BigInt* bi) { return JS::BigIntValue(bi); }
  template <typename T>
  static JS::Value wrap(T* priv) {
    static_assert(std::is_base_of_v<Cell, T>,
                  "Type must be a GC thing derived from js::gc::Cell");
    return JS::PrivateGCThingValue(priv);
  }
  static JS::Value empty() { return JS::UndefinedValue(); }
};

template <>
struct TaggedPtr<jsid> {
  static jsid wrap(JSString* str) {
    return JS::PropertyKey::fromNonIntAtom(str);
  }
  static jsid wrap(JS::Symbol* sym) { return SYMBOL_TO_JSID(sym); }
  static jsid empty() { return JSID_VOID; }
};

template <>
struct TaggedPtr<TaggedProto> {
  static TaggedProto wrap(JSObject* obj) { return TaggedProto(obj); }
  static TaggedProto empty() { return TaggedProto(); }
};

template <typename T>
struct MightBeForwarded {
  static_assert(std::is_base_of_v<Cell, T>, "T must derive from Cell");
  static_assert(!std::is_same_v<Cell, T> && !std::is_same_v<TenuredCell, T>,
                "T must not be Cell or TenuredCell");

  static const bool value =
      std::is_base_of_v<JSObject, T> || std::is_base_of_v<Shape, T> ||
      std::is_base_of_v<BaseShape, T> || std::is_base_of_v<JSString, T> ||
      std::is_base_of_v<JS::BigInt, T> ||
      std::is_base_of_v<js::BaseScript, T> || std::is_base_of_v<js::Scope, T> ||
      std::is_base_of_v<js::RegExpShared, T> ||
      std::is_base_of_v<js::ObjectGroup, T>;
};

template <typename T>
inline bool IsForwarded(const T* t) {
  if (!MightBeForwarded<T>::value) {
    MOZ_ASSERT(!t->isForwarded());
    return false;
  }

  return t->isForwarded();
}

inline bool IsForwarded(const JS::Value& value) {
  auto isForwarded = [](auto t) { return IsForwarded(t); };
  return MapGCThingTyped(value, isForwarded).valueOr(false);
}

template <typename T>
inline T* Forwarded(const T* t) {
  const RelocationOverlay* overlay = RelocationOverlay::fromCell(t);
  MOZ_ASSERT(overlay->isForwarded());
  return reinterpret_cast<T*>(overlay->forwardingAddress());
}

inline Value Forwarded(const JS::Value& value) {
  auto forward = [](auto t) { return TaggedPtr<Value>::wrap(Forwarded(t)); };
  return MapGCThingTyped(value, forward).valueOr(value);
}

template <typename T>
inline T MaybeForwarded(T t) {
  if (IsForwarded(t)) {
    t = Forwarded(t);
  }
  return t;
}

inline const JSClass* MaybeForwardedObjectClass(const JSObject* obj) {
  return MaybeForwarded(obj->groupRaw())->clasp();
}

template <typename T>
inline bool MaybeForwardedObjectIs(JSObject* obj) {
  MOZ_ASSERT(!obj->isForwarded());
  return MaybeForwardedObjectClass(obj) == &T::class_;
}

template <typename T>
inline T& MaybeForwardedObjectAs(JSObject* obj) {
  MOZ_ASSERT(MaybeForwardedObjectIs<T>(obj));
  return *static_cast<T*>(obj);
}

inline RelocationOverlay::RelocationOverlay(Cell* dst) {
  MOZ_ASSERT(dst->flags() == 0);
  uintptr_t ptr = uintptr_t(dst);
  MOZ_ASSERT((ptr & RESERVED_MASK) == 0);
  header_ = ptr | FORWARD_BIT;
}

/* static */
inline RelocationOverlay* RelocationOverlay::forwardCell(Cell* src, Cell* dst) {
  MOZ_ASSERT(!src->isForwarded());
  MOZ_ASSERT(!dst->isForwarded());
  return new (src) RelocationOverlay(dst);
}

inline bool IsAboutToBeFinalizedDuringMinorSweep(Cell** cellp) {
  MOZ_ASSERT(JS::RuntimeHeapIsMinorCollecting());

  if ((*cellp)->isTenured()) {
    return false;
  }

  return !Nursery::getForwardedPointer(cellp);
}

#ifdef JSGC_HASH_TABLE_CHECKS

template <typename T>
inline bool IsGCThingValidAfterMovingGC(T* t) {
  return !IsInsideNursery(t) && !t->isForwarded();
}

template <typename T>
inline void CheckGCThingAfterMovingGC(T* t) {
  if (t) {
    MOZ_RELEASE_ASSERT(IsGCThingValidAfterMovingGC(t));
  }
}

template <typename T>
inline void CheckGCThingAfterMovingGC(const WeakHeapPtr<T*>& t) {
  CheckGCThingAfterMovingGC(t.unbarrieredGet());
}

inline void CheckValueAfterMovingGC(const JS::Value& value) {
  ApplyGCThingTyped(value, [](auto t) { CheckGCThingAfterMovingGC(t); });
}

#endif  // JSGC_HASH_TABLE_CHECKS

} /* namespace gc */
} /* namespace js */

#endif  // gc_Marking_inl_h
