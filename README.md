# KSer

Ketexon's serialization utility

## Introduction

This is a proof of concept for basic reflection using these c++ features:
- strings in templates arguments
- structured binding pack
- folding over lambdas

### Motivation

C++17's introduced [structured bindings](https://en.cppreference.com/w/cpp/language/structured_binding):

```c++
struct Struct {
	int x;
	std::string y;
	float z;
};
auto [a,b,c] = Struct { 1, "hi", 3.5f };
```

C++26's structured binding pack, which allows you to do this as a [pack](https://en.cppreference.com/w/cpp/language/pack):

```c++
struct Struct {
	int x;
	std::string y;
	float z;
};

auto f(auto& s) {
	auto [...x] = s;
}
```

The only way to iterate over all values of a pack is a [fold expression](https://en.cppreference.com/w/cpp/language/fold) currently (you can [access by index](https://en.cppreference.com/w/cpp/language/pack_indexing) `x...[0]` since c++26, but you can't do this within a for loop since for loops aren't yet constexpr).

Thus, the only option is to use a templated function. And with templated lambdas to the rescue, we can use a [fold expression](https://en.cppreference.com/w/cpp/language/fold) over the pack with an immediately invoked lambda:

```c++
struct Struct {
	int x;
	std::string y;
	float z;
};

auto f(auto& s) {
	auto [...x] = s;
	([&] {
		std::cout << x << std::endl;
	}(), ...);
}

int main(){
	Struct s { 1, "hello", 3.5f };
	f(s);
	// prinst:
	// ```
	// 1
	// hello
	// 3.5
	// ```
	return 0;
}
```

Since this is a templated lambda, we can change the behavior based on the type
to get a value of certain type and value conditions (with short circuiting
to get the first of said values):

```c++
template<typename T>
T f(auto& s) {
	auto [...x] = s;
	T out;
	(... || [&] {
		if constexpr(std::assignable_from<decltype((out)), decltype(x.value)>) {
			out = x;
			return true;
		}
		return false;
	}());
	return out;
}
```

This is all we need, since now we can just create our own custom
type specifically for serialization.

```c++
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
```

Another tool this uses is compile-time strings, which allows us to
store the name of a field in its type and have it be 0 overhead by default.

```c++
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
```

## Usage

See examples.

```c++
#include <kser/kser.hpp>
#include <iostream>
#include <map>

struct MyStruct {
	kser::NamedField<int, "age"> age;
	kser::NamedField<std::string, "name"> name;
	kser::NamedField<float, "max_health"> max_health;
	float cur_health;
};

int main(){
	MyStruct s {
		21, 		// age
		"Aubrey", 	// name
		100.0f, 	// max_health
		50.0f,		// cur_health
	};

	// Get a field value by name
	auto age = kser::get_value<int>(s, "age");
	std::cout << "Age: " << age << std::endl;

	// Geting a field works for any assignable type
	// such as std::any, std::variant, or float
	auto name = kser::get_value<std::any>(s, "name");
	try {
		std::cout << "Name: " << std::any_cast<std::string>(name) << std::endl;
	} catch (const std::bad_any_cast& e) {
		std::cerr << "Bad any cast: " << e.what() << std::endl;
	}

	// if you want to get the field of the exact type, not convertible to,
	// use the strict mode (get_value_strict or get_value<T, true>)
	try {
		auto age = kser::get_value_strict<float>(s, "age");
	} catch (const kser::TypeMismatch& e) {
		std::cerr << "Type mismatch: " << e.what() << std::endl;
	}

	// this will throw an exception if the field is not found
	try {
		auto meow = kser::get_value<float>(s, "meow");
	} catch (const kser::FieldNotFound& e) {
		std::cerr << "Field not found: " << e.what() << std::endl;
	}

	// and fields not wrapped in NamedField are not accessible
	auto cur_health = kser::try_get_value<float>(s, "cur_health");
	std::cout << "Cur health found: " << cur_health.has_value() << std::endl;

	// there are also non-throwing versions with the prefix try_
	// you can also get a reference to the field itself
	// so that you can modify it directly

	// the non-throwing version returns an std::optional
	// of a std::reference_wrapper
	auto meow_opt = kser::try_get_field_with_name<int>(s, "meow");
	std::cout << "Meow found: " << meow_opt.has_value() << std::endl;

	auto age_opt = kser::try_get_field_with_name<int>(s, "age");
	age_opt->get().value = 23;
	std::cout << "New age after try_get_field_with_name: " << age_opt->get().value << std::endl;

	// Set a field value by name
	kser::set_value(s, "age", 22);
	std::cout << "New age after set_value: " << s.age.value << std::endl;

	// you can also get all values
	// this works by default for any container that
	// you can do v[key] = value; with
	using variant_t = std::variant<std::monostate, int, std::string, float>;
	auto values = kser::get_value_map<std::map<std::string_view, variant_t>>(s);

	std::cout << "Values map size: " << values.size() << std::endl;
	std::cout << "Age: " << std::get<int>(values["age"]) << std::endl;
	std::cout << "Name: " << std::get<std::string>(values["name"]) << std::endl;
	std::cout << "Max health: " << std::get<float>(values["max_health"]) << std::endl;

	// you can also set a bunch of values
	// this works by default for std::any, std::variant,
	// and any static_castable type.
	// a custom type TVal are supported too, but you need
	// to provide a custom Caster<TOut> that implements
	// - a default constructor
	// - TOut operator()(TVal map_value);
	// see AnyCaster, GetCaster, and StaticCastCaster
	std::map<std::string_view, variant_t> new_values {
		{"age", 95},
		{"name", "Bob"},
		{"max_health", 200.0f},
	};
	kser::set_values(s, new_values);

	std::cout << "New age after set_values: " << s.age.value << std::endl;
	std::cout << "New name after set_values: " << s.name.value << std::endl;
	std::cout << "New max health after set_values: " << s.max_health.value << std::endl;
	std::cout << "Cur health after set_values: " << s.cur_health << std::endl;

	// there is also templating visiting functionality
	auto visitor = [](auto& field) {
		using value_t = std::decay_t<decltype(field.value)>;
		std::cout << "Visiting field: " << field.field_name() << ", ";
		if constexpr (std::same_as<value_t, int>) {
			std::cout << "int field: " << field.value << std::endl;
		}
		else if constexpr (std::same_as<value_t, std::string>) {
			std::cout << "string field: " << field.value << std::endl;
		}
		else if constexpr (std::same_as<value_t, float>) {
			std::cout << "float field: " << field.value << std::endl;
		}
	};

	std::cout << "Visiting fields" << std::endl;
	kser::visit_fields(s, visitor);

	// you can also stop the visitor early by returning true
	auto visitor_early = [](auto& field) {
		using value_t = std::decay_t<decltype(field.value)>;
		std::cout << "Visiting field: " << field.field_name();
		if constexpr (std::same_as<value_t, int>) {
			std::cout << ", int field: " << field.value << std::endl;
			return true; // stop visiting
		}
		return false;
	};

	std::cout << "Visiting fields with early stopping" << std::endl;
	kser::visit_fields(s, visitor_early);

	return 0;
}
```