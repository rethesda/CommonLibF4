#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <compare>
#include <concepts>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <execution>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

static_assert(
	std::is_integral_v<std::time_t> &&
		sizeof(std::time_t) == sizeof(std::size_t),
	"wrap std::time_t instead");

#pragma warning(push)
#include <boost/atomic.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iterator/function_input_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/predef.h>
#include <boost/stl_interfaces/iterator_interface.hpp>
#include <boost/stl_interfaces/sequence_container_interface.hpp>
#include <spdlog/spdlog.h>
#pragma warning(pop)

#if BOOST_COMP_CLANG
// offsetof is required to evaluate to a constant expression, but msvc's cstddef does not
// define it as a constant expression (it probably uses compiler hooks to evaulate it as
// one)
// clang chokes on this, so we redefine the macro to use clang's builtins, just as its defined
// in libc++
#undef offsetof
#define offsetof(a_type, a_member) __builtin_offsetof(a_type, a_member)
#endif

#include "F4SE/Impl/WinAPI.h"

namespace F4SE
{
	using namespace std::literals;

	namespace stl
	{
		template <class CharT>
		using basic_zstring = std::basic_string_view<CharT>;

		using zstring = basic_zstring<char>;
		using zwstring = basic_zstring<wchar_t>;

		// owning pointer
		template <
			class T,
			class = std::enable_if_t<
				std::is_pointer_v<T>>>
		using owner = T;

		// non-owning pointer
		template <
			class T,
			class = std::enable_if_t<
				std::is_pointer_v<T>>>
		using observer = T;

		// non-null pointer
		template <
			class T,
			class = std::enable_if_t<
				std::is_pointer_v<T>>>
		using not_null = T;

		namespace nttp
		{
			template <class CharT, std::size_t N>
			struct string
			{
				using char_type = CharT;
				using pointer = char_type*;
				using const_pointer = const char_type*;
				using reference = char_type&;
				using const_reference = const char_type&;
				using size_type = std::size_t;

				consteval string(const_pointer a_string) noexcept
				{
					for (size_type i = 0; i < N; ++i) {
						c[i] = a_string[i];
					}
				}

				[[nodiscard]] consteval const_reference operator[](size_type a_pos) const noexcept
				{
					assert(a_pos < N);
					return c[a_pos];
				}

				[[nodiscard]] consteval const_reference back() const noexcept { return (*this)[size() - 1]; }
				[[nodiscard]] consteval const_pointer data() const noexcept { return c; }
				[[nodiscard]] consteval const_reference front() const noexcept { return (*this)[0]; }
				[[nodiscard]] consteval size_type length() const noexcept { return N; }
				[[nodiscard]] consteval size_type size() const noexcept { return length(); }

				char_type c[N]{ static_cast<char_type>('\0') };
			};

			template <class CharT, std::size_t N>
			string(const CharT (&)[N]) -> string<CharT, N - 1>;
		}

		struct source_location
		{
		public:
			constexpr source_location() noexcept = default;
			constexpr source_location(const source_location&) noexcept = default;
			constexpr source_location(source_location&&) noexcept = default;

			~source_location() noexcept = default;

			constexpr source_location& operator=(const source_location&) noexcept = default;
			constexpr source_location& operator=(source_location&&) noexcept = default;

			[[nodiscard]] static constexpr source_location current(
				std::uint_least32_t a_line = __builtin_LINE(),
				std::uint_least32_t a_column = __builtin_COLUMN(),
				const char* a_fileName = __builtin_FILE(),
				const char* a_functionName = __builtin_FUNCTION()) noexcept { return { a_line, a_column, a_fileName, a_functionName }; }

			[[nodiscard]] constexpr std::uint_least32_t line() const noexcept { return _line; }
			[[nodiscard]] constexpr std::uint_least32_t column() const noexcept { return _column; }
			[[nodiscard]] constexpr const char* file_name() const noexcept { return _fileName; }
			[[nodiscard]] constexpr const char* function_name() const noexcept { return _functionName; }

		protected:
			constexpr source_location(
				std::uint_least32_t a_line,
				std::uint_least32_t a_column,
				const char* a_fileName,
				const char* a_functionName) noexcept :
				_line(a_line),
				_column(a_column),
				_fileName(a_fileName),
				_functionName(a_functionName)
			{}

		private:
			std::uint_least32_t _line{ 0 };
			std::uint_least32_t _column{ 0 };
			const char* _fileName{ "" };
			const char* _functionName{ "" };
		};

		template <
			class Enum,
			class Underlying = std::underlying_type_t<Enum>>
		class enumeration
		{
		public:
			using enum_type = Enum;
			using underlying_type = Underlying;

			static_assert(std::is_enum_v<enum_type>, "enum_type must be an enum");
			static_assert(std::is_integral_v<underlying_type>, "underlying_type must be an integral");

			constexpr enumeration() noexcept = default;

			constexpr enumeration(const enumeration&) noexcept = default;

			constexpr enumeration(enumeration&&) noexcept = default;

			template <class U2>  // NOLINTNEXTLINE(google-explicit-constructor)
			constexpr enumeration(enumeration<Enum, U2> a_rhs) noexcept :
				_impl(static_cast<underlying_type>(a_rhs.get()))
			{}

			template <class... Args>
			constexpr enumeration(Args... a_values) noexcept  //
				requires(std::same_as<Args, enum_type>&&...) :
				_impl((static_cast<underlying_type>(a_values) | ...))
			{}

			~enumeration() noexcept = default;

			constexpr enumeration& operator=(const enumeration&) noexcept = default;
			constexpr enumeration& operator=(enumeration&&) noexcept = default;

			template <class U2>
			constexpr enumeration& operator=(enumeration<Enum, U2> a_rhs) noexcept
			{
				_impl = static_cast<underlying_type>(a_rhs.get());
			}

			constexpr enumeration& operator=(enum_type a_value) noexcept
			{
				_impl = static_cast<underlying_type>(a_value);
				return *this;
			}

			[[nodiscard]] explicit constexpr operator bool() const noexcept { return _impl != static_cast<underlying_type>(0); }

			[[nodiscard]] constexpr enum_type operator*() const noexcept { return get(); }
			[[nodiscard]] constexpr enum_type get() const noexcept { return static_cast<enum_type>(_impl); }
			[[nodiscard]] constexpr underlying_type underlying() const noexcept { return _impl; }

			template <class... Args>
			constexpr enumeration& set(Args... a_args) noexcept  //
				requires(std::same_as<Args, enum_type>&&...)
			{
				_impl |= (static_cast<underlying_type>(a_args) | ...);
				return *this;
			}

			template <class... Args>
			constexpr enumeration& reset(Args... a_args) noexcept  //
				requires(std::same_as<Args, enum_type>&&...)
			{
				_impl &= ~(static_cast<underlying_type>(a_args) | ...);
				return *this;
			}

			template <class... Args>
			[[nodiscard]] constexpr bool any(Args... a_args) const noexcept  //
				requires(std::same_as<Args, enum_type>&&...)
			{
				return (_impl & (static_cast<underlying_type>(a_args) | ...)) != static_cast<underlying_type>(0);
			}

			template <class... Args>
			[[nodiscard]] constexpr bool all(Args... a_args) const noexcept  //
				requires(std::same_as<Args, enum_type>&&...)
			{
				return (_impl & (static_cast<underlying_type>(a_args) | ...)) == (static_cast<underlying_type>(a_args) | ...);
			}

			template <class... Args>
			[[nodiscard]] constexpr bool none(Args... a_args) const noexcept  //
				requires(std::same_as<Args, enum_type>&&...)
			{
				return (_impl & (static_cast<underlying_type>(a_args) | ...)) == static_cast<underlying_type>(0);
			}

		private :
			underlying_type _impl{ 0 };
		};

		template <class... Args>
		enumeration(Args...) -> enumeration<
			std::common_type_t<Args...>,
			std::underlying_type_t<
				std::common_type_t<Args...>>>;
	}
}

#define F4SE_MAKE_LOGICAL_OP(a_op)                                                                          \
	template <                                                                                              \
		class E,                                                                                            \
		class U1,                                                                                           \
		class U2>                                                                                           \
	[[nodiscard]] constexpr bool operator a_op(enumeration<E, U1> a_lhs, enumeration<E, U2> a_rhs) noexcept \
	{                                                                                                       \
		return a_lhs.get() a_op a_rhs.get();                                                                \
	}                                                                                                       \
                                                                                                            \
	template <                                                                                              \
		class E,                                                                                            \
		class U>                                                                                            \
	[[nodiscard]] constexpr bool operator a_op(enumeration<E, U> a_lhs, E a_rhs) noexcept                   \
	{                                                                                                       \
		return a_lhs.get() a_op a_rhs;                                                                      \
	}                                                                                                       \
                                                                                                            \
	template <                                                                                              \
		class E,                                                                                            \
		class U>                                                                                            \
	[[nodiscard]] constexpr bool operator a_op(E a_lhs, enumeration<E, U> a_rhs) noexcept                   \
	{                                                                                                       \
		return a_lhs a_op a_rhs.get();                                                                      \
	}

#define F4SE_MAKE_ARITHMETIC_OP(a_op)                                                        \
	template <                                                                               \
		class E,                                                                             \
		class U>                                                                             \
	[[nodiscard]] constexpr auto operator a_op(enumeration<E, U> a_enum, U a_shift) noexcept \
		->enumeration<E, U>                                                                  \
	{                                                                                        \
		return static_cast<E>(static_cast<U>(a_enum.get()) a_op a_shift);                    \
	}                                                                                        \
                                                                                             \
	template <                                                                               \
		class E,                                                                             \
		class U>                                                                             \
	constexpr auto operator a_op##=(enumeration<E, U>& a_enum, U a_shift) noexcept           \
		->enumeration<E, U>&                                                                 \
	{                                                                                        \
		return a_enum = a_enum a_op a_shift;                                                 \
	}

#define F4SE_MAKE_ENUMERATION_OP(a_op)                                                                      \
	template <                                                                                              \
		class E,                                                                                            \
		class U1,                                                                                           \
		class U2>                                                                                           \
	[[nodiscard]] constexpr auto operator a_op(enumeration<E, U1> a_lhs, enumeration<E, U2> a_rhs) noexcept \
		->enumeration<E, std::common_type_t<U1, U2>>                                                        \
	{                                                                                                       \
		return static_cast<E>(static_cast<U1>(a_lhs.get()) a_op static_cast<U2>(a_rhs.get()));              \
	}                                                                                                       \
                                                                                                            \
	template <                                                                                              \
		class E,                                                                                            \
		class U>                                                                                            \
	[[nodiscard]] constexpr auto operator a_op(enumeration<E, U> a_lhs, E a_rhs) noexcept                   \
		->enumeration<E, U>                                                                                 \
	{                                                                                                       \
		return static_cast<E>(static_cast<U>(a_lhs.get()) a_op static_cast<U>(a_rhs));                      \
	}                                                                                                       \
                                                                                                            \
	template <                                                                                              \
		class E,                                                                                            \
		class U>                                                                                            \
	[[nodiscard]] constexpr auto operator a_op(E a_lhs, enumeration<E, U> a_rhs) noexcept                   \
		->enumeration<E, U>                                                                                 \
	{                                                                                                       \
		return static_cast<E>(static_cast<U>(a_lhs) a_op static_cast<U>(a_rhs.get()));                      \
	}                                                                                                       \
                                                                                                            \
	template <                                                                                              \
		class E,                                                                                            \
		class U1,                                                                                           \
		class U2>                                                                                           \
	constexpr auto operator a_op##=(enumeration<E, U1>& a_lhs, enumeration<E, U2> a_rhs) noexcept           \
		->enumeration<E, U1>&                                                                               \
	{                                                                                                       \
		return a_lhs = a_lhs a_op a_rhs;                                                                    \
	}                                                                                                       \
                                                                                                            \
	template <                                                                                              \
		class E,                                                                                            \
		class U>                                                                                            \
	constexpr auto operator a_op##=(enumeration<E, U>& a_lhs, E a_rhs) noexcept                             \
		->enumeration<E, U>&                                                                                \
	{                                                                                                       \
		return a_lhs = a_lhs a_op a_rhs;                                                                    \
	}                                                                                                       \
                                                                                                            \
	template <                                                                                              \
		class E,                                                                                            \
		class U>                                                                                            \
	constexpr auto operator a_op##=(E& a_lhs, enumeration<E, U> a_rhs) noexcept                             \
		->E&                                                                                                \
	{                                                                                                       \
		return a_lhs = *(a_lhs a_op a_rhs);                                                                 \
	}

#define F4SE_MAKE_INCREMENTER_OP(a_op)                                                       \
	template <                                                                               \
		class E,                                                                             \
		class U>                                                                             \
	constexpr auto operator a_op##a_op(enumeration<E, U>& a_lhs) noexcept                    \
		->enumeration<E, U>&                                                                 \
	{                                                                                        \
		return a_lhs a_op## = static_cast<E>(1);                                             \
	}                                                                                        \
                                                                                             \
	template <                                                                               \
		class E,                                                                             \
		class U>                                                                             \
	[[nodiscard]] constexpr auto operator a_op##a_op(enumeration<E, U>& a_lhs, int) noexcept \
		->enumeration<E, U>                                                                  \
	{                                                                                        \
		const auto tmp = a_lhs;                                                              \
		a_op##a_op a_lhs;                                                                    \
		return tmp;                                                                          \
	}

namespace F4SE
{
	namespace stl
	{
		template <
			class E,
			class U>
		[[nodiscard]] constexpr auto operator~(enumeration<E, U> a_enum) noexcept
			-> enumeration<E, U>
		{
			return static_cast<E>(~static_cast<U>(a_enum.get()));
		}

		F4SE_MAKE_LOGICAL_OP(==);
		F4SE_MAKE_LOGICAL_OP(!=);
		F4SE_MAKE_LOGICAL_OP(<);
		F4SE_MAKE_LOGICAL_OP(<=);
		F4SE_MAKE_LOGICAL_OP(>);
		F4SE_MAKE_LOGICAL_OP(>=);

		F4SE_MAKE_ARITHMETIC_OP(<<);
		F4SE_MAKE_ENUMERATION_OP(<<);
		F4SE_MAKE_ARITHMETIC_OP(>>);
		F4SE_MAKE_ENUMERATION_OP(>>);

		F4SE_MAKE_ENUMERATION_OP(|);
		F4SE_MAKE_ENUMERATION_OP(&);
		F4SE_MAKE_ENUMERATION_OP(^);

		F4SE_MAKE_ENUMERATION_OP(+);
		F4SE_MAKE_ENUMERATION_OP(-);

		F4SE_MAKE_INCREMENTER_OP(+);  // ++
		F4SE_MAKE_INCREMENTER_OP(-);  // --

		template <class T>
		class atomic_ref :
			public boost::atomic_ref<std::remove_cv_t<T>>
		{
		private:
			using super = boost::atomic_ref<std::remove_cv_t<T>>;

		public:
			using value_type = typename super::value_type;

			explicit atomic_ref(T& a_obj) noexcept(std::is_nothrow_constructible_v<super, value_type&>) :
				super(const_cast<value_type&>(a_obj))
			{}

			using super::super;
			using super::operator=;
		};

		template <class T>
		atomic_ref(T&) -> atomic_ref<T>;

		template class atomic_ref<std::int8_t>;
		template class atomic_ref<std::uint8_t>;
		template class atomic_ref<std::int16_t>;
		template class atomic_ref<std::uint16_t>;
		template class atomic_ref<std::int32_t>;
		template class atomic_ref<std::uint32_t>;
		template class atomic_ref<std::int64_t>;
		template class atomic_ref<std::uint64_t>;

		static_assert(atomic_ref<std::int8_t>::is_always_lock_free);
		static_assert(atomic_ref<std::uint8_t>::is_always_lock_free);
		static_assert(atomic_ref<std::int16_t>::is_always_lock_free);
		static_assert(atomic_ref<std::uint16_t>::is_always_lock_free);
		static_assert(atomic_ref<std::int32_t>::is_always_lock_free);
		static_assert(atomic_ref<std::uint32_t>::is_always_lock_free);
		static_assert(atomic_ref<std::int64_t>::is_always_lock_free);
		static_assert(atomic_ref<std::uint64_t>::is_always_lock_free);

		template <class T, class U>
		[[nodiscard]] auto adjust_pointer(U* a_ptr, std::ptrdiff_t a_adjust) noexcept
		{
			auto addr = a_ptr ? reinterpret_cast<std::uintptr_t>(a_ptr) + a_adjust : 0;
			if constexpr (std::is_const_v<U> && std::is_volatile_v<U>) {
				return reinterpret_cast<std::add_cv_t<T>*>(addr);
			} else if constexpr (std::is_const_v<U>) {
				return reinterpret_cast<std::add_const_t<T>*>(addr);
			} else if constexpr (std::is_volatile_v<U>) {
				return reinterpret_cast<std::add_volatile_t<T>*>(addr);
			} else {
				return reinterpret_cast<T*>(addr);
			}
		}

		template <class T>
		void emplace_vtable(T* a_ptr)
		{
			reinterpret_cast<std::uintptr_t*>(a_ptr)[0] = T::VTABLE[0].address();
		}

		template <class T>
		void memzero(volatile T* a_ptr, std::size_t a_size = sizeof(T))
		{
			const auto begin = reinterpret_cast<volatile char*>(a_ptr);
			constexpr char val{ 0 };
			std::fill_n(begin, a_size, val);
		}

		[[noreturn]] inline void report_and_fail(std::string_view a_msg, source_location a_loc = source_location::current())
		{
			const auto body = [&]() {
				constexpr std::array directories{
					"include/"sv,
					"src/"sv,
				};

				const std::filesystem::path p = a_loc.file_name();
				const auto filename = p.generic_string();
				std::string_view fileview = filename;

				constexpr auto npos = std::string::npos;
				std::size_t pos = npos;
				std::size_t off = 0;
				for (const auto& dir : directories) {
					pos = fileview.find(dir);
					if (pos != npos) {
						off = dir.length();
						break;
					}
				}

				if (pos != npos) {
					fileview = fileview.substr(pos + off);
				}

				return fmt::format(FMT_STRING("{}({}): {}"), fileview, a_loc.line(), a_msg);
			}();

			const auto caption = []() -> std::string {
				const auto maxPath = WinAPI::GetMaxPath();
				std::vector<char> buf;
				buf.reserve(maxPath);
				buf.resize(maxPath / 2);
				std::uint32_t result = 0;
				do {
					buf.resize(buf.size() * 2);
					result = WinAPI::GetModuleFileName(
						WinAPI::GetCurrentModule(),
						buf.data(),
						static_cast<std::uint32_t>(buf.size()));
				} while (result && result == buf.size() && buf.size() <= std::numeric_limits<std::uint32_t>::max());

				if (result && result != buf.size()) {
					std::filesystem::path p(buf.begin(), buf.begin() + result);
					return p.filename().string();
				} else {
					return {};
				}
			}();

			spdlog::log(
				spdlog::source_loc{
					a_loc.file_name(),
					static_cast<int>(a_loc.line()),
					a_loc.function_name() },
				spdlog::level::critical,
				a_msg);
			WinAPI::MessageBox(nullptr, body.c_str(), (caption.empty() ? nullptr : caption.c_str()), 0);
			WinAPI::TerminateProcess(WinAPI::GetCurrentProcess(), EXIT_FAILURE);
		}

		template <
			class Enum,
			std::enable_if_t<
				std::is_enum_v<
					Enum>,
				int> = 0>
		[[nodiscard]] constexpr auto to_underlying(Enum a_val) noexcept
		{
			using underlying_type_t = std::underlying_type_t<Enum>;
			return static_cast<underlying_type_t>(a_val);
		}

		template <class To, class From>
		[[nodiscard]] To unrestricted_cast(From a_from)
		{
			if constexpr (std::is_same_v<
							  std::remove_cv_t<From>,
							  std::remove_cv_t<To>>) {
				return To{ a_from };

				// From != To
			} else if constexpr (std::is_reference_v<From>) {
				return unrestricted_cast<To>(std::addressof(a_from));

				// From: NOT reference
			} else if constexpr (std::is_reference_v<To>) {
				return *unrestricted_cast<
					std::add_pointer_t<
						std::remove_reference_t<To>>>(a_from);

				// To: NOT reference
			} else if constexpr (std::is_pointer_v<From> &&
								 std::is_pointer_v<To>) {
				return static_cast<To>(
					const_cast<void*>(
						static_cast<const volatile void*>(a_from)));
			} else if constexpr ((std::is_pointer_v<From> && std::is_integral_v<To>) ||
								 (std::is_integral_v<From> && std::is_pointer_v<To>)) {
				return reinterpret_cast<To>(a_from);
			} else {
				union
				{
					std::remove_cv_t<std::remove_reference_t<From>> from;
					std::remove_cv_t<std::remove_reference_t<To>> to;
				};

				from = std::forward<From>(a_from);
				return to;
			}
		}
	}
}

#undef F4SE_MAKE_INCREMENTER_OP
#undef F4SE_MAKE_ENUMERATION_OP
#undef F4SE_MAKE_ARITHMETIC_OP
#undef F4SE_MAKE_LOGICAL_OP

namespace RE
{
	using namespace std::literals;
	namespace stl = F4SE::stl;
	namespace WinAPI = F4SE::WinAPI;
}

namespace REL
{
	using namespace std::literals;
	namespace stl = F4SE::stl;
	namespace WinAPI = F4SE::WinAPI;
}

#include "REL/Relocation.h"

#include "RE/NiRTTI_IDs.h"
#include "RE/RTTI_IDs.h"
#include "RE/VTABLE_IDs.h"

#include "RE/msvc/functional.h"
#include "RE/msvc/memory.h"
#include "RE/msvc/typeinfo.h"
