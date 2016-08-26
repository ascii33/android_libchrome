// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/control_message_proxy.h"

#include <stddef.h>
#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/lib/message_builder.h"
#include "mojo/public/cpp/bindings/lib/serialization.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/interfaces/bindings/interface_control_messages.mojom.h"

namespace mojo {
namespace internal {

namespace {

using RunCallback =
    base::Callback<void(interface_control::RunResponseMessageParamsPtr)>;

class RunResponseForwardToCallback : public MessageReceiver {
 public:
  explicit RunResponseForwardToCallback(const RunCallback& callback)
      : callback_(callback) {}
  bool Accept(Message* message) override;

 private:
  RunCallback callback_;
  DISALLOW_COPY_AND_ASSIGN(RunResponseForwardToCallback);
};

bool RunResponseForwardToCallback::Accept(Message* message) {
  interface_control::internal::RunResponseMessageParams_Data* params =
      reinterpret_cast<
          interface_control::internal::RunResponseMessageParams_Data*>(
          message->mutable_payload());
  interface_control::RunResponseMessageParamsPtr params_ptr;
  SerializationContext context;
  Deserialize<interface_control::RunResponseMessageParamsDataView>(
      params, &params_ptr, &context);

  callback_.Run(std::move(params_ptr));
  return true;
}

void SendRunMessage(MessageReceiverWithResponder* receiver,
                    interface_control::RunInputPtr input_ptr,
                    const RunCallback& callback,
                    SerializationContext* context) {
  auto params_ptr = interface_control::RunMessageParams::New();
  params_ptr->input = std::move(input_ptr);
  size_t size = PrepareToSerialize<interface_control::RunMessageParamsDataView>(
      params_ptr, context);
  RequestMessageBuilder builder(interface_control::kRunMessageId, size);

  interface_control::internal::RunMessageParams_Data* params = nullptr;
  Serialize<interface_control::RunMessageParamsDataView>(
      params_ptr, builder.buffer(), &params, context);
  MessageReceiver* responder = new RunResponseForwardToCallback(callback);
  if (!receiver->AcceptWithResponder(builder.message(), responder))
    delete responder;
}

void SendRunOrClosePipeMessage(
    MessageReceiverWithResponder* receiver,
    interface_control::RunOrClosePipeInputPtr input_ptr,
    SerializationContext* context) {
  auto params_ptr = interface_control::RunOrClosePipeMessageParams::New();
  params_ptr->input = std::move(input_ptr);

  size_t size = PrepareToSerialize<
      interface_control::RunOrClosePipeMessageParamsDataView>(params_ptr,
                                                              context);
  MessageBuilder builder(interface_control::kRunOrClosePipeMessageId, size);

  interface_control::internal::RunOrClosePipeMessageParams_Data* params =
      nullptr;
  Serialize<interface_control::RunOrClosePipeMessageParamsDataView>(
      params_ptr, builder.buffer(), &params, context);
  bool ok = receiver->Accept(builder.message());
  ALLOW_UNUSED_LOCAL(ok);
}

void RunVersionCallback(
    const base::Callback<void(uint32_t)>& callback,
    interface_control::RunResponseMessageParamsPtr run_response) {
  uint32_t version = 0u;
  if (run_response->output && run_response->output->is_query_version_result())
    version = run_response->output->get_query_version_result()->version;
  callback.Run(version);
}

}  // namespace

ControlMessageProxy::ControlMessageProxy(MessageReceiverWithResponder* receiver)
    : receiver_(receiver) {
}

void ControlMessageProxy::QueryVersion(
    const base::Callback<void(uint32_t)>& callback) {
  auto input_ptr = interface_control::RunInput::New();
  input_ptr->set_query_version(interface_control::QueryVersion::New());
  SendRunMessage(receiver_, std::move(input_ptr),
                 base::Bind(&RunVersionCallback, callback), &context_);
}

void ControlMessageProxy::RequireVersion(uint32_t version) {
  auto require_version = interface_control::RequireVersion::New();
  require_version->version = version;
  auto input_ptr = interface_control::RunOrClosePipeInput::New();
  input_ptr->set_require_version(std::move(require_version));
  SendRunOrClosePipeMessage(receiver_, std::move(input_ptr), &context_);
}

}  // namespace internal
}  // namespace mojo
