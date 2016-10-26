set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyBoostModule
  BoostTidyModule.cpp
  LexicalCastRedundantTypeCheck.cpp
  UseToStringCheck.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyUtils
  )
