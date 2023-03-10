#pragma once

#include "metacpp.hpp"

// Lisp parser
namespace lisp {								// namespace lisp

#define CHARS const metacpp::data::string <Chars...>

// Fundamental types
template <long int X>
struct Int {
	static constexpr long int value = X;
};

template <double X>
struct Float {
	static constexpr double value = X;
};

// Default function dispatcher
template <typename T, int Index>
requires metacpp::data::impl_is_string <T> ::value
struct impl_ftn_dispatcher {
	static constexpr bool success = false;
	static constexpr int next = Index;
};

// Constructor for list types
constexpr char impl_list_cstr[] = "list";
using impl_list_str = metacpp::to_string_t <impl_list_cstr>;

// template <typename T>
// using impl_list_matcher = metacpp::lang::match_string <impl_list_str, T>;

// Parse list
template <typename, int>
struct impl_parse_list {};

template <char... Chars, int Index>
requires (impl_ftn_dispatcher <CHARS, Index> ::success)
struct impl_parse_list <CHARS, Index> {
	// Skip whitespace
	static constexpr int impl_next_start = metacpp::lang::match_whitespace <
		CHARS, impl_ftn_dispatcher <CHARS, Index> ::next
	> ::next;

	using impl_current_type = typename impl_ftn_dispatcher <CHARS, Index> ::type;

	static constexpr int next = impl_parse_list <CHARS, impl_next_start> ::next;
	using type = metacpp::concat_t <
		impl_current_type,
		typename impl_parse_list <CHARS, impl_next_start> ::type
	>;
};

// Until we reach the end of the list (e.g. ')')
template <char... Chars, int Index>
requires (metacpp::lang::match_char <CHARS, ')', Index> ::success)
struct impl_parse_list <CHARS, Index> {
	static constexpr int next = Index + 1;
	using type = metacpp::data::generic_list <>;
};

// Auto type casting
struct auto_type {
	struct eInt {};
	struct eFloat {};
};

template <typename...>
struct impl_auto_cast {
	using type = auto_type::eInt;
};

template <long int X, typename ... Values>
struct impl_auto_cast <Int <X>, Values...> {
	using type = typename impl_auto_cast <
		metacpp::data::generic_list <Values...>
	> ::type;
};

template <double X, typename ... Values>
struct impl_auto_cast <Float <X>, Values...> {
	using type = auto_type::eFloat;
};

// Arithmetic operations
enum class op_type {
	plus,
	minus,
	multiply,
	divide
};

template <op_type Op, typename>
struct impl_op {};

template <typename T>
struct impl_op <op_type::plus, T> {
	static constexpr int impl_value = 0;
};

template <typename T>
struct impl_op <op_type::multiply, T> {
	static constexpr int impl_value = 1;
};

// ADDITION

// Addition with pure integers
template <long int X, typename ... Ts>
requires std::is_same <typename impl_auto_cast <Ts...> ::type, auto_type::eInt> ::value
struct impl_op <op_type::plus, metacpp::data::generic_list <Int <X>, Ts...>> {
	static constexpr long int impl_value = X + impl_op <
		op_type::plus,
		metacpp::data::generic_list <Ts...>
	> ::impl_value;

	using type = Int <impl_value>;
};

// Otherwise, upcast to double
template <typename T, typename ... Ts>
struct impl_op <op_type::plus, metacpp::data::generic_list <T, Ts...>> {
	static constexpr double impl_value = T::value + impl_op <
		op_type::plus,
		metacpp::data::generic_list <Ts...>
	> ::impl_value;

	using type = Float <impl_value>;
};

// MULTIPLICATION

// Multiplication with pure integers
template <long int X, typename ... Ts>
// TODO: avoid the nesting...						over here!
requires std::is_same <typename impl_auto_cast <Ts...> ::type, auto_type::eInt> ::value
struct impl_op <op_type::multiply, metacpp::data::generic_list <Int <X>, Ts...>> {
	static constexpr long int impl_value = X * impl_op <
		op_type::multiply,
		metacpp::data::generic_list <Ts...>
	> ::impl_value;

	using type = Int <impl_value>;
};

// Otherwise, upcast to double
template <typename T, typename ... Ts>
struct impl_op <op_type::multiply, metacpp::data::generic_list <T, Ts...>> {
	static constexpr double impl_value = T::value * impl_op <
		op_type::multiply,
		metacpp::data::generic_list <Ts...>
	> ::impl_value;

	static_assert(impl_value != 0);
	using type = Float <impl_value>;
};

// SUBTRACTION

// Subtraction with pure integers
template <long int X, long int Y>
struct impl_op <op_type::minus, metacpp::data::generic_list <Int <X>, Int <Y>>> {
	static constexpr long int impl_value = X - Y;
	using type = Int <impl_value>;
};

// Otherwise, upcast to double
template <typename X, typename Y>
struct impl_op <op_type::minus, metacpp::data::generic_list <X, Y>> {
	static constexpr double impl_value = double(X::value) - double(Y::value);
	using type = Float <impl_value>;
};

// DIVISION

// Integer result only when perfectly divisible
template <long int X, long int Y>
requires (X % Y == 0)
struct impl_op <op_type::divide, metacpp::data::generic_list <Int <X>, Int <Y>>> {
	static constexpr long int impl_value = X / Y;
	using type = Int <impl_value>;
};

// Otherwise, upcast to double
template <typename X, typename Y>
struct impl_op <op_type::divide, metacpp::data::generic_list <X, Y>> {
	static constexpr double impl_value = double(X::value)/double(Y::value);
	using type = Float <impl_value>;
};

// List dispatcher (starts with 'list')
template <char... Chars, int Index>
requires (metacpp::lang::match_string <CHARS, impl_list_str, Index> ::success)
struct impl_ftn_dispatcher <CHARS, Index> {
	// NOTE: successfuly matched list
	static constexpr int impl_next_start = metacpp::lang::match_whitespace <
		CHARS, metacpp::lang::match_string <CHARS, impl_list_str, Index> ::next
	> ::next;

	static constexpr int next = impl_parse_list <CHARS, impl_next_start> ::next;
	using type = typename impl_parse_list <CHARS, impl_next_start> ::type;

	static constexpr bool success = true;
};

// Addition dispatcher (starts with '+')
template <char ... Chars, int Index>
requires metacpp::lang::match_char <CHARS, '+', Index> ::success
struct impl_ftn_dispatcher <CHARS, Index> {
	// NOTE: successfuly matched plus
	static constexpr int impl_next_start = metacpp::lang::match_whitespace
		<CHARS, Index + 1> ::next;

	using impl_current_type = typename impl_parse_list <CHARS, impl_next_start> ::type;
	static_assert(metacpp::size <impl_current_type> ::value > 0,
		"Expected at least one argument to '+'"
	);

	static constexpr int next = impl_parse_list <CHARS, impl_next_start> ::next;
	using type = typename impl_op <op_type::plus, impl_current_type> ::type;

	static constexpr bool success = true;
};

// Multiplication dispatcher (starts with '*')
template <char... Chars, int Index>
requires metacpp::lang::match_char <CHARS, '*', Index> ::success
struct impl_ftn_dispatcher <CHARS, Index> {
	// NOTE: successfuly matched plus
	static constexpr int impl_next_start = metacpp::lang::match_whitespace
		<CHARS, Index + 1> ::next;

	using impl_current_type = typename impl_parse_list <CHARS, impl_next_start> ::type;
	static_assert(metacpp::size <impl_current_type> ::value > 0,
		"Expected at least one argument to '*'"
	);

	static constexpr int next = impl_parse_list <CHARS, impl_next_start> ::next;
	using type = typename impl_op <op_type::multiply, impl_current_type> ::type;

	static constexpr bool success = true;
};

// Subtraction dispatcher (starts with '-')
template <char... Chars, int Index>
requires metacpp::lang::match_char <CHARS, '-', Index> ::success
struct impl_ftn_dispatcher <CHARS, Index> {
	// NOTE: successfuly matched plus
	static constexpr int impl_next_start = metacpp::lang::match_whitespace
		<CHARS, Index + 1> ::next;

	using impl_current_type = typename impl_parse_list <CHARS, impl_next_start> ::type;
	static_assert(metacpp::size <impl_current_type> ::value == 2,
		"Expected two arguments to '-'"
	);

	static constexpr int next = impl_parse_list <CHARS, impl_next_start> ::next;
	using type = typename impl_op <op_type::minus, impl_current_type> ::type;

	static constexpr bool success = true;
};

// Division dispatcher (starts with '/')
template <char... Chars, int Index>
requires metacpp::lang::match_char <CHARS, '/', Index> ::success
struct impl_ftn_dispatcher <CHARS, Index> {
	// NOTE: successfuly matched plus
	static constexpr int impl_next_start = metacpp::lang::match_whitespace
		<CHARS, Index + 1> ::next;

	using impl_current_type = typename impl_parse_list <CHARS, impl_next_start> ::type;
	static_assert(metacpp::size <impl_current_type> ::value == 2,
		"Expected two arguments to '/'"
	);

	static constexpr int next = impl_parse_list <CHARS, impl_next_start> ::next;
	using type = typename impl_op <op_type::divide, impl_current_type> ::type;

	static constexpr bool success = true;
};

// Expression dispatcher (starts with '(')
template <char... Chars, int Index>
requires (metacpp::lang::match_char <CHARS, '(', Index> ::success)
struct impl_ftn_dispatcher <CHARS, Index> {
	static constexpr int impl_next_start = metacpp::lang::match_whitespace
		<CHARS, Index + 1> ::next;

	static constexpr int next = impl_ftn_dispatcher <CHARS, impl_next_start> ::next;
	using type = metacpp::data::generic_list <
		typename impl_ftn_dispatcher <CHARS, impl_next_start> ::type
	>;

	static constexpr bool success = true;
};

// Integer dispatcher
template <char... Chars, int Index>
requires (metacpp::lang::match_float <CHARS, double, Index> ::success
	&& !metacpp::lang::match_float <CHARS, double, Index> ::dot)
struct impl_ftn_dispatcher <CHARS, Index> {
	// Skip whitespace
	static constexpr int next = metacpp::lang::match_whitespace <
		CHARS, metacpp::lang::match_int <CHARS, int,Index> ::next
	> ::next;

	using type = metacpp::data::generic_list <
		Int <metacpp::lang::match_int <CHARS, int, Index> ::value()>
	>;

	static constexpr bool success = true;
};

// Read float when there is a dot
template <char... Chars, int Index>
requires (metacpp::lang::match_float <CHARS, double,Index> ::success
	&& metacpp::lang::match_float <CHARS, double, Index> ::dot)
struct impl_ftn_dispatcher <CHARS, Index> {
	// Skip whitespace
	static constexpr int next = metacpp::lang::match_whitespace <
		CHARS, metacpp::lang::match_float <CHARS, double, Index> ::next
	> ::next;

	using type = metacpp::data::generic_list <
		Float <metacpp::lang::match_float <CHARS, double, Index> ::value()>
	>;

	static constexpr bool success = true;
};

template <int Index, char ... Chars>
struct impl_is_non_whitespace {
	static constexpr auto impl_array = std::array <bool, sizeof...(Chars)> {
		(Chars == ' ' || Chars == '\t' || Chars == '\n')...
	};

	static constexpr bool impl_value() {
		for (int i = Index; i < sizeof...(Chars); i++) {
			if (!impl_array[i])
				return true;
		}

		return false;
	}

	static constexpr bool value = impl_value();
};

// Final dispatcher (starts at beginning of string)
template <typename, int>
struct eval {
	using type = metacpp::data::generic_list <>;
};

template <char... Chars, int Index>
requires impl_is_non_whitespace <Index, Chars...> ::value
struct eval <CHARS, Index> {
	static constexpr int impl_next_start = metacpp::lang::match_whitespace <CHARS, Index> ::next;
	using impl_current_type = typename impl_ftn_dispatcher <CHARS, impl_next_start> ::type;
	static constexpr int impl_after_current = impl_ftn_dispatcher <CHARS, impl_next_start> ::next;

	using type = metacpp::concat_t <
		impl_current_type,
		typename eval <CHARS, impl_after_current> ::type
	>;

	// NOTE: no next, since we expect to parse the entire string
};

template <typename T, int Index = 0>
using eval_t = typename eval <T, Index> ::type;

}										// namespace lisp

// + Meta overrrides
namespace metacpp::io {

// Printing integers
// TODO: generate at compile time
template <long int X>
struct impl_printf <lisp::Int <X>> {
	static std::string value() {
		return std::to_string(X) + "I";
	}
};

// Printing floats
template <double X>
struct impl_printf <lisp::Float <X>> {
	static std::string value() {
		return std::to_string(X) + "F";
	}
};

}
