// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_REQUEST_CONTEXT_H_
#define MOJO_EDK_SYSTEM_REQUEST_CONTEXT_H_

#include "base/containers/stack_container.h"
#include "base/macros.h"
#include "mojo/edk/system/handle_signals_state.h"
#include "mojo/edk/system/watcher.h"

namespace mojo {
namespace edk {

// A RequestContext is a thread-local object which exists for the duration of
// a single system API call. It is constructed immediately upon EDK entry and
// destructed immediately before returning to the caller, after any internal
// locks have been released.
//
// NOTE: It is legal to construct a RequestContext while another one already
// exists on the current thread, but it is not safe to use the nested context
// for any reason. Therefore it is important to always use
// |RequestContext::current()| rather than referring to any local instance
// directly.
class RequestContext {
 public:
  RequestContext();
  ~RequestContext();

  // Returns the current thread-local RequestContext.
  static RequestContext* current();

  // Adds a finalizer to this RequestContext corresponding to a watch callback
  // which should be triggered in response to some handle state change. If
  // the Watcher hasn't been cancelled by the time this RequestContext is
  // destroyed, its WatchCallback will be invoked with |result| and |state|
  // arguments.
  void AddWatchNotifyFinalizer(scoped_refptr<Watcher> watcher,
                               MojoResult result,
                               const HandleSignalsState& state);

  // Adds a finalizer to this RequestContext which cancels a watch.
  void AddWatchCancelFinalizer(scoped_refptr<Watcher> watcher);

 private:
  // Is this request context the current one?
  bool IsCurrent() const;

  struct WatchNotifyFinalizer {
    WatchNotifyFinalizer(scoped_refptr<Watcher> watcher,
                         MojoResult result,
                         const HandleSignalsState& state);
    ~WatchNotifyFinalizer();

    scoped_refptr<Watcher> watcher;
    MojoResult result;
    HandleSignalsState state;
  };

  // Chosen by fair dice roll.
  //
  // TODO: We should measure the distribution of # of finalizers typical to
  // any RequestContext and adjust this number accordingly. It's probably
  // almost always 1, but 4 seems like a harmless upper bound for now.
  static const size_t kStaticWatchFinalizersCapacity = 4;

  using WatchNotifyFinalizerList =
      base::StackVector<WatchNotifyFinalizer, kStaticWatchFinalizersCapacity>;
  using WatchCancelFinalizerList =
      base::StackVector<scoped_refptr<Watcher>, kStaticWatchFinalizersCapacity>;

  WatchNotifyFinalizerList watch_notify_finalizers_;
  WatchCancelFinalizerList watch_cancel_finalizers_;

  DISALLOW_COPY_AND_ASSIGN(RequestContext);
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_REQUEST_CONTEXT_H_
