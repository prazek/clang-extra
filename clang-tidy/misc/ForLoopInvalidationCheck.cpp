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

namespace {
const auto DefaultContainers = "std::unordered_map";
const auto DefaultFunctions = "insert; erase; operator[]";

} // namespace

ForLoopInvalidationCheck::ForLoopInvalidationCheck(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      Containers(utils::options::parseStringList(Options.get(
          "ContainersWithPushBack", DefaultContainers))),
      Functions(utils::options::parseStringList(
          Options.get("SmartPointers", DefaultFunctions))) {}

void ForLoopInvalidationCheck::registerMatchers(MatchFinder *Finder) {
  std::string regex;
  {
    bool first = true;
    for (const auto& elem: Containers) {
      if (!first)
        regex += '|';
      regex += elem;
      first = false;
    }
  }

  auto RangeInitMatcher = hasType(qualType(hasDeclaration(classTemplateSpecializationDecl(
       matchesName(regex)))));

  SmallVector<StringRef, 5> FunctionsVec(Functions.begin(), Functions.end());

  auto DeclRef = declRefExpr(to(varDecl().bind("called_object")));

  Finder->addMatcher(
      callExpr(
          anyOf(
              cxxMemberCallExpr(on(DeclRef)),
              cxxOperatorCallExpr(hasArgument(0, DeclRef))
          ),
          callee(decl(cxxMethodDecl(hasAnyName(FunctionsVec)))),
          hasAncestor(cxxForRangeStmt(
              hasRangeInit(RangeInitMatcher),
              hasRangeInit(declRefExpr(to(varDecl(equalsBoundNode("called_object")))))
          ))
      ).bind("expr"), this);
}

void ForLoopInvalidationCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedExpression = Result.Nodes.getNodeAs<CallExpr>("expr");

  diag(MatchedExpression->getLocStart(), "this call may lead to iterator invalidation");
}

} // namespace misc
} // namespace tidy
} // namespace clang
