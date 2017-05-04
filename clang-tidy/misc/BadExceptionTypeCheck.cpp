//===--- BadExceptionTypeCheck.cpp - clang-tidy----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "BadExceptionTypeCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void BadExceptionTypeCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
    cxxThrowExpr(has(expr(unless(hasType(cxxRecordDecl(allOf(
      unless(hasName("::std::basic_string")),
      unless(hasName("::std::auto_ptr")),
      unless(hasName("::std::unique_ptr")),
      unless(hasName("::std::shared_ptr")),
      unless(hasName("::std::weak_ptr"))
    )))))))
    .bind("throw")
  , this);
}

void BadExceptionTypeCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Throw = Result.Nodes.getNodeAs<CXXThrowExpr>("throw");
  const auto ExceptionQualType = Throw->getSubExpr()->getType();

  diag(Throw->getLocStart(),
       "the type of the thrown exception (%0) is not appropriate; "
       "use a class indicating what happened instead")
      << ExceptionQualType.getAsString();
}

} // namespace misc
} // namespace tidy
} // namespace clang
