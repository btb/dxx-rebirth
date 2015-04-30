/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include "dxxsconf.h"
#include "compiler-begin.h"

template <typename I>
class partial_range_t;

template <typename I, typename UO, typename UL>
__attribute_warn_unused_result
static inline partial_range_t<I> unchecked_partial_range(I range_begin, const UO &o, const UL &l);

template <typename I, typename UL>
__attribute_warn_unused_result
static inline partial_range_t<I> unchecked_partial_range(I range_begin, const UL &l);

template <typename T, typename UO, typename UL, std::size_t NF, std::size_t NE, typename I = decltype(begin(std::declval<T &>()))>
__attribute_warn_unused_result
static inline partial_range_t<I> partial_range(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, const UO &o, const UL &l);

template <typename T, typename UL, std::size_t NF, std::size_t NE, typename I = decltype(begin(std::declval<T &>()))>
__attribute_warn_unused_result
static inline partial_range_t<I> partial_range(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, const UL &l);

template <typename T, typename UO, typename UL, std::size_t NF, std::size_t NE, typename I = decltype(begin(std::declval<const T &>()))>
__attribute_warn_unused_result
static inline partial_range_t<I> partial_const_range(const char (&file)[NF], unsigned line, const char (&estr)[NE], const T &t, const UO &o, const UL &l);

template <typename T, typename UL, std::size_t NF, std::size_t NE, typename I = decltype(begin(std::declval<const T &>()))>
__attribute_warn_unused_result
static inline partial_range_t<I> partial_const_range(const char (&file)[NF], unsigned line, const char (&estr)[NE], const T &t, const UL &l);

/* Explicitly block use on rvalue t because returned partial_range_t<I>
 * will outlive the rvalue.
 */
template <typename T, typename UO, typename UL, std::size_t NF, std::size_t NE, typename I = decltype(begin(std::declval<T &&>()))>
static inline partial_range_t<I> partial_const_range(const char (&file)[NF] __attribute_unused, unsigned line __attribute_unused, const char (&estr)[NE] __attribute_unused, T &&t __attribute_unused, const UO &o __attribute_unused, const UL &l __attribute_unused) = delete;

template <typename T, typename UL, std::size_t NF, std::size_t NE, typename I = decltype(begin(std::declval<T &&>()))>
static inline partial_range_t<I> partial_const_range(const char (&file)[NF] __attribute_unused, unsigned line __attribute_unused, const char (&estr)[NE] __attribute_unused, T &&t __attribute_unused, const UL &l __attribute_unused) = delete;

#define partial_range(T,...)	partial_range(__FILE__, __LINE__, #T, T, ##__VA_ARGS__)
#define partial_const_range(T,...)	partial_const_range(__FILE__, __LINE__, #T, T, ##__VA_ARGS__)
