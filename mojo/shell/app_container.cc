// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/native_library.h"
#include "base/thread_task_runner_handle.h"
#include "base/threading/thread.h"
#include "mojo/public/system/core.h"
#include "mojo/shell/app_container.h"

typedef MojoResult (*MojoMainFunction)(mojo::Handle pipe);

namespace mojo {
namespace shell {

void LaunchAppOnThread(
    const base::FilePath& app_path,
    Handle app_handle) {
  MojoResult result = MOJO_RESULT_OK;
  MojoMainFunction main_function = NULL;

  base::NativeLibrary app_library = base::LoadNativeLibrary(app_path, NULL);
  if (!app_library) {
    LOG(ERROR) << "Failed to load library: " << app_path.value().c_str();
    goto completed;
  }

  main_function = reinterpret_cast<MojoMainFunction>(
      base::GetFunctionPointerFromNativeLibrary(app_library, "MojoMain"));
  if (!main_function) {
    LOG(ERROR) << "Entrypoint MojoMain not found.";
    goto completed;
  }

  result = main_function(app_handle);
  if (result < MOJO_RESULT_OK) {
    LOG(ERROR) << "MojoMain returned an error: " << result;
    // TODO(*): error handling?
    goto completed;
  }

completed:
  base::UnloadNativeLibrary(app_library);
  Close(app_handle);
}

AppContainer::AppContainer()
    : weak_factory_(this) {
}

AppContainer::~AppContainer() {
}

void AppContainer::LaunchApp(const base::FilePath& app_path) {
  Handle app_handle;
  MojoResult result = CreateMessagePipe(&shell_handle_, &app_handle);
  if (result < MOJO_RESULT_OK) {
    // Failure..
  }

  // Launch the app on its own thread.
  // TODO(beng): Create a unique thread name.
  thread_.reset(new base::Thread("app_thread"));
  thread_->Start();
  thread_->message_loop_proxy()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&LaunchAppOnThread, app_path, app_handle),
      base::Bind(&AppContainer::AppCompleted, weak_factory_.GetWeakPtr()));

  const char* hello_msg = "Hello";
  result = WriteMessage(shell_handle_, hello_msg, strlen(hello_msg)+1,
                        NULL, 0, MOJO_WRITE_MESSAGE_FLAG_NONE);
  if (result < MOJO_RESULT_OK) {
    // Failure..
  }
}


void AppContainer::AppCompleted() {
  thread_.reset();
  Close(shell_handle_);

  // Probably want to do something more sophisticated here, like notify someone
  // else to do this.
  base::MessageLoop::current()->Quit();
}

}  // namespace shell
}  // namespace mojo
