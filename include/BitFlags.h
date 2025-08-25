// Copyright 2015-2025 The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT
//
#pragma once
#include <type_traits>
#include <utility>

namespace utl {
	// Optionally use specialization to provide additional info about the enum type
#ifdef UTL_BITFLAGS_ENABLE_TRAITS
	// Specialization for bitmask enums
	template <typename FlagBitsType>
		requires(std::is_enum_v<FlagBitsType>)
	struct FlagTraits
	{
		static constexpr bool isBitmask = false;
	};

	// Check if enum has a static member `allFlags` that contains all flags
	template <typename T, typename = void>
	struct HasAllFlags : std::false_type {};
	template <typename T>
	struct HasAllFlags<T, std::void_t<decltype(T::allFlags)>> : std::true_type {};

	// Check if enum is a bitmask
	template <typename E>
	concept EnumIsBitFlagable = std::is_enum_v<E> && FlagTraits<E>::isBitmask;

	inline static constexpr bool traitsEnabled = false;
#else
	template <typename E>
	concept EnumIsBitFlagable = std::is_enum_v<E>;
	inline static constexpr bool traitsEnabled = false;
#endif

	template <typename EnumType >
		requires(std::is_enum_v<EnumType>)
	class BitFlags {
	public:
		using MaskType = std::underlying_type_t<EnumType>;


		// Constructors
		constexpr BitFlags() noexcept : m_mask(0) {}
		constexpr BitFlags(EnumType bit) noexcept : m_mask(static_cast<MaskType>(bit)) {}
		constexpr BitFlags(BitFlags const& rhs) noexcept = default;
		constexpr explicit BitFlags(MaskType Bitmask) noexcept : m_mask(Bitmask) {}

		// Relational operators
		constexpr auto operator <=>(BitFlags const&) const = default;

		// Logical operator
		constexpr bool operator!() const noexcept { return !m_mask; }

		// Bitwise operators
		constexpr BitFlags operator&(BitFlags const& rhs) const noexcept { return  BitFlags(m_mask & rhs.m_mask); }
		constexpr BitFlags operator|(BitFlags const& rhs) const noexcept { return  BitFlags(m_mask | rhs.m_mask); }
		constexpr BitFlags operator^(BitFlags const& rhs) const noexcept { return  BitFlags(m_mask ^ rhs.m_mask); }
		constexpr BitFlags operator~() const noexcept { return BitFlags(m_mask ^ static_cast<MaskType>(-1)); }

		// Assignment operators
		constexpr BitFlags& operator=(BitFlags const& rhs) noexcept = default;
		constexpr BitFlags& operator|=(BitFlags const& rhs) noexcept { m_mask |= rhs.m_mask; return *this; }
		constexpr BitFlags& operator&=(BitFlags const& rhs) noexcept { m_mask &= rhs.m_mask; return *this; }
		constexpr BitFlags& operator^=(BitFlags const& rhs) noexcept { m_mask ^= rhs.m_mask; return *this; }

		// Cast operators
		explicit constexpr operator bool() const noexcept { return !!m_mask; }
		explicit constexpr operator MaskType() const noexcept { return m_mask; }

		// Getters
		constexpr MaskType getMask() const noexcept { return m_mask; }

		// Helpers
		constexpr bool has(EnumType bit) const noexcept { return (m_mask & static_cast<MaskType>(bit)) != 0; }
		constexpr bool contains(EnumType bit) const noexcept { return (m_mask & static_cast<MaskType>(bit)) != 0; }
		constexpr bool contains(BitFlags const& Bitmask) const noexcept { return (m_mask & Bitmask.m_mask) == Bitmask.m_mask; }
		constexpr bool none() const noexcept { return m_mask == 0; }
		constexpr void set(EnumType bit) noexcept { m_mask |= static_cast<MaskType>(bit); }
		constexpr void clear(EnumType bit) noexcept { m_mask &= ~static_cast<MaskType>(bit); }
		constexpr void reset() noexcept { m_mask = 0; }
		constexpr void all() const noexcept { m_mask = ~MaskType{ 0 }; }


	private:
		MaskType m_mask;
	};

	namespace BitFlagOperators {
		// bitwise operators
		template <typename BitType>
		constexpr BitFlags<BitType> operator&(BitType bit, BitFlags<BitType> const& bitflag) noexcept
		{
			return bitflag.operator&(bit);
		}

		template <typename BitType>
		constexpr BitFlags<BitType> operator|(BitType bit, BitFlags<BitType> const& bitflag) noexcept
		{
			return bitflag.operator|(bit);
		}

		template <typename BitType>
		constexpr BitFlags<BitType> operator^(BitType bit, BitFlags<BitType> const& bitflag) noexcept
		{
			return bitflag.operator^(bit);
		}

		// bitwise operators on BitType
		template <EnumIsBitFlagable BitType>
		inline constexpr BitFlags<BitType> operator&(BitType lhs, BitType rhs) noexcept
		{
			return BitFlags<BitType>(lhs) & rhs;
		}

		template <EnumIsBitFlagable BitType>
		inline constexpr BitFlags<BitType> operator|(BitType lhs, BitType rhs) noexcept
		{
			return BitFlags<BitType>(lhs) | rhs;
		}

		template <EnumIsBitFlagable BitType>
		inline constexpr BitFlags<BitType> operator^(BitType lhs, BitType rhs) noexcept
		{
			return BitFlags<BitType>(lhs) ^ rhs;
		}

		template <EnumIsBitFlagable BitType>
		inline constexpr BitFlags<BitType> operator~(BitType bit) noexcept
		{
			return ~(BitFlags<BitType>(bit));
		}
	}
}




