//===--- FileOpenFlagCheck.h - clang-tidy----------------------------------===//
//
//                      The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_ANDROID_FILE_OPEN_FLAG_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_ANDROID_FILE_OPEN_FLAG_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace android {

/// Finds code that opens file without using the O_CLOEXEC flag.
///
/// open(), openat(), and open64() had better to include O_CLOEXEC in their
/// flags argument. Only consider simple cases that the corresponding argument
/// is constant or binary operation OR among constants like 'O_CLOEXEC' or
/// 'O_CLOEXEC | O_RDONLY'. No constant propagation is performed.
///
/// Only the symbolic 'O_CLOEXEC' macro definition is checked, not the concrete
/// value.
class FileOpenFlagCheck : public ClangTidyCheck {
public:
  FileOpenFlagCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace android
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_ANDROID_FILE_OPEN_FLAG_H
