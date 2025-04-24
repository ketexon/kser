#include <iostream>
#include <kser/serialize.hpp>
#include <ktest/KTest.hpp>

struct Nested {
	kser::NamedField<int, "a"> a;
};

struct Data {
	kser::NamedField<int, "int_val"> int_val;
	kser::NamedField<Nested, "nested"> nested_val;
};

TEST_CASE("Serialize json", test_serialize_json){
	Data d {
		10,
		Nested {
			20,
		},
	};

	test.AssertEq(kser::serialize_json("hello"), "\"hello\"", "Strings get quoted");
	test.AssertEq(kser::serialize_json(10), "10", "Integers");
	test.AssertEq(kser::serialize_json(10.5f), "10.50", "float");
	test.AssertEq(kser::serialize_json(10.5), "10.50", "double");

	test.AssertEq(kser::serialize_json(Nested { 10 }), "{\"a\": 10}", "Struct");
	test.AssertEq(kser::serialize_json(d), "{\"int_val\": 10, \"nested\": {\"a\": 20}}", "Nested");
}