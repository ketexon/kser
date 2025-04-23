#include <ktest/KTest.hpp>
#include <kser/kser.hpp>
#include <iomanip>
#include <any>
#include <variant>

using namespace std::string_literals;

struct S {
	kser::SerializedField<int, "a"> a{1};
	kser::SerializedField<std::string, "b"> b{"hello"};
};

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
	kser::SerializedField<int, "a"> a{1};
	kser::SerializedField<float, "b"> b{1.0f};

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
		kser::SerializedField<int, "a"> a{1};
		kser::SerializedField<std::string, "b"> b{"hello"};
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