#include <kser/kser.hpp>
#include <kser/serialize.hpp>
#include <iostream>
#include <map>

struct Player {
	kser::NamedField<int, "max_health"> max_health;
	kser::NamedField<float, "damage"> damage;
	int cur_health;
};

int main(){
	Player player {
		100,
		10.0f,
		50,
	};

	std::cout << "Has cur_health: " << kser::has_field(player, "cur_health") << std::endl;
	// Has cur_health: 0

	std::cout << "Has max_health: " << kser::has_field(player, "max_health") << std::endl;
	// Has cur_health: 1

	kser::set_value(player, "max_health", 120);
	std::cout << "Max Health: " << player.max_health.value << std::endl;
	// Max Health: 120

	using variant_t = std::variant<int, float>;
	auto fields = kser::get_value_map<std::map<std::string_view, variant_t>>(player);

	std::cout << "Damage: " << std::get<float>(fields["damage"]) << std::endl;
	// Damage: 10

	std::cout << kser::serialize_json(player) << std::endl;
	// {"max_health": 120, "damage": 10.00}

	return 0;
}
