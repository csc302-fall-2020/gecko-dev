/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_layers_FrameUniformityData_h_
#define mozilla_layers_FrameUniformityData_h_

#include "ipc/IPCMessageUtils.h"
#include "js/TypeDecls.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"

namespace mozilla {
namespace layers {
class Layer;

class FrameUniformityData {
  friend struct IPC::ParamTraits<FrameUniformityData>;

 public:
  bool ToJS(JS::MutableHandleValue aOutValue, JSContext* aContext);
  // Contains the calculated frame uniformities
  std::map<uintptr_t, float> mUniformities;
};

struct LayerTransforms {
  LayerTransforms() = default;

  gfx::Point GetAverage();
  gfx::Point GetStdDev();
  bool Sanitize();

  // 60 fps * 5 seconds worth of data
  AutoTArray<gfx::Point, 300> mTransforms;
};

class LayerTransformRecorder {
 public:
  LayerTransformRecorder() = default;
  ~LayerTransformRecorder();

  void RecordTransform(Layer* aLayer, const gfx::Point& aTransform);
  void Reset();
  void EndTest(FrameUniformityData* aOutData);

 private:
  float CalculateFrameUniformity(uintptr_t aLayer);
  LayerTransforms* GetLayerTransforms(uintptr_t aLayer);
  using FrameTransformMap =
      std::map<uintptr_t, mozilla::UniquePtr<LayerTransforms>>;
  FrameTransformMap mFrameTransforms;
};

}  // namespace layers
}  // namespace mozilla

namespace IPC {
template <>
struct ParamTraits<mozilla::layers::FrameUniformityData> {
  typedef mozilla::layers::FrameUniformityData paramType;

  static void Write(Message* aMsg, const paramType& aParam) {
    WriteParam(aMsg, aParam.mUniformities);
  }

  static bool Read(const Message* aMsg, PickleIterator* aIter,
                   paramType* aResult) {
    return ParamTraitsStd<std::map<uintptr_t, float>>::Read(
        aMsg, aIter, &aResult->mUniformities);
  }
};

}  // namespace IPC

#endif  // mozilla_layers_FrameUniformityData_h_
