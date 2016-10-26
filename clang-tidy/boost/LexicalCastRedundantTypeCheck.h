//===--- LexicalCastRedundantTypeCheck.h - clang-tidy------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BOOST_LEXICAL_CAST_REDUNDANT_TYPE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BOOST_LEXICAL_CAST_REDUNDANT_TYPE_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace boost {

/// Checks for reduntand, or different type using boost::lexical_cast
///
/// Given:
/// \code
///   int a = boost::lexical_cast<int>("42");
///   =>
///   auto a = boost::lexical_cast<int>("42");
/// \code
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/boost-LexicalCastRedundantType.html
class LexicalCastRedundantTypeCheck : public ClangTidyCheck {
public:
  LexicalCastRedundantTypeCheck(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  void apply(const DeclStmt *D, const std::string &Warning, bool DoFixit);
};

} // namespace boost
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BOOST_LEXICALCASTREDUNDANTTYPE_H