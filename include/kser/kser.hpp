#pragma once

#include <algorithm>
#include <string_view>
#include <concepts>
#include <optional>

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
	struct SerializedField : Field<T> {
		std::string_view field_name() const {
			return Name.string_view();
		}
	};

	template<typename T, typename S>
	constexpr Field<T>*
	try_get_ptr_field_with_name(S& s, std::string_view name) {
		auto& [...x] = s;
		Field<T>* out = nullptr;
		(([&] {
			if(out) return;
			if constexpr (std::derived_from<decltype(x), Field<T>>) {
				if (name == x.field_name()) {
					out = &x;
				}
			}
		})(), ...);
		return out;
	}

	template<typename T, typename S>
	constexpr std::optional<std::reference_wrapper<Field<T>>>
	try_get_field_with_name(S& s, std::string_view name) {
		auto ptr = try_get_ptr_field_with_name<T>(s, name);
		if (ptr) {
			return std::ref(*ptr);
		}
		return std::nullopt;
	}

	template<typename T, typename S>
	constexpr Field<T>& get_field_with_name(S& s, std::string_view name) {
		auto field_opt = try_get_field_with_name<T>(s, name);
		if (!field_opt) {
			throw FieldNotFound(name);
		}
		return field_opt->get();
	}

	template<typename T, typename S>
	constexpr std::optional<T> try_get_value(S& s, std::string_view name) {
		auto& [...x] = s;
		std::optional<T> out;
		(([&] {
			if(out) return;
			if constexpr (
				IsField<std::decay_t<decltype(x)>>
			) {
				if constexpr (std::assignable_from<decltype((out)), decltype(x.value)>) {
					if (name == x.field_name()) {
						out = x.value;
					}
				}
			}
		})(), ...);
		return out;
	}

	template<typename T, bool Strict = false, typename S>
		requires std::default_initializable<T>
	constexpr T get_value(S& s, std::string_view name) {
		auto& [...x] = s;
		bool set = false;
		T out;
		(([&] {
			if(set) return;
			if constexpr (
				IsField<std::decay_t<decltype(x)>>
			) {
				if constexpr (Strict && std::same_as<T, std::decay_t<decltype(x.value)>>) {
					if (name == x.field_name()) {
						out = x.value;
						set = true;
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
						set = true;
					}
				}
				else {
					if (name == x.field_name()) {
						throw TypeMismatch(name);
					}
				}
			}
		})(), ...);
		return out;
	}

	template<typename T, typename S>
		requires std::default_initializable<T>
	constexpr T get_value_strict(S& s, std::string_view name) {
		return get_value<T, true>(s, name);
	}
}