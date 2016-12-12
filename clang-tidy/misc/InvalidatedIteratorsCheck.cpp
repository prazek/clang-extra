//===--- InvalidatedIteratorsCheck.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "InvalidatedIteratorsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Analysis/CFG.h"
#include "../utils/ExprSequence.h"


using namespace clang::ast_matchers;
using namespace clang::tidy::utils;
using namespace clang::ast_type_traits;

namespace clang {
namespace tidy {
namespace misc {

void InvalidatedIteratorsCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(declRefExpr(hasDeclaration(varDecl(allOf(hasAncestor(functionDecl().bind("BlockFunc")),
                                                              hasParent(declStmt().bind("DeclarationStmt")),
                                                              hasType(referenceType()),
                                                              hasDescendant(declRefExpr(hasType(cxxRecordDecl(hasName("std::vector")))))
                                                             )).bind("Declaration"))).bind("Expr"), this);
}





void InvalidatedIteratorsCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *RefUse  = Result.Nodes.getNodeAs<DeclRefExpr>("Expr");
  const auto *RefDecl = Result.Nodes.getNodeAs<VarDecl>("Declaration");
  const auto *RefDeclStmt = Result.Nodes.getNodeAs<DeclStmt>("DeclarationStmt");
  const auto *Func    = Result.Nodes.getNodeAs<FunctionDecl>("BlockFunc");
  Stmt *FuncBody = Func->getBody();
  //FuncBody->dumpColor();

  CFG::BuildOptions Options;
  Options.AddImplicitDtors = true;
  Options.AddTemporaryDtors = true;

  auto *Context = Result.Context;
  // TODO: go through all Options

  const std::unique_ptr<CFG> TheCFG = CFG::buildCFG(nullptr, FuncBody, Result.Context, Options);
  const std::unique_ptr<StmtToBlockMap> BlockMap(new StmtToBlockMap(TheCFG.get(), Result.Context));
  const std::unique_ptr<ExprSequence> Sequence(new ExprSequence(TheCFG.get(), Result.Context));
  const CFGBlock *Block = BlockMap->blockContainingStmt(RefUse);
  if (!Block) {
    return;
  }

  // TODO: check if it's the correct vector.
  auto PushBackMatcher =
      cxxMemberCallExpr(has(memberExpr(hasDeclaration(cxxMethodDecl(hasName("push_back"))),
                        has(declRefExpr())))).bind("PushBackCall");

  for (const auto &BlockElem : *Block) {
    //BlockElem.getAs<CFGStmt>()->getStmt()->dumpColor();
    auto Results = match(PushBackMatcher, *(BlockElem.getAs<CFGStmt>()->getStmt()), *Context);
    for (const auto &Match : Results) {
      const auto *PushBackUse = Match.getNodeAs<CXXMemberCallExpr>("PushBackCall");
      if (Sequence->potentiallyAfter(PushBackUse, RefDeclStmt) && Sequence->potentiallyAfter(RefUse, PushBackUse)) {
        diag(RefUse->getLocation(), "%0 might be invalidated before the access")
          << RefDecl;
        diag(PushBackUse->getExprLoc(), "possible place of invalidation", DiagnosticIDs::Note);
        return;
      }
    }
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
