// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/system/isolated_connection.h"

#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/invitation.h"

namespace mojo {

IsolatedConnection::IsolatedConnection()
    : token_(base::UnguessableToken::Create()) {}

IsolatedConnection::~IsolatedConnection() {
  // We send a dummy invitation over a temporary channel, re-using |token_| as
  // the name. This ensures that the connection set up by Connect(), if any,
  // will be replaced with a short-lived, self-terminating connection.
  //
  // This is a bit of a hack since Mojo does not provide any API for explicitly
  // terminating isolated connections, but this is a decision made to minimize
  // the API surface dedicated to isolated connections in anticipation of the
  // concept being deprecated eventually.
  PlatformChannel channel;
  OutgoingInvitation::SendIsolated(channel.TakeLocalEndpoint(),
                                   token_.ToString());
}

ScopedMessagePipeHandle IsolatedConnection::Connect(
    PlatformChannelEndpoint endpoint) {
  return OutgoingInvitation::SendIsolated(std::move(endpoint),
                                          token_.ToString());
}

ScopedMessagePipeHandle IsolatedConnection::Connect(
    PlatformChannelServerEndpoint endpoint) {
  return OutgoingInvitation::SendIsolated(std::move(endpoint),
                                          token_.ToString());
}

}  // namespace mojo
