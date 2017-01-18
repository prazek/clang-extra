//===--- InvalidatedIteratorsCheck.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "InvalidatedIteratorsCheck.h"
#include "../utils/ExprSequence.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Analysis/CFG.h"
#include <iostream>

using namespace clang::ast_matchers;
using namespace clang::tidy::utils;
using namespace clang::ast_type_traits;

namespace clang {
namespace tidy {
namespace misc {

namespace {
const char BlockFuncName[] = "BlockFunc";
const char DeclStmtName[] = "DeclarationStmt";
const char DeclName[] = "Declaration";
const char ExprName[] = "Expr";
}

void InvalidatedIteratorsCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus)
    return;

  Finder->addMatcher(
      declRefExpr(
          hasDeclaration(
              varDecl(allOf(hasAncestor(functionDecl().bind(BlockFuncName)),
                            hasParent(declStmt().bind(DeclStmtName)),
                            hasType(referenceType()),
                            hasDescendant(declRefExpr(
                                hasType(cxxRecordDecl(hasName("std::vector"))),
                                hasDeclaration(varDecl().bind("TheVector"))))))
                  .bind(DeclName)))
          .bind(ExprName),
      this);
}

bool InvalidatedIteratorsCheck::canFuncInvalidate(const FunctionDecl *Func,
                                                  unsigned ArgId,
                                                  unsigned NumCallArgs,
                                                  ASTContext *Context) {
  const auto MemoTuple = std::make_tuple(Func, ArgId, NumCallArgs);
  const auto MemoChkIter = CanFuncInvalidateMemo.find(MemoTuple);
  if (MemoChkIter != CanFuncInvalidateMemo.end())
    return MemoChkIter->second;

  // Temporarily insert a negative answer so that recursive functions
  // won't loop our method.
  const auto InsertResult =
      CanFuncInvalidateMemo.insert(std::make_pair(MemoTuple, false));
  assert(InsertResult.second);
  const auto MemoIter = InsertResult.first;

  if (!Func->hasBody()) {
    // We cannot assume anything about the function; it possibly invalidates
    // iterators.
    return (MemoIter->second = true);
  }

  Func->dumpColor();
  if (Func->getNumParams() != NumCallArgs) {
    // Sometimes number of call arguments is not equal to the number
    // of declaration arguments. As far as we know, it happens only when
    // calling some class operators. In this case, there is also an
    // additional *this argument as the first argument of the call.
    // We should get rid of it so as to match the declaration arguments.
    assert(Func->getNumParams() == NumCallArgs - 1 &&
           "Number of function parameters should be equal to number of call "
           "arguments or call arguments minus one");

    if (ArgId == 0) {
      // We don't care about member calls. These should be covered in other
      // matchers.
      return false;
    }
    ArgId--;
  }
  // std::cerr << "Total arguments: " << Func->getNumParams() << std::endl;
  const auto *Arg = Func->parameters()[ArgId];

  auto ModifyingMatcher = functionDecl(hasDescendant(getModifyingMatcher(Arg)));

  const auto Matches = match(ModifyingMatcher, *Func, *Context);
  for (const auto &Match : Matches) {
    const bool IsPushBack =
        Match.getNodeAs<CXXMemberCallExpr>("PushBackCall") != nullptr;
    if (IsPushBack)
      return (MemoIter->second = true);

    const auto *CallUse = Match.getNodeAs<CallExpr>("CallExpr");
    assert(CallUse);

    if (canCallInvalidate(CallUse, Match, Context))
      return (MemoIter->second = true);
  }
  return false;
}

bool InvalidatedIteratorsCheck::canCallInvalidate(
    const CallExpr *Call, ast_matchers::BoundNodes Match, ASTContext *Context) {
  const auto *FuncArg = Match.getNodeAs<Expr>("FuncArg");
  const auto *FuncDecl = Match.getNodeAs<FunctionDecl>("FuncDecl");

  for (unsigned i = 0; i < Call->getNumArgs(); ++i) {
    const auto *Arg = Call->getArg(i);
    if (Arg == FuncArg) {
      Call->dumpColor();
      Arg->dumpColor();
      std::cerr << "Arg: " << i << " / " << Call->getNumArgs() << std::endl;
    }

    if (Arg == FuncArg &&
        canFuncInvalidate(FuncDecl, i, Call->getNumArgs(), Context))
      return true;
  }

  return false;
}

InvalidatedIteratorsCheck::ExprMatcherType
InvalidatedIteratorsCheck::getModifyingMatcher(const VarDecl *VectorDecl) {
  auto InvalidateMatcher =
      cxxMemberCallExpr(has(memberExpr(hasDeclaration(cxxMethodDecl(hasAnyName(
                                           "push_back", "emplace_back", "clear",
                                           "insert", "emplace"))),
                                       has(declRefExpr(hasDeclaration(
                                           varDecl(equalsNode(VectorDecl))))))))
          .bind("PushBackCall");
  auto FuncCallMatcher =
      callExpr(hasDeclaration(functionDecl().bind("FuncDecl")),
               hasAnyArgument(
                   declRefExpr(hasDeclaration(varDecl(equalsNode(VectorDecl))))
                       .bind("FuncArg")))
          .bind("CallExpr");
  auto ModifyingMatcher = expr(eachOf(InvalidateMatcher, FuncCallMatcher));

  return ModifyingMatcher;
}

void InvalidatedIteratorsCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *RefUse = Result.Nodes.getNodeAs<DeclRefExpr>(ExprName);
  const auto *RefDecl = Result.Nodes.getNodeAs<VarDecl>(DeclName);
  const auto *RefDeclStmt = Result.Nodes.getNodeAs<DeclStmt>(DeclStmtName);
  const auto *Func = Result.Nodes.getNodeAs<FunctionDecl>(BlockFuncName);
  const auto *VectorDecl = Result.Nodes.getNodeAs<VarDecl>("TheVector");
  Stmt *FuncBody = Func->getBody();

  CFG::BuildOptions Options;
  Options.AddImplicitDtors = true;
  Options.AddTemporaryDtors = true;

  auto *Context = Result.Context;

  const std::unique_ptr<CFG> TheCFG =
      CFG::buildCFG(nullptr, FuncBody, Result.Context, Options);
  const std::unique_ptr<StmtToBlockMap> BlockMap(
      new StmtToBlockMap(TheCFG.get(), Result.Context));
  const std::unique_ptr<ExprSequence> Sequence(
      new ExprSequence(TheCFG.get(), Result.Context));
  const CFGBlock *Block = BlockMap->blockContainingStmt(RefUse);
  if (!Block) {
    return;
  }

  auto ModifyingMatcher = getModifyingMatcher(VectorDecl);

  for (const auto &BlockElem : *Block) {
    const auto StmtPtr = BlockElem.getAs<CFGStmt>();
    if (!StmtPtr.hasValue())
      continue;
    auto Results = match(ModifyingMatcher, *(StmtPtr->getStmt()), *Context);

    for (const auto &Match : Results) {
      const auto *PushBackUse =
          Match.getNodeAs<CXXMemberCallExpr>("PushBackCall");
      const auto *CallUse = Match.getNodeAs<CallExpr>("CallExpr");

      if (CallUse && !canCallInvalidate(CallUse, Match, Context))
        continue;

      const Expr *Use = (PushBackUse != nullptr ? cast<Expr>(PushBackUse)
                                                : cast<Expr>(CallUse));

      if (Sequence->potentiallyAfter(Use, RefDeclStmt) &&
          Sequence->potentiallyAfter(RefUse, Use)) {
        diag(RefUse->getLocation(), "%0 might be invalidated before the access")
            << RefDecl;
        // TODO: add a whole possible chain of invalidations.
        diag(Use->getExprLoc(), "possible place of invalidation",
             DiagnosticIDs::Note);
        return;
      }
    }
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
