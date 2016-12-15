//===--- InvalidRangeCheck.cpp - clang-tidy--------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "InvalidRangeCheck.h"
#include "../utils/OptionsUtils.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace obvious {

const std::string CXX_AlgorithmNames =
    "std::for_each; std::find; std::find_if; std::find_end; "
    "std::find_first_of; std::adjacent_find; std::count; std::count_if;"
    "std::mismatch; std::equal; std::search; std::copy; "
    "std::copy_backward; std::swap_ranges; std::transform; std::replace"
    "std::replace_if; std::replace_copy; std::replace_copy_if; std::fill; "
    "std::fill_n; std::generate; std::generate_n; std::remove; std::remove_if"
    "std::remove_copy; std::remove_copy_if; std::unique; std::unique_copy;"
    "std::reverse; std::reverse_copy; std::rotate; std::rotate_copy; "
    "std::random_shuffle; std::partition; std::stable_partition; std::sort;"
    "std::stable_sort; std::partial_sort; std::partial_sort_copy; "
    "std::nth_element; std::lower_bound; std::upper_bound; std::equal_range;"
    "std::binary_search; std::merge; std::inplace_merge; std::includes; "
    "std::set_union; std::set_intersection; std::set_difference; "
    "std::set_symetric_difference; std::push_heap; std::pop_heap"
    "std::make_heap; std::sort_heap; std::min; std::max; std::min_element; "
    "std::max_element; std::lexicographical_compare; std::next_permutation; "
    "std::prev_permutation";

const auto CXX11_AlgorithmNames =
    CXX_AlgorithmNames + "; "
    "std::all_of; std::any_of; std::none_of; std::find_if_not; "
    "std::is_permutation; std::copy_n; std::copy_if; std::move; "
    "std::move_backward; std::shuffle; std::is_partitioned;"
    "std::partition_copy; std::partition_point; std::is_sorted"
    "std::is_sorted_until; std::is_heap; std::is_heap_until; std::minmax; "
    "std::minmax_element";

InvalidRangeCheck::InvalidRangeCheck(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      AlgorithmNames(utils::options::parseStringList(Options.get(
          "AlgorithmNames", getLangOpts().CPlusPlus11 ? CXX11_AlgorithmNames
                                                      : CXX_AlgorithmNames))) {}

void InvalidRangeCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus)
    return;

  // Not so small vector. 80 because there are about that many algorithms.
  const auto Names =
      SmallVector<StringRef, 80>(AlgorithmNames.begin(), AlgorithmNames.end());

  auto CallsAlgorithm = hasDeclaration(
      functionDecl(Names.size() > 0 ? hasAnyName(Names) : anything()));

  auto MemberCallWithName = [](StringRef MethodName, StringRef BindThisName) {
    return cxxMemberCallExpr(
        hasDeclaration(cxxMethodDecl(hasName(MethodName))),
        onImplicitObjectArgument(declRefExpr().bind(BindThisName)));
  };
  // FIXME: match for std::begin(arg) and std::end(arg2)
  Finder->addMatcher(
      callExpr(hasArgument(0, MemberCallWithName("begin", "first_arg")),
               hasArgument(1, MemberCallWithName("end", "second_arg")),
               CallsAlgorithm),
      this);
}

void InvalidRangeCheck::check(const MatchFinder::MatchResult &Result) {

  const auto *FirstArg = Result.Nodes.getNodeAs<DeclRefExpr>("first_arg");
  const auto *SecondArg = Result.Nodes.getNodeAs<DeclRefExpr>("second_arg");
  if (FirstArg->getNameInfo().getAsString() ==
      SecondArg->getNameInfo().getAsString())
    return;

  diag(FirstArg->getExprLoc(),
       "call to algorithm with begin and end from different objects");
}

void InvalidRangeCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "AlgorithmNames",
                utils::options::serializeStringList(AlgorithmNames));
}

} // namespace obvious
} // namespace tidy
} // namespace clang
