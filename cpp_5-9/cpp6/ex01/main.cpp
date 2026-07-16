//
// Created by caida-si on 15/07/2026.
//

#include "Serializer.hpp"
#include "Data.hpp"
#include <iostream>

int main() {
	Data originalData;
	originalData.id = 42;
	originalData.name = "Test Data";
	originalData.value = 3.14159;

	std::cout << "Original Data:" << std::endl;
	std::cout << "  Address: " << &originalData << std::endl;
	std::cout << "  ID: " << originalData.id << std::endl;
	std::cout << "  Name: " << originalData.name << std::endl;
	std::cout << "  Value: " << originalData.value << std::endl;
	std::cout << std::endl;

	uintptr_t serialized = Serializer::serialize(&originalData);
	std::cout << "Serialized: " << serialized << std::endl;
	std::cout << std::endl;

	Data* deserialized = Serializer::deserialize(serialized);
	std::cout << "Deserialized Data:" << std::endl;
	std::cout << "  Address: " << deserialized << std::endl;
	std::cout << "  ID: " << deserialized->id << std::endl;
	std::cout << "  Name: " << deserialized->name << std::endl;
	std::cout << "  Value: " << deserialized->value << std::endl;
	std::cout << std::endl;

	if (deserialized == &originalData)
		std::cout << "✓ Pointers match! Serialization successful." << std::endl;
	else
		std::cout << "✗ Pointers don't match! Serialization failed." << std::endl;

	return 0;
}