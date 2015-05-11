/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include <stdexcept>
#include <iterator>
#include <cstdio>
#include <string>
#include "fwd-partial_range.h"
#include "compiler-addressof.h"
#include "compiler-type_traits.h"

namespace partial_range_detail
{

template <typename T>
static inline auto adl_begin(T &t) -> decltype(begin(t))
{
	return begin(t);
}

template <typename T>
static inline auto adl_end(T &t) -> decltype(end(t))
{
	return end(t);
}

}

template <typename I>
class partial_range_t
{
public:
	typedef I iterator;
	iterator m_begin, m_end;
	partial_range_t(iterator b, iterator e) :
		m_begin(b), m_end(e)
	{
	}
	partial_range_t(const partial_range_t &) = default;
	partial_range_t(partial_range_t &&) = default;
	template <typename T>
		partial_range_t(T &t) :
			m_begin(partial_range_detail::adl_begin(t)), m_end(partial_range_detail::adl_end(t))
	{
	}
	__attribute_warn_unused_result
	iterator begin() const { return m_begin; }
	__attribute_warn_unused_result
	iterator end() const { return m_end; }
	bool empty() const __attribute_warn_unused_result
	{
		return m_begin == m_end;
	}
	__attribute_warn_unused_result
	std::size_t size() const { return std::distance(m_begin, m_end); }
	std::reverse_iterator<iterator> rbegin() const __attribute_warn_unused_result { return std::reverse_iterator<iterator>{m_end}; }
	std::reverse_iterator<iterator> rend() const __attribute_warn_unused_result { return std::reverse_iterator<iterator>{m_begin}; }
	partial_range_t<std::reverse_iterator<iterator>> reversed() const __attribute_warn_unused_result
	{
		return {rbegin(), rend()};
	}
};

#ifdef DXX_HAVE_BOOST_FOREACH
#include <boost/range/const_iterator.hpp>
#include <boost/range/mutable_iterator.hpp>

namespace boost
{
	template <typename iterator>
		struct range_mutable_iterator< partial_range_t<iterator> >
		{
			typedef iterator type;
		};

	template <typename iterator>
		struct range_const_iterator< partial_range_t<iterator> >
		{
			typedef iterator type;
		};
}
#endif

struct base_partial_range_error_t : std::out_of_range
{
	DXX_INHERIT_CONSTRUCTORS(base_partial_range_error_t, std::out_of_range);
#define REPORT_FORMAT_STRING	 "%s:%u: %s %lu past %p end %lu \"%s\""
	template <std::size_t N>
		__attribute_cold
	static void prepare(char (&buf)[N], const char *file, unsigned line, const char *estr, const char *desc, unsigned long expr, const void *t, unsigned long d)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, file, line, desc, expr, t, d, estr);
	}
	static constexpr std::size_t required_buffer_size(std::size_t N)
	{
		return sizeof(REPORT_FORMAT_STRING) + sizeof("65535") + (sizeof("18446744073709551615") * 2) + sizeof("0x0000000000000000") + N;
	}
#undef REPORT_FORMAT_STRING
};

template <typename T>
struct partial_range_error_t : base_partial_range_error_t
{
	using base_partial_range_error_t::required_buffer_size;
	DXX_INHERIT_CONSTRUCTORS(partial_range_error_t, base_partial_range_error_t);
	template <std::size_t N>
		__attribute_cold
		__attribute_noreturn
	static void report2(const char *file, unsigned line, const char *estr, const char *desc, unsigned long expr, const T &t, unsigned long d);
	template <std::size_t NF, std::size_t NE, std::size_t ND>
		__attribute_cold
		__attribute_noreturn
	static void report(const char (&file)[NF], unsigned line, const char (&estr)[NE], const char (&desc)[ND], unsigned long expr, const T &t, unsigned long d)
	{
		/* Round reporting into large buckets.  Code size is more
		 * important than stack space.
		 */
		report2<(required_buffer_size(NF + NE + ND) | 0xff) + 1>(file, line, estr, desc, expr, t, d);
	}
};

template <typename T>
template <std::size_t N>
void partial_range_error_t<T>::report2(const char *file, unsigned line, const char *estr, const char *desc, unsigned long expr, const T &t, unsigned long d)
{
	char buf[N];
	base_partial_range_error_t::prepare(buf, file, line, estr, desc, expr, addressof(t), d);
	throw partial_range_error_t<T>(buf);
}

namespace partial_range_detail
{

template <typename T, std::size_t NF, std::size_t NE>
static inline void check_range_bounds(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, const std::size_t o, const std::size_t l, const std::size_t d)
{
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
	/*
	 * If EXPR and d are compile-time constant, and the (EXPR > d)
	 * branch is optimized out, then the expansion of
	 * PARTIAL_RANGE_COMPILE_CHECK_BOUND is optimized out, preventing
	 * the compile error.
	 *
	 * If EXPR and d are compile-time constant, and the (EXPR > d)
	 * branch is not optimized out, then this function is guaranteed to
	 * throw if it is ever called.  In that case, the compile fails,
	 * since the program is guaranteed not to work as the programmer
	 * intends.
	 *
	 * If they are not compile-time constant, but the compiler can
	 * optimize based on constants, then it will optimize out the
	 * expansion of PARTIAL_RANGE_COMPILE_CHECK_BOUND, preventing the
	 * compile error.  The function might throw on invalid inputs,
	 * including constant inputs that the compiler failed to recognize
	 * as compile-time constant.
	 *
	 * If the compiler cannot optimize based on the result of
	 * __builtin_constant_p (such as at -O0), then configure tests set
	 * !DXX_HAVE_BUILTIN_CONSTANT_P and the macro expands to nothing.
	 */
#define PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S)	\
	(__builtin_constant_p(EXPR) && __builtin_constant_p(d) && (DXX_ALWAYS_ERROR_FUNCTION(partial_range_will_always_throw, S " will always throw"), 0))
#else
#define PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S)	0
#endif
#define PARTIAL_RANGE_CHECK_BOUND(EXPR,S)	\
	if (EXPR > d)	\
		((void)(PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S))),	\
		partial_range_error_t<const T>::report(file, line, estr, S, EXPR, t, d)
	PARTIAL_RANGE_CHECK_BOUND(o, "begin");
	PARTIAL_RANGE_CHECK_BOUND(l, "end");
#undef PARTIAL_RANGE_CHECK_BOUND
#undef PARTIAL_RANGE_COMPILE_CHECK_BOUND
}

/* C arrays lack a size method, but have a constant size */
template <typename T, std::size_t d>
static constexpr tt::integral_constant<std::size_t, d> get_range_size(T (&)[d])
{
	return {};
}

template <typename T>
static constexpr std::size_t get_range_size(T &t)
{
	return t.size();
}

template <typename T, std::size_t NF, std::size_t NE>
static inline void check_partial_range(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, const std::size_t o, const std::size_t l)
{
	check_range_bounds<T, NF, NE>(file, line, estr, t, o, l, get_range_size(t));
}

}

template <typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> unchecked_partial_range(I range_begin, const std::size_t o, const std::size_t l, tt::true_type)
{
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
	/* Compile-time only check.  Runtime handles (o > l) correctly, and
	 * it can happen in a correct program.  If it is guaranteed to
	 * happen, then the range is always empty, which likely indicates a
	 * bug.
	 */
	if (__builtin_constant_p(!(o < l)) && !(o < l))
		DXX_ALWAYS_ERROR_FUNCTION(partial_range_is_always_empty, "offset never less than length");
#endif
	auto range_end = range_begin;
	/* Use <= so that (o == 0) makes the expression always-true, so the
	 * compiler will optimize out the test.
	 */
	if (o <= l)
	{
		using std::advance;
		advance(range_begin, o);
		advance(range_end, l);
	}
	return {range_begin, range_end};
}

template <typename I>
static inline partial_range_t<I> unchecked_partial_range(I, std::size_t, std::size_t, tt::false_type) = delete;

template <typename I, typename UO, typename UL>
__attribute_warn_unused_result
static inline partial_range_t<I> unchecked_partial_range(I range_begin, const UO &o, const UL &l)
{
	/* Require unsigned length */
	typedef typename tt::conditional<tt::is_unsigned<UO>::value, tt::is_unsigned<UL>, tt::false_type>::type enable_type;
	return unchecked_partial_range<I>(range_begin, o, l, enable_type());
}

template <typename I, typename UL>
__attribute_warn_unused_result
static inline partial_range_t<I> unchecked_partial_range(I range_begin, const UL &l)
{
	return unchecked_partial_range<I, UL, UL>(range_begin, 0, l);
}

template <typename T, typename UO, typename UL, std::size_t NF, std::size_t NE, typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> (partial_range)(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, const UO &o, const UL &l)
{
	partial_range_detail::check_partial_range(file, line, estr, t, o, l);
	auto range_begin = begin(t);
	return unchecked_partial_range<I, UO, UL>(range_begin, o, l);
}

template <typename T, typename UL, std::size_t NF, std::size_t NE, typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> (partial_range)(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, const UL &l)
{
	return partial_range<T, UL, UL, NF, NE, I>(file, line, estr, t, 0, l);
}

template <typename T, typename UO, typename UL, std::size_t NF, std::size_t NE, typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> (partial_const_range)(const char (&file)[NF], unsigned line, const char (&estr)[NE], const T &t, const UO &o, const UL &l)
{
	return partial_range<const T, UO, UL, NF, NE, I>(file, line, estr, t, o, l);
}

template <typename T, typename UL, std::size_t NF, std::size_t NE, typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> (partial_const_range)(const char (&file)[NF], unsigned line, const char (&estr)[NE], const T &t, const UL &l)
{
	return partial_range<const T, UL, NF, NE, I>(file, line, estr, t, l);
}
