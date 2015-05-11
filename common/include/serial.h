/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <tuple>

#include "dxxsconf.h"
#include "compiler-addressof.h"
#include "compiler-array.h"
#include "compiler-integer_sequence.h"
#include "compiler-range_for.h"
#include "compiler-static_assert.h"
#include "compiler-type_traits.h"

namespace serial {

template <typename A1, typename... Args>
class message;

	/* Classifiers to identify whether a type is a message<...> */
template <typename>
class is_message : public tt::false_type
{
};

template <typename A1, typename... Args>
class is_message<message<A1, Args...>> : public tt::true_type
{
};

template <typename T>
class integral_type
{
	static_assert(tt::is_integral<T>::value, "integral_type used on non-integral type");
public:
	static const std::size_t maximum_size = sizeof(T);
};

template <typename T>
class enum_type
{
	static_assert(tt::is_enum<T>::value, "enum_type used on non-enum type");
public:
	static const std::size_t maximum_size = sizeof(T);
};

template <typename>
class is_cxx_array : public tt::false_type
{
};

template <typename T, std::size_t N>
class is_cxx_array<array<T, N>> : public tt::true_type
{
};

template <typename T>
class is_cxx_array<const T> : public is_cxx_array<T>
{
};

template <typename T>
class is_generic_class : public tt::conditional<is_cxx_array<T>::value, tt::false_type, tt::is_class<T>>::type
{
};

template <typename Accessor, typename A1, typename A1rr = typename tt::remove_reference<A1>::type>
static inline typename tt::enable_if<tt::is_integral<A1rr>::value, void>::type process_buffer(Accessor &, A1 &&);

template <typename Accessor, typename A1, typename A1rr = typename tt::remove_reference<A1>::type>
static inline typename tt::enable_if<tt::is_enum<A1rr>::value, void>::type process_buffer(Accessor &, A1 &&);

template <typename Accessor, typename A1, typename A1rr = typename tt::remove_reference<A1>::type>
static inline typename tt::enable_if<is_generic_class<A1rr>::value, void>::type process_buffer(Accessor &, A1 &&);

template <typename Accessor, typename A1>
typename tt::enable_if<is_cxx_array<A1>::value, void>::type process_buffer(Accessor &, A1 &);

template <typename Accessor, typename A1, typename... Args>
static void process_buffer(Accessor &, const message<A1, Args...> &);

template <typename>
class class_type;

template <typename>
class array_type;

struct endian_access
{
	/*
	 * Endian access modes:
	 * - foreign_endian: assume buffered data is foreign endian
	 *   Byte swap regardless of host byte order
	 * - little_endian: assume buffered data is little endian
	 *   Copy on little endian host, byte swap on big endian host
	 * - big_endian: assume buffered data is big endian
	 *   Copy on big endian host, byte swap on little endian host
	 * - native_endian: assume buffered data is native endian
	 *   Copy regardless of host byte order
	 */
	static const uint16_t foreign_endian = 0;
	static const uint16_t little_endian = 255;
	static const uint16_t big_endian = 256;
	static const uint16_t native_endian = 257;
};

	/* Implementation details - avoid namespace pollution */
namespace detail {

	/*
	 * gcc before 4.8 chokes on tuple<tuple<struct {}>> due to ambiguous
	 * base class after applying EBO
	 */
template <typename T>
class wrapped_empty_value : T
{
public:
	wrapped_empty_value() = default;
	wrapped_empty_value(T &&t) : T(std::forward<T>(t)) {}
	T &get() { return *this; }
	const T &get() const { return *this; }
};

template <typename T>
static inline T &extract_value(wrapped_empty_value<T> &t)
{
	return t.get();
}

template <typename T>
static inline const T &extract_value(const wrapped_empty_value<T> &t)
{
	return t.get();
}

template <typename T, typename Trr = typename tt::remove_reference<T>::type>
struct capture_type
{
	typedef
		typename tt::conditional<tt::is_lvalue_reference<T>::value,
			std::reference_wrapper<Trr>,
			typename tt::conditional<tt::is_empty<Trr>::value,
				wrapped_empty_value<Trr>,
				std::tuple<Trr>
			>::type
		>::type type;
};

template <typename T, typename Trr = typename tt::remove_reference<T>::type>
static inline auto capture_value(Trr &t) -> decltype(std::ref(t))
{
	return std::ref(t);
}

template <typename T, typename Trr = typename tt::remove_reference<T>::type>
static inline typename tt::enable_if<tt::is_empty<Trr>::value, detail::wrapped_empty_value<Trr>>::type capture_value(Trr &&t)
{
	return std::forward<Trr>(t);
}

template <typename T, typename Trr = typename tt::remove_reference<T>::type>
static inline typename tt::enable_if<!tt::is_empty<Trr>::value && tt::is_rvalue_reference<T>::value, std::tuple<Trr>>::type capture_value(Trr &&t)
{
	return std::tuple<Trr>{std::forward<T>(t)};
}

template <std::size_t amount, uint8_t value>
class pad_type
{
};

template <std::size_t amount, uint8_t value>
message<array<uint8_t, amount>> udt_to_message(const pad_type<amount, value> &);

/*
 * This can never be instantiated, but will be requested if a UDT
 * specialization is missing.
 */
template <typename T>
struct missing_udt_specialization
{
	missing_udt_specialization() = delete;
};

template <typename T>
void udt_to_message(T &, missing_udt_specialization<T> = missing_udt_specialization<T>());

template <typename Accessor, typename UDT>
void preprocess_udt(Accessor &, UDT &) {}

template <typename Accessor, typename UDT>
void postprocess_udt(Accessor &, UDT &) {}

template <typename Accessor, typename UDT>
static inline void process_udt(Accessor &accessor, UDT &udt)
{
	process_buffer(accessor, udt_to_message(udt));
}

template <typename Accessor, typename E>
void check_enum(Accessor &, E) {}

template <typename T, typename D>
struct base_bytebuffer_t : std::iterator<std::random_access_iterator_tag, T>, endian_access
{
public:
	// Default bytebuffer_t usage to little endian
	static uint16_t endian() { return little_endian; }
	typedef typename std::iterator<std::random_access_iterator_tag, T>::pointer pointer;
	typedef typename std::iterator<std::random_access_iterator_tag, T>::difference_type difference_type;
	base_bytebuffer_t(pointer u) : p(u) {}
	operator pointer() const { return p; }
	D &operator+=(difference_type d)
	{
		p += d;
		return *static_cast<D *>(this);
	}
	operator const void *() const = delete;
protected:
	pointer p;
};

#define SERIAL_UDT_ROUND_UP(X,M)	(((X) + (M) - 1) & ~((M) - 1))
template <std::size_t amount,
	std::size_t SERIAL_UDT_ROUND_MULTIPLIER = sizeof(void *),
	std::size_t SERIAL_UDT_ROUND_UP_AMOUNT = SERIAL_UDT_ROUND_UP(amount, SERIAL_UDT_ROUND_MULTIPLIER),
	std::size_t FULL_SIZE = amount / 64 ? 64 : SERIAL_UDT_ROUND_UP_AMOUNT,
	std::size_t REMAINDER_SIZE = amount % 64>
union pad_storage
{
	static_assert(amount % SERIAL_UDT_ROUND_MULTIPLIER ? SERIAL_UDT_ROUND_UP_AMOUNT > amount && SERIAL_UDT_ROUND_UP_AMOUNT < amount + SERIAL_UDT_ROUND_MULTIPLIER : SERIAL_UDT_ROUND_UP_AMOUNT == amount, "round up error");
	static_assert(SERIAL_UDT_ROUND_UP_AMOUNT % SERIAL_UDT_ROUND_MULTIPLIER == 0, "round modulus error");
	static_assert(amount % FULL_SIZE == REMAINDER_SIZE || FULL_SIZE == REMAINDER_SIZE, "padding alignment error");
	array<uint8_t, FULL_SIZE> f;
	array<uint8_t, REMAINDER_SIZE> p;
	pad_storage(tt::false_type, uint8_t value)
	{
		f.fill(value);
	}
	pad_storage(tt::true_type, uint8_t)
	{
	}
#undef SERIAL_UDT_ROUND_UP
};

template <typename Accessor, std::size_t amount, uint8_t value>
static inline void process_udt(Accessor &accessor, const pad_type<amount, value> &)
{
	/* If reading from accessor, accessor data is const and buffer is
	 * overwritten by read.
	 * If writing to accessor, accessor data is non-const, so initialize
	 * buffer to be written.
	 */
	pad_storage<amount> s(tt::is_const<typename tt::remove_pointer<typename Accessor::pointer>::type>(), value);
	for (std::size_t count = amount; count; count -= s.f.size())
	{
		if (count < s.f.size())
		{
			assert(count == s.p.size());
			process_buffer(accessor, s.p);
			break;
		}
		process_buffer(accessor, s.f);
	}
}

static inline void sequence(std::initializer_list<uint8_t>) {}

template <typename T>
static inline T &extract_value(std::reference_wrapper<T> t)
{
	return t;
}

template <typename T>
static inline T &extract_value(std::tuple<T> &t)
{
	return std::get<0>(t);
}

template <typename T>
static inline const T &extract_value(const std::tuple<T> &t)
{
	return std::get<0>(t);
}

}

template <std::size_t amount, uint8_t value = 0xcc>
static inline detail::pad_type<amount, value> pad()
{
	return {};
}

#define DEFINE_SERIAL_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	DEFINE_SERIAL_CONST_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	DEFINE_SERIAL_MUTABLE_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\

#define _DEFINE_SERIAL_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	template <typename Accessor>	\
	static inline void process_udt(Accessor &accessor, TYPE &NAME)	\
	{	\
		using serial::process_buffer;	\
		process_buffer(accessor, _SERIAL_UDT_UNWRAP_LIST MEMBERLIST);	\
	}	\
	\
	__attribute_unused	\
	static inline auto udt_to_message(TYPE &NAME) -> decltype(serial::make_message MEMBERLIST) { \
		return serial::make_message MEMBERLIST;	\
	}

#define DEFINE_SERIAL_CONST_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	_DEFINE_SERIAL_UDT_TO_MESSAGE(const TYPE, NAME, MEMBERLIST)
#define DEFINE_SERIAL_MUTABLE_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)	\
	_DEFINE_SERIAL_UDT_TO_MESSAGE(TYPE, NAME, MEMBERLIST)

#define ASSERT_SERIAL_UDT_MESSAGE_SIZE(T, SIZE)	\
	assert_equal(serial::class_type<T>::maximum_size, SIZE, "sizeof(" #T ") is not " #SIZE)

template <typename M1, typename T1, typename M1rcv_rr = typename tt::remove_cv<typename tt::remove_reference<M1>::type>::type>
struct udt_message_compatible_same_type : tt::is_same<M1rcv_rr, T1>
{
	static_assert(tt::is_same<M1rcv_rr, T1>::value, "parameter type mismatch");
};

template <bool, typename M, typename T>
class assert_udt_message_compatible2;

template <typename M, typename T>
class assert_udt_message_compatible2<false, M, T> : public tt::false_type
{
};

template <typename M1, typename T1>
class assert_udt_message_compatible2<true, message<M1>, std::tuple<T1>> : public udt_message_compatible_same_type<M1, T1>
{
};

template <typename M1, typename M2, typename... Mn, typename T1, typename T2, typename... Tn>
class assert_udt_message_compatible2<true, message<M1, M2, Mn...>, std::tuple<T1, T2, Tn...>> :
	public assert_udt_message_compatible2<udt_message_compatible_same_type<M1, T1>::value, message<M2, Mn...>, std::tuple<T2, Tn...>>
{
};

template <typename M, typename T>
class assert_udt_message_compatible1;

template <typename M1, typename... Mn, typename T1, typename... Tn>
class assert_udt_message_compatible1<message<M1, Mn...>, std::tuple<T1, Tn...>> : public assert_udt_message_compatible2<sizeof...(Mn) == sizeof...(Tn), message<M1, Mn...>, std::tuple<T1, Tn...>>
{
	static_assert(sizeof...(Mn) <= sizeof...(Tn), "too few types in tuple");
	static_assert(sizeof...(Mn) >= sizeof...(Tn), "too few types in message");
};

template <typename, typename>
class assert_udt_message_compatible;

template <typename C, typename T1, typename... Tn>
class assert_udt_message_compatible<C, std::tuple<T1, Tn...>> : public assert_udt_message_compatible1<typename class_type<C>::as_message, std::tuple<T1, Tn...>>
{
};

#define _SERIAL_UDT_UNWRAP_LIST(A1,...)	A1, ## __VA_ARGS__

#define ASSERT_SERIAL_UDT_MESSAGE_TYPE(T, TYPELIST)	\
	ASSERT_SERIAL_UDT_MESSAGE_CONST_TYPE(T, TYPELIST);	\
	ASSERT_SERIAL_UDT_MESSAGE_MUTABLE_TYPE(T, TYPELIST);	\

#define _ASSERT_SERIAL_UDT_MESSAGE_TYPE(T, TYPELIST)	\
	static_assert(serial::assert_udt_message_compatible<T, std::tuple<_SERIAL_UDT_UNWRAP_LIST TYPELIST>>::value, "udt/message mismatch")

#define ASSERT_SERIAL_UDT_MESSAGE_CONST_TYPE(T, TYPELIST)	\
	_ASSERT_SERIAL_UDT_MESSAGE_TYPE(const T, TYPELIST)
#define ASSERT_SERIAL_UDT_MESSAGE_MUTABLE_TYPE(T, TYPELIST)	\
	_ASSERT_SERIAL_UDT_MESSAGE_TYPE(T, TYPELIST)

union endian_skip_byteswap_u
{
	uint8_t c[2];
	uint16_t s;
	constexpr endian_skip_byteswap_u(const uint16_t &u) : s(u)
	{
		static_assert(offsetof(endian_skip_byteswap_u, c) == offsetof(endian_skip_byteswap_u, s), "union layout error");
	}
};

static inline constexpr uint8_t endian_skip_byteswap(const uint16_t &endian)
{
	return endian_skip_byteswap_u{endian}.c[0];
}

template <typename T, std::size_t N>
union unaligned_storage
{
	T a;
	uint8_t u[N];
	assert_equal(sizeof(a), sizeof(u), "sizeof(T) is not N");
};

template <typename T, typename = void>
class message_dispatch_type;

template <typename T>
class message_dispatch_type<T, typename tt::enable_if<tt::is_integral<T>::value, void>::type>
{
protected:
	typedef integral_type<T> effective_type;
};

template <typename T>
class message_dispatch_type<T, typename tt::enable_if<tt::is_enum<T>::value, void>::type>
{
protected:
	typedef enum_type<T> effective_type;
};

template <typename T>
class message_dispatch_type<T, typename tt::enable_if<is_cxx_array<T>::value, void>::type>
{
protected:
	typedef array_type<T> effective_type;
};

template <typename T>
class message_dispatch_type<T, typename tt::enable_if<is_generic_class<T>::value && !is_message<T>::value, void>::type>
{
protected:
	typedef class_type<T> effective_type;
};

template <typename T>
class message_type : message_dispatch_type<typename tt::remove_reference<T>::type>
{
	typedef message_dispatch_type<typename tt::remove_reference<T>::type> base_type;
	typedef typename base_type::effective_type effective_type;
public:
	static const std::size_t maximum_size = effective_type::maximum_size;
};

template <typename A1>
class message_dispatch_type<message<A1>, void>
{
protected:
	typedef message_type<A1> effective_type;
public:
	typedef message<A1> as_message;
};

template <typename T>
class class_type : public message_type<decltype(udt_to_message(std::forward<T>(*static_cast<T*>(nullptr))))>
{
};

template <typename T, std::size_t N>
class array_type<const array<T, N>>
{
public:
	static const std::size_t maximum_size = message_type<T>::maximum_size * N;
};

template <typename T, std::size_t N>
class array_type<array<T, N>> : public array_type<const array<T, N>>
{
};

template <typename A1, typename A2, typename... Args>
class message_type<message<A1, A2, Args...>>
{
public:
	typedef message<A1, A2, Args...> as_message;
	static const std::size_t maximum_size = message_type<A1>::maximum_size + message_type<message<A2, Args...>>::maximum_size;
};

template <typename A1, typename... Args>
class message
{
	typedef std::tuple<typename detail::capture_type<A1 &&>::type, typename detail::capture_type<Args &&>::type...> tuple_type;
	template <typename T1>
		static void check_type()
		{
			static_assert(message_type<T1>::maximum_size > 0, "empty field in message");
		}
	static void check_types()
	{
		check_type<A1>();
		detail::sequence({(check_type<Args>(), static_cast<uint8_t>(0))...});
	}
	tuple_type t;
public:
	message(A1 &&a1, Args &&... args) :
		t(detail::capture_value<A1>(std::forward<A1>(a1)), detail::capture_value<Args>(std::forward<Args>(args))...)
	{
		check_types();
	}
	const tuple_type &get_tuple() const
	{
		return t;
	}
};

template <typename A1, typename... Args>
static inline message<A1 &&, Args &&...> make_message(A1 &&a1, Args &&... args)
{
	return {std::forward<A1>(a1), std::forward<Args>(args)...};
}

#define SERIAL_DEFINE_SIZE_SPECIFIC_USWAP_BUILTIN(HBITS,BITS)	\
	static inline constexpr uint##BITS##_t bswap(const uint##BITS##_t &u)	\
	{	\
		return __builtin_bswap##BITS(u);	\
	}

#define SERIAL_DEFINE_SIZE_SPECIFIC_USWAP_EXPLICIT(HBITS,BITS)	\
	static inline constexpr uint##BITS##_t bswap(const uint##BITS##_t &u)	\
	{	\
		return (static_cast<uint##BITS##_t>(bswap(static_cast<uint##HBITS##_t>(u))) << HBITS) |	\
			static_cast<uint##BITS##_t>(bswap(static_cast<uint##HBITS##_t>(u >> HBITS)));	\
	}

#define SERIAL_DEFINE_SIZE_SPECIFIC_BSWAP(HBITS,BITS)	\
	SERIAL_DEFINE_SIZE_SPECIFIC_USWAP(HBITS,BITS);	\
	static inline constexpr int##BITS##_t bswap(const int##BITS##_t &i) \
	{	\
		return bswap(static_cast<uint##BITS##_t>(i));	\
	}

static inline constexpr uint8_t bswap(const uint8_t &u)
{
	return u;
}

static inline constexpr int8_t bswap(const int8_t &u)
{
	return u;
}

#ifdef DXX_HAVE_BUILTIN_BSWAP16
#define SERIAL_DEFINE_SIZE_SPECIFIC_USWAP SERIAL_DEFINE_SIZE_SPECIFIC_USWAP_BUILTIN
#else
#define SERIAL_DEFINE_SIZE_SPECIFIC_USWAP SERIAL_DEFINE_SIZE_SPECIFIC_USWAP_EXPLICIT
#endif

SERIAL_DEFINE_SIZE_SPECIFIC_BSWAP(8, 16);
#undef SERIAL_DEFINE_SIZE_SPECIFIC_USWAP

#ifdef DXX_HAVE_BUILTIN_BSWAP
#define SERIAL_DEFINE_SIZE_SPECIFIC_USWAP SERIAL_DEFINE_SIZE_SPECIFIC_USWAP_BUILTIN
#else
#define SERIAL_DEFINE_SIZE_SPECIFIC_USWAP SERIAL_DEFINE_SIZE_SPECIFIC_USWAP_EXPLICIT
#endif

SERIAL_DEFINE_SIZE_SPECIFIC_BSWAP(16, 32);
SERIAL_DEFINE_SIZE_SPECIFIC_BSWAP(32, 64);

#undef SERIAL_DEFINE_SIZE_SPECIFIC_BSWAP
#undef SERIAL_DEFINE_SIZE_SPECIFIC_USWAP
#undef SERIAL_DEFINE_SIZE_SPECIFIC_USWAP_BUILTIN
#undef SERIAL_DEFINE_SIZE_SPECIFIC_USWAP_EXPLICIT

namespace reader {

class bytebuffer_t : public detail::base_bytebuffer_t<const uint8_t, bytebuffer_t>
{
public:
	bytebuffer_t(pointer u) : base_bytebuffer_t(u) {}
	explicit bytebuffer_t(const bytebuffer_t &) = default;
	bytebuffer_t(bytebuffer_t &&) = default;
};

template <typename A1>
static inline void unaligned_copy(const uint8_t *src, unaligned_storage<A1, 1> &dst)
{
	dst.u[0] = *src;
}

#define SERIAL_DEFINE_SIZE_SPECIFIC_UNALIGNED_COPY(BITS)	\
	template <typename A1>	\
	static inline void unaligned_copy(const uint8_t *src, unaligned_storage<A1, BITS / 8> &dst)	\
	{	\
		std::copy_n(src, sizeof(dst.u), dst.u);	\
	}

SERIAL_DEFINE_SIZE_SPECIFIC_UNALIGNED_COPY(16);
SERIAL_DEFINE_SIZE_SPECIFIC_UNALIGNED_COPY(32);
SERIAL_DEFINE_SIZE_SPECIFIC_UNALIGNED_COPY(64);

#undef SERIAL_DEFINE_SIZE_SPECIFIC_UNALIGNED_COPY

template <typename Accessor, typename A1>
static inline void process_integer(Accessor &buffer, A1 &a1)
{
	using std::advance;
	unaligned_storage<A1, message_type<A1>::maximum_size> u;
	unaligned_copy(buffer, u);
	a1 = endian_skip_byteswap(buffer.endian()) ? u.a : bswap(u.a);
	advance(buffer, sizeof(u.u));
}

template <typename Accessor, typename A, typename T = typename A::value_type>
static inline typename tt::enable_if<sizeof(T) == 1 && tt::is_integral<T>::value, void>::type process_array(Accessor &accessor, A &a)
{
	std::copy_n(static_cast<typename Accessor::pointer>(accessor), a.size(), &a[0]);
	advance(accessor, a.size());
}

}

namespace writer {

class bytebuffer_t : public detail::base_bytebuffer_t<uint8_t, bytebuffer_t>
{
public:
	bytebuffer_t(pointer u) : base_bytebuffer_t(u) {}
	explicit bytebuffer_t(const bytebuffer_t &) = default;
	bytebuffer_t(bytebuffer_t &&) = default;
};

template <typename A1>
static inline void unaligned_copy(const unaligned_storage<A1, 1> &src, uint8_t *dst)
{
	*dst = src.u[0];
}

/* If inline unaligned_copy, gcc inlining of copy_n creates a loop instead
 * of a store.
 */
#define SERIAL_DEFINE_SIZE_SPECIFIC_UNALIGNED_COPY(BITS)	\
	template <typename A1>	\
	static inline void unaligned_copy(unaligned_storage<A1, BITS / 8> src, uint8_t *dst)	\
	{	\
		std::copy_n(src.u, sizeof(src.u), dst);	\
	}

SERIAL_DEFINE_SIZE_SPECIFIC_UNALIGNED_COPY(16);
SERIAL_DEFINE_SIZE_SPECIFIC_UNALIGNED_COPY(32);
SERIAL_DEFINE_SIZE_SPECIFIC_UNALIGNED_COPY(64);

#undef SERIAL_DEFINE_SIZE_SPECIFIC_UNALIGNED_COPY

template <typename Accessor, typename A1>
static inline void process_integer(Accessor &buffer, const A1 &a1)
{
	using std::advance;
	unaligned_storage<A1, message_type<A1>::maximum_size> u{endian_skip_byteswap(buffer.endian()) ? a1 : bswap(a1)};
	unaligned_copy(u, buffer);
	advance(buffer, sizeof(u.u));
}

template <typename Accessor, typename A, typename T = typename A::value_type>
static inline typename tt::enable_if<sizeof(T) == 1 && tt::is_integral<T>::value, void>::type process_array(Accessor &accessor, const A &a)
{
	std::copy_n(&a[0], a.size(), static_cast<typename Accessor::pointer>(accessor));
	advance(accessor, a.size());
}

}

template <typename Accessor, typename A1, typename A1rr>
static inline typename tt::enable_if<tt::is_integral<A1rr>::value, void>::type process_buffer(Accessor &accessor, A1 &&a1)
{
	process_integer(accessor, a1);
}

template <typename Accessor, typename A1, typename A1rr>
static inline typename tt::enable_if<tt::is_enum<A1rr>::value, void>::type process_buffer(Accessor &accessor, A1 &&a1)
{
	using detail::check_enum;
	process_integer(accessor, a1);
	/* Hook for enum types to check that the given value is legal */
	check_enum(accessor, a1);
}

template <typename Accessor, typename A1, typename A1rr>
static inline typename tt::enable_if<is_generic_class<A1rr>::value, void>::type process_buffer(Accessor &accessor, A1 &&a1)
{
	using detail::preprocess_udt;
	using detail::process_udt;
	using detail::postprocess_udt;
	preprocess_udt(accessor, a1);
	process_udt(accessor, std::forward<A1>(a1));
	postprocess_udt(accessor, a1);
}

template <typename Accessor, typename A, typename T = typename A::value_type>
static typename tt::enable_if<!(sizeof(T) == 1 && tt::is_integral<T>::value), void>::type process_array(Accessor &accessor, A &a)
{
	range_for (auto &i, a)
		process_buffer(accessor, i);
}

template <typename Accessor, typename A1>
typename tt::enable_if<is_cxx_array<A1>::value, void>::type process_buffer(Accessor &accessor, A1 &a1)
{
	process_array(accessor, a1);
}

template <typename Accessor, typename... Args, std::size_t... N>
static inline void process_message_tuple(Accessor &accessor, const std::tuple<Args...> &t, index_sequence<N...>)
{
	detail::sequence({(process_buffer(accessor, detail::extract_value(std::get<N>(t))), static_cast<uint8_t>(0))...});
}

template <typename Accessor, typename A1, typename... Args>
static void process_buffer(Accessor &accessor, const message<A1, Args...> &m)
{
	process_message_tuple(accessor, m.get_tuple(), make_tree_index_sequence<1 + sizeof...(Args)>());
}

/* Require at least two arguments to prevent self-selection */
template <typename Accessor, typename A1, typename A2, typename... An>
static void process_buffer(Accessor &accessor, A1 &&a1, A2 &&a2, An &&... an)
{
	detail::sequence({
		(process_buffer(accessor, std::forward<A1>(a1)),
		 process_buffer(accessor, std::forward<A2>(a2)), static_cast<uint8_t>(0)),
		(process_buffer(accessor, std::forward<An>(an)), static_cast<uint8_t>(0))...
	});
}

}
