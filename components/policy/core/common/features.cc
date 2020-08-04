// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/features.h"

namespace policy {

namespace features {

const base::Feature kPolicyAtomicGroup{"PolicyAtomicGroup",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kCBCMPolicyInvalidations{"CBCMPolicyInvalidations",
                                             base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kCBCMRemoteCommands{"CBCMRemoteCommands",
                                        base::FEATURE_DISABLED_BY_DEFAULT};

#if defined(OS_APPLE)
const base::Feature kIgnoreSensitivePoliciesOnUnmanagedMac{
    "IgnoreSensitivePoliciesOnUnmanagedMac", base::FEATURE_ENABLED_BY_DEFAULT};
#endif

}  // namespace features

}  // namespace policy
