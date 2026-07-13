#ifndef VECT2_HPP
#define VECT2_HPP

#include <iostream>

class vect2 {
private:
	int x, y;
public:
	vect2() : x(0), y(0) {}
	vect2(int num1, int num2) : x(num1), y(num2) {}

	int operator[](int i) const { return i == 0 ? x : y; }
	int& operator[](int i) { return i == 0 ? x : y; }
	vect2 operator-() { return vect2(-x, -y); }
	vect2 operator*(int num) const { vect2 tmp = *this; tmp *= num; return tmp; }

	vect2& operator*=(int num) { x *= num; y *= num; return *this; }
	vect2& operator+=(const vect2& obj) { x += obj.x; y += obj.y; return *this; }
	vect2& operator-=(const vect2& obj) { x -= obj.x; y -= obj.y; return *this; }
	vect2& operator*=(const vect2& obj) { x *= obj.x; y *= obj.y; return *this; }

	vect2 operator+(const vect2& obj) const { vect2 tmp = *this; tmp += obj; return tmp; }
	vect2 operator-(const vect2& obj) const { vect2 tmp = *this; tmp -= obj; return tmp; }
	vect2 operator*(const vect2& obj) const { vect2 tmp = *this; tmp *= obj; return tmp; }

	vect2& operator++() { return *this += vect2(1, 1); }
	vect2& operator--() { return *this -= vect2(1, 1); }
	vect2 operator++(int) { vect2 tmp = *this; ++*this; return tmp; }
	vect2 operator--(int) { vect2 tmp = *this; --*this; return tmp; }

	bool operator==(const vect2& obj) const { return (x == obj.x && y == obj.y); }
	bool operator!=(const vect2& obj) const { return !(*this == obj); }
};

inline vect2 operator*(int num, const vect2& obj) { return obj * num; }
inline std::ostream& operator<<(std::ostream& os, const vect2& obj) { return os << '{' << obj[0] << ", " << obj[1] << "}"; }

#endif