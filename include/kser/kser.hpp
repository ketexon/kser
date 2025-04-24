#pragma once

#include <algorithm> 	// copy
#include <string_view>
#include <concepts>
#include <variant>
#include <any>
#include <optional>
#include <stdexcept> 	// runtime_error
#include <functional> 	// reference wrapper

namespace kser {
	struct FieldNotFound : std::runtime_error {
		FieldNotFound(std::string_view name)
			: std::runtime_error("Field not found: " + std::string(name)) {}
	};

	struct TypeMismatch : std::runtime_error {
		TypeMismatch(std::string_view name)
			: std::runtime_error("Field type mismatch: " + std::string(name)) {}
	};

	template<size_t N>
	struct StaticString {
		char value[N];

		constexpr StaticString(const char (&str)[N]) {
			std::copy(str, str + N, value);
		}

		constexpr std::string_view string_view() const {
			return std::string_view{value, value + N - 1};
		}
	};

	template<typename T>
	struct Field {
		using type = T;
		T value;
	};

	template<typename T>
	concept IsField = std::derived_from<T, Field<typename T::type>>;

	template<typename T, StaticString Name>
	struct NamedField : Field<T> {
		std::string_view field_name() const {
			return Name.string_view();
		}
	};

	template<typename T>
	constexpr Field<T>*
	try_get_ptr_field_with_name(auto& s, std::string_view name) {
		auto& [...x] = s;
		Field<T>* out = nullptr;
		(... || ([&] {
			if constexpr (std::derived_from<decltype(x), Field<T>>) {
				if (name == x.field_name()) {
					out = &x;
					return true;
				}
			}
			return false;
		})());
		return out;
	}

	template<typename T>
	constexpr std::optional<std::reference_wrapper<Field<T>>>
	try_get_field_with_name(auto& s, std::string_view name) {
		auto ptr = try_get_ptr_field_with_name<T>(s, name);
		if (ptr) {
			return std::ref(*ptr);
		}
		return std::nullopt;
	}

	template<typename T>
	constexpr Field<T>& get_field_with_name(auto& s, std::string_view name) {
		auto field_opt = try_get_field_with_name<T>(s, name);
		if (!field_opt) {
			throw FieldNotFound(name);
		}
		return field_opt->get();
	}

	template<typename T>
	constexpr std::optional<T> try_get_value(auto& s, std::string_view name) {
		auto& [...x] = s;
		std::optional<T> out;
		(... || ([&] {
			if constexpr (
				IsField<std::decay_t<decltype(x)>>
			) {
				if constexpr (std::assignable_from<decltype((out)), decltype(x.value)>) {
					if (name == x.field_name()) {
						out = x.value;
						return true;
					}
				}
			}
			return false;
		})());
		return out;
	}

	template<typename T, bool Strict = false>
		requires std::default_initializable<T>
	constexpr T get_value(auto& s, std::string_view name) {
		auto& [...x] = s;
		T out;
		auto set = (... || ([&] -> bool {
			if constexpr (
				IsField<std::decay_t<decltype(x)>>
			) {
				if constexpr (Strict && std::same_as<T, std::decay_t<decltype(x.value)>>) {
					if (name == x.field_name()) {
						out = x.value;
						return true;
					}
				}
				else if constexpr (Strict) {
					if (name == x.field_name()) {
						throw TypeMismatch(name);
					}
				}
				else if constexpr (std::assignable_from<decltype((out)), decltype(x.value)>) {
					if (name == x.field_name()) {
						out = x.value;
						return true;
					}
				}
				else {
					if (name == x.field_name()) {
						throw TypeMismatch(name);
					}
				}
			}
			return false;
		})());
		if (!set) {
			throw FieldNotFound(name);
		}
		return out;
	}

	template<typename T>
		requires std::default_initializable<T>
	constexpr T get_value_strict(auto& s, std::string_view name) {
		return get_value<T, true>(s, name);
	}

	template<typename TMap>
	constexpr void get_field_map(auto& s, TMap& out) {
		auto& [...x] = s;
		(([&] {
			if constexpr (
				IsField<std::decay_t<decltype(x)>>
			) {
				out[x.field_name()] = x;
			}
		})(), ...);
	}

	template<typename TMap>
		requires std::default_initializable<TMap>
	constexpr TMap get_field_map(auto& s) {
		TMap out;
		get_field_map(s, out);
		return out;
	}

	template<typename TMap>
	constexpr void get_value_map(auto& s, TMap& out) {
		auto& [...x] = s;
		(([&] {
			if constexpr (
				IsField<std::decay_t<decltype(x)>>
			) {
				out[x.field_name()] = x.value;
			}
		})(), ...);
	}

	template<typename TMap>
		requires std::default_initializable<TMap>
	constexpr TMap get_value_map(auto& s) {
		TMap out;
		get_value_map(s, out);
		return out;
	}

	template<typename T>
	struct StaticCastCaster {
		auto operator ()(auto& x) {
			return static_cast<T>(x);
		}
	};

	template<typename T>
	struct GetCaster {
		auto operator ()(auto& x) {
			return std::get<T>(x);
		}
	};

	template<typename T>
	struct AnyCastCaster {
		auto operator ()(auto& x) {
			return std::any_cast<std::decay_t<T>>(x);
		}
	};

	template<typename T>
	struct DefaultCaster{};

	template<typename TOut, typename TVal, template<typename> typename TCast>
	using Caster = std::conditional_t<
		std::same_as<TCast<TOut>, DefaultCaster<TOut>>,
		std::conditional_t<
			std::same_as<TVal, std::any>,
			AnyCastCaster<TOut>,
			std::conditional_t<
				requires(TVal x) { { std::get<TOut>(x) } -> std::convertible_to<TOut>; },
				GetCaster<TOut>,
				StaticCastCaster<TOut>
			>
		>,
		TCast<TOut>
	>;

	template<template<typename> typename TCast = DefaultCaster>
	constexpr int set_values(auto& s, const auto& in) {
		auto& [...x] = s;
		return (... + ([&] {
			if constexpr (
				IsField<std::decay_t<decltype(x)>>
			) {
				auto it = in.find(x.field_name());
				if (it != in.end()) {
					Caster<decltype(x.value), decltype(it->second), TCast> caster;
					x.value = caster(it->second);
					return 1;
				}
			}
			return 0;
		})());
	}

	constexpr bool set_value(auto& s, std::string_view name, auto value) {
		auto& [...x] = s;
		return (... || ([&] {
			if constexpr (
				IsField<std::decay_t<decltype(x)>>
			) {
				if (name == x.field_name()) {
					x.value = value;
					return true;
				}
			}
			return false;
		})());
	}

	constexpr void visit_fields(auto& s, auto& visitor) {
		auto& [...x] = s;
		(... || ([&] -> bool {
			if constexpr (
				IsField<std::decay_t<decltype(x)>>
			) {
				if constexpr(std::same_as<decltype(visitor(x)), bool>) {
					return visitor(x);
				}
				else {
					visitor(x);
				}
			}
			return false;
		})());
	}

	constexpr void visit_values(auto& s, auto& visitor) {
		auto& [...x] = s;
		(... || ([&] {
			if constexpr (
				IsField<std::decay_t<decltype(x)>>
			) {
				if constexpr(std::same_as<decltype(visitor(x)), bool>) {
					return visitor(x.value);
				}
				else {
					visitor(x.value);
				}
				return false;
			}
		})());
	}
}