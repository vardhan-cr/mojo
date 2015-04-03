// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines all the "cc" command-line switches.

#ifndef CC_BASE_SWITCHES_H_
#define CC_BASE_SWITCHES_H_

// Since cc is used from the render process, anything that goes here also needs
// to be added to render_process_host_impl.cc.

namespace cc {
namespace switches {

// Switches for the renderer compositor only.
extern const char kDisableThreadedAnimation[];
extern const char kDisableCompositedAntialiasing[];
extern const char kDisableMainFrameBeforeActivation[];
extern const char kEnableMainFrameBeforeActivation[];
extern const char kJankInsteadOfCheckerboard[];
extern const char kTopControlsHideThreshold[];
extern const char kTopControlsShowThreshold[];
extern const char kSlowDownRasterScaleFactor[];
extern const char kCompositeToMailbox[];
extern const char kMaxTilesForInterestArea[];
extern const char kMaxUnusedResourceMemoryUsagePercentage[];
extern const char kEnablePinchVirtualViewport[];
extern const char kDisablePinchVirtualViewport[];
extern const char kStrictLayerPropertyChangeChecking[];
extern const char kEnablePropertyTreeVerification[];

// Switches for both the renderer and ui compositors.
extern const char kUIDisablePartialSwap[];
extern const char kEnableGpuBenchmarking[];

// Debug visualizations.
extern const char kShowCompositedLayerBorders[];
extern const char kUIShowCompositedLayerBorders[];
extern const char kShowFPSCounter[];
extern const char kUIShowFPSCounter[];
extern const char kShowLayerAnimationBounds[];
extern const char kUIShowLayerAnimationBounds[];
extern const char kShowPropertyChangedRects[];
extern const char kUIShowPropertyChangedRects[];
extern const char kShowSurfaceDamageRects[];
extern const char kUIShowSurfaceDamageRects[];
extern const char kShowScreenSpaceRects[];
extern const char kUIShowScreenSpaceRects[];
extern const char kShowReplicaScreenSpaceRects[];
extern const char kUIShowReplicaScreenSpaceRects[];

// Unit test related.
extern const char kCCLayerTreeTestNoTimeout[];
extern const char kCCRebaselinePixeltests[];

}  // namespace switches
}  // namespace cc

#endif  // CC_BASE_SWITCHES_H_
