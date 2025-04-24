#include <ktest/KTest.hpp>
#include <kser/kser.hpp>
#include <iomanip>
#include <any>
#include <variant>
#include <map>

using namespace std::string_literals;

struct S {
	kser::NamedField<int, "a"> a{1};
	kser::NamedField<std::string, "b"> b{"hello"};
};

TEST_CASE("Same size as underlying", test_same_size) {
	kser::NamedField<int, "a"> a;
	test.AssertEq(sizeof(int), sizeof(a), "int has same size as NamedField<int>");
}

TEST_CASE("Field concepts", test_field_concepts) {
	bool v;

	// note: we do it like this so that these
	// become runtime tests and not compile-time tests
	v = false;
	if constexpr (kser::IsField<kser::Field<int>>){
		v = true;
	}

	test.Assert(v, "kser::Field<int> is a field");

	v = true;
	if constexpr(kser::IsField<int>){
		v = false;
	}

	test.Assert(v, "int is not a field");
}

TEST_CASE("Serialized field name and constructor", test_serialized_field_name) {
	kser::NamedField<int, "a"> a{1};
	kser::NamedField<float, "b"> b{1.0f};

	test.AssertEq(a.field_name(), "a"s, "Field name should be \"a\"");
	test.AssertEq(b.field_name(), "b"s, "Field name should be \"b\"");

	test.AssertEq(a.value, 1, "Field value should be 1");
	test.AssertApprox(b.value, 1.0f, "Field value should be 1.0f");
}

TEST_CASE("Get fields", test_get_field){
	S s;
	try {
		auto a = kser::get_field_with_name<int>(s, "a");
		test.AssertEq(a.value, 1, "Field returned has right value");
	} catch (const kser::FieldNotFound& e) {
		test.Assert(false, "Exception thrown for existing field");
	}

	bool throws = false;
	try {
		auto c = kser::get_field_with_name<std::string>(s, "c");
	} catch (const kser::FieldNotFound& e) {
		throws = true;
	}
	test.Assert(throws, "Exception thrown for nonexistent field");
}

TEST_CASE("Try get fields", test_try_get_field) {
	struct S {
		kser::NamedField<int, "a"> a{1};
		kser::NamedField<std::string, "b"> b{"hello"};
	};

	S s {
		1,
		"hi"
	};
	auto a = kser::try_get_field_with_name<int>(s, "a");
	test.Assert(a.has_value(), "Returns optional");
	test.AssertEq(a->get().value, 1, "Returns right value");

	auto c = kser::try_get_field_with_name<std::string>(s, "c");
	test.Assert(!c.has_value(), "Returns empty optional for nonexistent field");

	a->get().value = 3;
	test.AssertEq(s.a.value, 3, "Field value changed");
}

TEST_CASE("Get field value", test_get_field_value) {
	S s;
	auto a = kser::get_value<std::optional<int>>(s, "a");
	test.Assert(a.has_value(), "Returns optional");
	test.AssertEq(*a, 1, "Returns right value");

	auto b_any = kser::get_value<std::any>(s, "b");
	test.Assert(b_any.has_value(), "Returns any");
	auto b_value = std::any_cast<std::string>(b_any);
	test.AssertEq(b_value, "hello", "Correct value in any");

	auto b_variant = kser::get_value<std::variant<std::monostate, int, std::string>>(s, "b");
	test.AssertEq(b_variant.index(), 2, "Variant holds string");
	test.AssertEq(std::get<std::string>(b_variant), "hello", "Correct value in variant");
}

TEST_CASE("Get value strict mode", test_get_value_strict) {
	S s;

	try {
		auto a = kser::get_value<int, true>(s, "a");
		test.AssertEq(a, 1, "Strict returns right value");
	} catch (const kser::TypeMismatch& e) {
		test.Assert(false, "Type mismatch on correct type");
	}

	try {
		auto a = kser::get_value<std::string, true>(s, "a");
		test.Assert(false, "Type mismatch on wrong type");
	} catch (const kser::TypeMismatch& e) {}

	try {
		auto a = kser::get_value<float, true>(s, "a");
		test.Assert(false, "Type mismatch on compatible type");
	} catch (const kser::TypeMismatch& e) {}
}

TEST_CASE("Field and value maps", test_field_and_value_maps) {
	S s {
		10,
		"hello",
	};
	using variant = std::variant<std::monostate, int, std::string>;
	auto map = kser::get_value_map<std::map<std::string_view, variant>>(s);

	test.AssertEq(map.size(), 2, "Map has right size");
	test.AssertEq(map["a"].index(), 1, "Map has int");
	test.AssertEq(std::get<int>(map["a"]), 10, "Map has right int value");

	test.AssertEq(map["b"].index(), 2, "Map has string");
	test.AssertEq(std::get<std::string>(map["b"]), "hello", "Map has right string value");
}

TEST_CASE("Setting values", test_set_values) {
	S s {
		3,
		"meow",
	};

	bool did_set = kser::set_value(s, "a", 10);
	test.Assert(did_set, "Set value");
	test.AssertEq(s.a.value, 10, "Field a set");

	using variant = std::variant<std::monostate, int, std::string>;

	auto map = std::map<std::string_view, variant>{
		{"a", 10},
		{"b", "hello"},
	};

	int num_set;
	try {
		num_set = kser::set_values(s, map);
	} catch (const std::bad_variant_access& e) {
		test.Assert(false, "Exception thrown for variant cast");
	} catch (const std::bad_any_cast& e) {
		test.Assert(false, "Wrong default cast (should be variant, got any)");
	}

	test.AssertEq(s.a.value, 10, "Field a set");
	test.AssertEq(s.b.value, "hello", "Field b set");

	auto any_map = std::map<std::string_view, std::any>{
		{"a", 20},
		{"b", "hi"s},
	};

	try {
		num_set = kser::set_values(s, any_map);
		test.AssertEq(num_set, 2, "Set all values for any");
	} catch (const std::bad_any_cast& e) {
		test.Assert(false, "Exception thrown for any cast");
	} catch (const std::bad_variant_access& e) {
		test.Assert(false, "Wrong default cast (should be any, got variant)");
	}

	test.AssertEq(s.a.value, 20, "Field a set");
	test.AssertEq(s.b.value, "hi", "Field b set");

	auto empty_map = std::map<std::string_view, std::any>{};
	num_set = kser::set_values(s, empty_map);
	test.AssertEq(num_set, 0, "Set no values");

	struct NumberOnly {
		kser::NamedField<int, "x"> x;
		kser::NamedField<int, "y"> y;
	};

	NumberOnly n {
		10,
		20,
	};

	auto number_map = std::map<std::string_view, int>{
		{"x", 30},
		{"y", 40},
	};

	num_set = kser::set_values(n, number_map);
	test.AssertEq(num_set, 2, "Set all values for number map");
	test.AssertEq(n.x.value, 30, "Field x set");
	test.AssertEq(n.y.value, 40, "Field y set");
}

TEST_CASE("Visitor", test_visitor) {
	S s {
		1,
		"hello",
	};

	bool visited_a = false;
	bool visited_b = false;

	auto field_visitor = [&](auto& field) {
		using value_t = std::decay_t<decltype(field.value)>;
		if constexpr (std::same_as<value_t, int>) {
			visited_a = true;
			test.AssertEq(field.value, 1, "Visitor visits field a");
		}
		else if constexpr (std::same_as<value_t, std::string>) {
			visited_b = true;
			test.AssertEq(field.value, "hello", "Visitor visits field b");
		}
	};

	kser::visit_fields(s, field_visitor);
	test.Assert(visited_a, "Visitor visited field a");
	test.Assert(visited_b, "Visitor visited field b");

	visited_a = false;
	visited_b = false;

	auto field_visitor_shorting = [&](auto& field) {
		using value_t = std::decay_t<decltype(field.value)>;
		if constexpr (std::same_as<value_t, int>) {
			visited_a = true;
			return true;
		}
		else if constexpr (std::same_as<value_t, std::string>) {
			visited_b = true;
		}
		return false;
	};

	kser::visit_fields(s, field_visitor_shorting);

	test.Assert(visited_a, "Visitor visited field a (shorting)");
	test.Assert(!visited_b, "Visitor did not visit field b (shorting)");

	visited_a = false;
	visited_b = false;

	auto value_visitor = [&](auto& value) {
		using value_t = std::decay_t<decltype(value)>;
		if constexpr (std::same_as<value_t, int>) {
			visited_a = true;
			test.AssertEq(value, 1, "Visitor visits field a");
		}
		else if constexpr (std::same_as<value_t, std::string>) {
			visited_b = true;
			test.AssertEq(value, "hello", "Visitor visits field b");
		}
	};

	kser::visit_values(s, value_visitor);
	test.Assert(visited_a, "Visitor visited field a");
	test.Assert(visited_b, "Visitor visited field b");


	visited_a = false;
	visited_b = false;

	auto value_visitor_shorting = [&](auto& value) {
		using value_t = std::decay_t<decltype(value)>;
		if constexpr (std::same_as<value_t, int>) {
			visited_a = true;
			return true;
		}
		else if constexpr (std::same_as<value_t, std::string>) {
			visited_b = true;
		}
		return false;
	};

	kser::visit_values(s, value_visitor_shorting);
	test.Assert(visited_a, "Visitor visited field a (shorting)");
	test.Assert(!visited_b, "Visitor did not visit field b (shorting)");
}