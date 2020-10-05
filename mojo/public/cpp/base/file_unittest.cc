// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/scoped_temp_dir.h"
#include "base/sync_socket.h"
#include "build/build_config.h"
#include "mojo/public/cpp/base/file_mojom_traits.h"
#include "mojo/public/cpp/base/read_only_file_mojom_traits.h"
#include "mojo/public/cpp/test_support/test_utils.h"
#include "mojo/public/mojom/base/file.mojom.h"
#include "mojo/public/mojom/base/read_only_file.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo_base {
namespace file_unittest {

TEST(FileTest, File) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::File file(
      temp_dir.GetPath().AppendASCII("test_file.txt"),
      base::File::FLAG_CREATE | base::File::FLAG_WRITE | base::File::FLAG_READ);
  const base::StringPiece test_content =
      "A test string to be stored in a test file";
  file.WriteAtCurrentPos(test_content.data(),
                         base::checked_cast<int>(test_content.size()));

  base::File file_out;
  ASSERT_TRUE(
      mojo::test::SerializeAndDeserialize<mojom::File>(&file, &file_out));
  std::vector<char> content(test_content.size());
  ASSERT_TRUE(file_out.IsValid());
  ASSERT_FALSE(file_out.async());
  ASSERT_EQ(static_cast<int>(test_content.size()),
            file_out.Read(0, content.data(),
                          base::checked_cast<int>(test_content.size())));
  EXPECT_EQ(test_content,
            base::StringPiece(content.data(), test_content.size()));
}

TEST(FileTest, AsyncFile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path = temp_dir.GetPath().AppendASCII("async_test_file.txt");

  base::File write_file(path, base::File::FLAG_CREATE | base::File::FLAG_WRITE);
  const base::StringPiece test_content = "test string";
  write_file.WriteAtCurrentPos(test_content.data(),
                               base::checked_cast<int>(test_content.size()));
  write_file.Close();

  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ |
                            base::File::FLAG_ASYNC);
  base::File file_out;
  ASSERT_TRUE(
      mojo::test::SerializeAndDeserialize<mojom::File>(&file, &file_out));
  ASSERT_TRUE(file_out.async());
}

TEST(FileTest, InvalidFile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  // Test that |file_out| is set to an invalid file.
  base::File file_out(
      temp_dir.GetPath().AppendASCII("test_file.txt"),
      base::File::FLAG_CREATE | base::File::FLAG_WRITE | base::File::FLAG_READ);

  base::File file = base::File();
  ASSERT_TRUE(
      mojo::test::SerializeAndDeserialize<mojom::File>(&file, &file_out));
  EXPECT_FALSE(file_out.IsValid());
}

TEST(FileTest, ReadOnlyFile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::File file(
      temp_dir.GetPath().AppendASCII("test_file.txt"),
      base::File::FLAG_CREATE | base::File::FLAG_WRITE | base::File::FLAG_READ);
  const base::StringPiece test_content =
      "A test string to be stored in a test file";
  file.WriteAtCurrentPos(test_content.data(),
                         base::checked_cast<int>(test_content.size()));
  file.Close();

  base::File readonly(temp_dir.GetPath().AppendASCII("test_file.txt"),
                      base::File::FLAG_OPEN | base::File::FLAG_READ);

  base::File file_out;
  ASSERT_TRUE(mojo::test::SerializeAndDeserialize<mojom::ReadOnlyFile>(
      &readonly, &file_out));
  std::vector<char> content(test_content.size());
  ASSERT_TRUE(file_out.IsValid());
  ASSERT_FALSE(file_out.async());
  ASSERT_EQ(static_cast<int>(test_content.size()),
            file_out.Read(0, content.data(),
                          base::checked_cast<int>(test_content.size())));
  EXPECT_EQ(test_content,
            base::StringPiece(content.data(), test_content.size()));
}

// This dies only if we can interrogate the underlying platform handle.
#if defined(OS_WIN) || defined(OS_POSIX) || defined(OS_FUCHSIA)
#if !defined(OS_NACL) && !defined(OS_AIX)
TEST(FileTest, ReadOnlyFileDeath) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::File file(
      temp_dir.GetPath().AppendASCII("test_file.txt"),
      base::File::FLAG_CREATE | base::File::FLAG_WRITE | base::File::FLAG_READ);
  const base::StringPiece test_content =
      "A test string to be stored in a test file";
  file.WriteAtCurrentPos(test_content.data(),
                         base::checked_cast<int>(test_content.size()));
  file.Close();

  base::File writable(
      temp_dir.GetPath().AppendASCII("test_file.txt"),
      base::File::FLAG_OPEN | base::File::FLAG_READ | base::File::FLAG_WRITE);

  base::File file_out;
  EXPECT_DEATH_IF_SUPPORTED(
      mojo::test::SerializeAndDeserialize<mojom::ReadOnlyFile>(&writable,
                                                               &file_out),
      "Check failed: IsReadOnlyFile");
}
#endif  // !defined(OS_NACL) && !defined(OS_AIX)
#endif  // defined(OS_WIN) || defined(OS_POSIX) || defined(OS_FUCHSIA)

// This should work on all platforms. This check might be relaxed in which case
// this test can be removed.
TEST(FileTest, NonPhysicalFileDeath) {
  base::SyncSocket sync_a;
  base::SyncSocket sync_b;
  ASSERT_TRUE(base::SyncSocket::CreatePair(&sync_a, &sync_b));
  base::File file_pipe_a(sync_a.Take());
  base::File file_pipe_b(sync_b.Take());

  base::File file_out;
  EXPECT_DEATH_IF_SUPPORTED(
      mojo::test::SerializeAndDeserialize<mojom::ReadOnlyFile>(&file_pipe_a,
                                                               &file_out),
      "Check failed: IsPhysicalFile");
  EXPECT_DEATH_IF_SUPPORTED(
      mojo::test::SerializeAndDeserialize<mojom::ReadOnlyFile>(&file_pipe_b,
                                                               &file_out),
      "Check failed: IsPhysicalFile");
}

}  // namespace file_unittest
}  // namespace mojo_base
