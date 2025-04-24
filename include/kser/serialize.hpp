#pragma once

#include <kser/kser.hpp>
#include <string>
#include <sstream>
#include <iomanip>

namespace kser {
	template<size_t TPrecision = 2>
	constexpr bool serialize_json(auto&& value, std::stringstream& ss) {
		using value_t = decltype(value);
		using decayed_t = std::decay_t<value_t>;
		if constexpr (std::same_as<decayed_t, bool>) {
			ss << std::boolalpha << value;
			return true;
		}
		else if constexpr (std::integral<decayed_t> || std::floating_point<decayed_t>) {
			ss << std::fixed << std::setprecision(TPrecision) << value;
			return true;
		}
		else if constexpr (std::assignable_from<std::string&, decayed_t>) {
			ss << std::quoted(std::string(value));
			return true;
		}
		else if constexpr (requires { kser::get_value<int>(value, std::declval<std::string_view>()); }) {
			bool first = true;
			ss << "{";
			kser::visit_name_values(value, [&ss, &first](auto&& name, auto&& v) {
				std::stringstream ss2;
				if(serialize_json(v, ss2)){
					if (!first) {
						ss << ", ";
					}
					first = false;
					ss << std::quoted(name) << ": " << ss2.str();
				}
			});
			ss << "}";
			return true;
		}
		return false;
	}

	template<size_t TPrecision = 2>
	constexpr std::string serialize_json(auto&& s){
		std::stringstream ss;
		serialize_json<TPrecision>(s, ss);
		return ss.str();
	}
}