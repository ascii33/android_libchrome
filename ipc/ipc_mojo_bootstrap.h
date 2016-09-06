// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IPC_IPC_MOJO_BOOTSTRAP_H_
#define IPC_IPC_MOJO_BOOTSTRAP_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "ipc/ipc.mojom.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_listener.h"
#include "mojo/public/cpp/bindings/associated_group.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace IPC {

// MojoBootstrap establishes a pair of associated interfaces between two
// processes in Chrome.
//
// Clients should implement MojoBootstrap::Delegate to get the associated pipes
// from MojoBootstrap object.
//
// This lives on IO thread other than Create(), which can be called from
// UI thread as Channel::Create() can be.
class IPC_EXPORT MojoBootstrap {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    virtual void OnPipesAvailable(mojom::ChannelAssociatedPtr sender,
                                  mojom::ChannelAssociatedRequest receiver) = 0;
  };

  virtual ~MojoBootstrap() {}

  // Create the MojoBootstrap instance, using |handle| as the message pipe, in
  // mode as specified by |mode|. The result is passed to |delegate|.
  static std::unique_ptr<MojoBootstrap> Create(
      mojo::ScopedMessagePipeHandle handle,
      Channel::Mode mode,
      Delegate* delegate,
      const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner);

  // Start the handshake over the underlying message pipe.
  virtual void Connect() = 0;

  // Stop queuing new messages and start transmitting them instead.
  virtual void Start() = 0;

  // Flush outgoing messages which were queued before Start().
  virtual void Flush() = 0;

  virtual mojo::AssociatedGroup* GetAssociatedGroup() = 0;
};

}  // namespace IPC

#endif  // IPC_IPC_MOJO_BOOTSTRAP_H_
