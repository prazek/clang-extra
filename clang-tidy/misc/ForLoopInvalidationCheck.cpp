//===--- ForLoopInvalidationCheck.cpp - clang-tidy-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ForLoopInvalidationCheck.h"
#include "../utils/OptionsUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

namespace {

const char CalledObject[] = "calledObject";
const char MethodDeclaration[] = "methodDeclaration";
const char MethodCall[] = "methodCall";
const char DefaultAllowedTypes[] =
    "::std::list; ::std::forward_list";

bool HaveEqualNames(const CXXMethodDecl *First, const CXXMethodDecl *Second) {
  return First->getQualifiedNameAsString() ==
         Second->getQualifiedNameAsString();
}

bool HaveEqualParametersList(const CXXMethodDecl *First,
                             const CXXMethodDecl *Second) {
  if (First->param_size() != Second->param_size())
    return false;

  for (size_t i = 0; i < First->param_size(); i++)
    if (First->parameters()[i]->getType() != Second->parameters()[i]->getType())
      return false;

  return true;
};

/// \brief Returns true if given method have alternative which is const.
bool HaveEquivalentConstMethod(const CXXMethodDecl *Method) {
  const auto *Class = Method->getParent();
  for (const auto *M : Class->methods()) {
    if (M != Method && M->isConst() && HaveEqualNames(Method, M) &&
        HaveEqualParametersList(Method, M))
      return true;
  }
  return false;
};

} // namespace

ForLoopInvalidationCheck::ForLoopInvalidationCheck(StringRef Name,
                                                   ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      AllowedTypes(utils::options::parseStringList(
          Options.get("AllowedTypes", DefaultAllowedTypes))) {}

void ForLoopInvalidationCheck::registerMatchers(MatchFinder *Finder) {
  auto AllowedTypesMatcher = hasAnyName(
      SmallVector<StringRef, 5>(AllowedTypes.begin(), AllowedTypes.end()));
  auto CalledObjectRef =
      declRefExpr(to(varDecl(hasType(qualType(hasDeclaration(
                                 namedDecl(unless(AllowedTypesMatcher))))))
                         .bind(CalledObject)));
  auto MethodMatcher = cxxMethodDecl(unless(isConst())).bind(MethodDeclaration);

  Finder->addMatcher(
      callExpr(anyOf(cxxMemberCallExpr(on(CalledObjectRef)),
                     cxxOperatorCallExpr(hasArgument(0, CalledObjectRef))),
               callee(MethodMatcher),
               hasAncestor(cxxForRangeStmt(hasRangeInit(
                   declRefExpr(to(varDecl(equalsBoundNode(CalledObject))))))))
          .bind(MethodCall),
      this);
}

void ForLoopInvalidationCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MethodDecl =
      Result.Nodes.getNodeAs<CXXMethodDecl>(MethodDeclaration);

  if (HaveEquivalentConstMethod(MethodDecl))
    return;

  const auto *MatchedExpression = Result.Nodes.getNodeAs<CallExpr>(MethodCall);

  diag(MatchedExpression->getLocStart(),
       "this call may lead to iterator invalidation");
}

} // namespace misc
} // namespace tidy
} // namespace clang
