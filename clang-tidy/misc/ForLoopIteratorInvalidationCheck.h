//===--- ForLoopIteratorInvalidationCheck.h - clang-tidy-----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_FOR_LOOP_ITERATOR_INVALIDATION_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_FOR_LOOP_ITERATOR_INVALIDATION_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace misc {

/// Find and flag suspicious method calls on objects iterated using
/// C++11 for-range loop. If such call can invalidate iterators then
/// behaviour is undefined.
/// Example:
/// \code
///   std::vector<int> v = {1, 2, 3};
///   for (auto i : v) {
///     v.push_back(0); // Boom!
///   }
/// \endcode
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-for-loop-iterator-invalidation.html
class ForLoopIteratorInvalidationCheck : public ClangTidyCheck {
public:
  ForLoopIteratorInvalidationCheck(StringRef Name, ClangTidyContext *Context);
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  std::vector<std::string> SafeTypes;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_FOR_LOOP_ITERATOR_INVALIDATION_H
