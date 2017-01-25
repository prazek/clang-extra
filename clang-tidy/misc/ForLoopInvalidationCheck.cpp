//===--- ForLoopInvalidationCheck.cpp - clang-tidy-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ForLoopInvalidationCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "../utils/OptionsUtils.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {


ForLoopInvalidationCheck::ForLoopInvalidationCheck(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {}

void ForLoopInvalidationCheck::registerMatchers(MatchFinder *Finder) {
  auto DeclRef = declRefExpr(to(varDecl().bind("called_object")));

  Finder->addMatcher(
      callExpr(
          anyOf(
              cxxMemberCallExpr(on(DeclRef)),
              cxxOperatorCallExpr(hasArgument(0, DeclRef))
          ),
          callee(decl(cxxMethodDecl(unless(isConst())).bind("decl"))),
          hasAncestor(cxxForRangeStmt(
              hasRangeInit(declRefExpr(to(varDecl(equalsBoundNode("called_object")))))
          ))
      ).bind("expr"), this);
}

void ForLoopInvalidationCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Decl = Result.Nodes.getNodeAs<CXXMethodDecl>("decl");

  auto HaveEqualNames = [](const CXXMethodDecl* First, const CXXMethodDecl* Second) {
    return First->getQualifiedNameAsString() == Second->getQualifiedNameAsString();
  };

  auto HaveEqualParameters = [](const CXXMethodDecl* First, const CXXMethodDecl* Second) {
    if (First->param_size() != Second->param_size())
      return false;

    for (size_t i = 0; i < First->param_size(); i++) {
      if (First->parameters()[i]->getType() != Second->parameters()[i]->getType())
        return false;
    }

    return true;
  };

  auto HaveConstEquivalent = [&HaveEqualNames, &HaveEqualParameters](const CXXMethodDecl* Method) {
    const auto* Class = Method->getParent();
    for (const auto* M: Class->methods()) {
      if (M == Method)
        continue;

      if (!HaveEqualNames(Method, M))
        continue;

      if (!HaveEqualParameters(Method, M))
        continue;

      if (!M->isConst())
        continue;

      return true;
    }
    return false;
  };

  if (HaveConstEquivalent(Decl))
    return;

  const auto *MatchedExpression = Result.Nodes.getNodeAs<CallExpr>("expr");

  diag(MatchedExpression->getLocStart(), "this call may lead to iterator invalidation");
}

} // namespace misc
} // namespace tidy
} // namespace clang
