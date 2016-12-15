set(LLVM_LINK_COMPONENTS support)

add_clang_library(clangTidyObviousModule
  ObviousTidyModule.cpp
  InvalidRangeCheck.cpp

  LINK_LIBS
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  clangTidy
  clangTidyUtils
  )
