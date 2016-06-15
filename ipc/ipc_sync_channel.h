// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IPC_IPC_SYNC_CHANNEL_H_
#define IPC_IPC_SYNC_CHANNEL_H_

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event_watcher.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_channel_proxy.h"
#include "ipc/ipc_sync_message.h"
#include "ipc/ipc_sync_message_filter.h"
#include "mojo/public/c/system/types.h"
#include "mojo/public/cpp/system/watcher.h"

namespace base {
class WaitableEvent;
};

namespace mojo {
class Watcher;
}

namespace IPC {

class ChannelFactory;
class MojoEvent;
class SyncMessage;

// This is similar to ChannelProxy, with the added feature of supporting sending
// synchronous messages.
//
// Overview of how the sync channel works
// --------------------------------------
// When the sending thread sends a synchronous message, we create a bunch
// of tracking info (created in Send, stored in the PendingSyncMsg
// structure) associated with the message that we identify by the unique
// "MessageId" on the SyncMessage. Among the things we save is the
// "Deserializer" which is provided by the sync message. This object is in
// charge of reading the parameters from the reply message and putting them in
// the output variables provided by its caller.
//
// The info gets stashed in a queue since we could have a nested stack of sync
// messages (each side could send sync messages in response to sync messages,
// so it works like calling a function). The message is sent to the I/O thread
// for dispatch and the original thread blocks waiting for the reply.
//
// SyncContext maintains the queue in a threadsafe way and listens for replies
// on the I/O thread. When a reply comes in that matches one of the messages
// it's looking for (using the unique message ID), it will execute the
// deserializer stashed from before, and unblock the original thread.
//
//
// Significant complexity results from the fact that messages are still coming
// in while the original thread is blocked. Normal async messages are queued
// and dispatched after the blocking call is complete. Sync messages must
// be dispatched in a reentrant manner to avoid deadlock.
//
//
// Note that care must be taken that the lifetime of the ipc_thread argument
// is more than this object.  If the message loop goes away while this object
// is running and it's used to send a message, then it will use the invalid
// message loop pointer to proxy it to the ipc thread.
class IPC_EXPORT SyncChannel : public ChannelProxy {
 public:
  enum RestrictDispatchGroup {
    kRestrictDispatchGroup_None = 0,
  };

  // Creates and initializes a sync channel. If create_pipe_now is specified,
  // the channel will be initialized synchronously.
  // The naming pattern follows IPC::Channel.
  static std::unique_ptr<SyncChannel> Create(
      const IPC::ChannelHandle& channel_handle,
      IPC::Channel::Mode mode,
      Listener* listener,
      const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner,
      bool create_pipe_now,
      base::WaitableEvent* shutdown_event);

  static std::unique_ptr<SyncChannel> Create(
      std::unique_ptr<ChannelFactory> factory,
      Listener* listener,
      const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner,
      bool create_pipe_now,
      base::WaitableEvent* shutdown_event);

  // Creates an uninitialized sync channel. Call ChannelProxy::Init to
  // initialize the channel. This two-step setup allows message filters to be
  // added before any messages are sent or received.
  static std::unique_ptr<SyncChannel> Create(
      Listener* listener,
      const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner,
      base::WaitableEvent* shutdown_event);

  ~SyncChannel() override;

  bool Send(Message* message) override;

  // Sets the dispatch group for this channel, to only allow re-entrant dispatch
  // of messages to other channels in the same group.
  //
  // Normally, any unblocking message coming from any channel can be dispatched
  // when any (possibly other) channel is blocked on sending a message. This is
  // needed in some cases to unblock certain loops (e.g. necessary when some
  // processes share a window hierarchy), but may cause re-entrancy issues in
  // some cases where such loops are not possible. This flags allows the tagging
  // of some particular channels to only re-enter in known correct cases.
  //
  // Incoming messages on channels belonging to a group that is not
  // kRestrictDispatchGroup_None will only be dispatched while a sync message is
  // being sent on a channel of the *same* group.
  // Incoming messages belonging to the kRestrictDispatchGroup_None group (the
  // default) will be dispatched in any case.
  void SetRestrictDispatchChannelGroup(int group);

  // Creates a new IPC::SyncMessageFilter and adds it to this SyncChannel.
  // This should be used instead of directly constructing a new
  // SyncMessageFilter.
  scoped_refptr<IPC::SyncMessageFilter> CreateSyncMessageFilter();

 protected:
  class ReceivedSyncMsgQueue;
  friend class ReceivedSyncMsgQueue;

  // SyncContext holds the per object data for SyncChannel, so that SyncChannel
  // can be deleted while it's being used in a different thread.  See
  // ChannelProxy::Context for more information.
  class SyncContext : public Context {
   public:
    SyncContext(
        Listener* listener,
        const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner,
        base::WaitableEvent* shutdown_event);

    // Adds information about an outgoing sync message to the context so that
    // we know how to deserialize the reply.
    void Push(SyncMessage* sync_msg);

    // Cleanly remove the top deserializer (and throw it away).  Returns the
    // result of the Send call for that message.
    bool Pop();

    // Returns a Mojo Event that signals when a sync send is complete or timed
    // out or the process shut down.
    MojoEvent* GetSendDoneEvent();

    // Returns a Mojo Event that signals when an incoming message that's not the
    // pending reply needs to get dispatched (by calling DispatchMessages.)
    MojoEvent* GetDispatchEvent();

    void DispatchMessages();

    // Checks if the given message is blocking the listener thread because of a
    // synchronous send.  If it is, the thread is unblocked and true is
    // returned. Otherwise the function returns false.
    bool TryToUnblockListener(const Message* msg);

    base::WaitableEvent* shutdown_event() { return shutdown_event_; }

    ReceivedSyncMsgQueue* received_sync_msgs() {
      return received_sync_msgs_.get();
    }

    void set_restrict_dispatch_group(int group) {
      restrict_dispatch_group_ = group;
    }

    int restrict_dispatch_group() const {
      return restrict_dispatch_group_;
    }

   private:
    ~SyncContext() override;
    // ChannelProxy methods that we override.

    // Called on the listener thread.
    void Clear() override;

    // Called on the IPC thread.
    bool OnMessageReceived(const Message& msg) override;
    void OnChannelError() override;
    void OnChannelOpened() override;
    void OnChannelClosed() override;

    // Cancels all pending Send calls.
    void CancelPendingSends();

    void OnShutdownEventSignaled(base::WaitableEvent* event);

    typedef std::deque<PendingSyncMsg> PendingSyncMessageQueue;
    PendingSyncMessageQueue deserializers_;
    base::Lock deserializers_lock_;

    scoped_refptr<ReceivedSyncMsgQueue> received_sync_msgs_;

    base::WaitableEvent* shutdown_event_;
    base::WaitableEventWatcher shutdown_watcher_;
    base::WaitableEventWatcher::EventCallback shutdown_watcher_callback_;
    int restrict_dispatch_group_;
  };

 private:
  SyncChannel(
      Listener* listener,
      const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner,
      base::WaitableEvent* shutdown_event);

  void OnDispatchHandleReady(MojoResult result);

  SyncContext* sync_context() {
    return reinterpret_cast<SyncContext*>(context());
  }

  // Both these functions wait for a reply, timeout or process shutdown.  The
  // latter one also runs a nested message loop in the meantime.
  static void WaitForReply(SyncContext* context, bool pump_messages);

  // Runs a nested message loop until a reply arrives, times out, or the process
  // shuts down.
  static void WaitForReplyWithNestedMessageLoop(SyncContext* context);

  // Starts the dispatch watcher.
  void StartWatching();

  // ChannelProxy overrides:
  void OnChannelInit() override;
  bool SendNow(std::unique_ptr<Message> message) override;
  bool SendOnIPCThread(std::unique_ptr<Message> message) override;

  // Used to signal events between the IPC and listener threads.
  mojo::Watcher dispatch_watcher_;

  // Tracks SyncMessageFilters created before complete channel initialization.
  std::vector<scoped_refptr<SyncMessageFilter>> pre_init_sync_message_filters_;

  DISALLOW_COPY_AND_ASSIGN(SyncChannel);
};

}  // namespace IPC

#endif  // IPC_IPC_SYNC_CHANNEL_H_
