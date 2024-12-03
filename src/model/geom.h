#pragma once

#include <compare>

namespace geom {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
    auto operator<=>(const Point&) const = default;
};

struct Vec2D {
    Vec2D() = default;
    Vec2D(double x, double y)
        : x(x)
        , y(y) {
    }

    Vec2D operator+(const Vec2D& other) const {
        return Vec2D{this->x + other.x, this->y + other.y};
    }

    Vec2D& operator+=(const Vec2D& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }    

    Vec2D& operator*=(double scale) {
        x *= scale;
        y *= scale;
        return *this;
    }

    auto operator<=>(const Vec2D&) const = default;

    double x = 0;
    double y = 0;
};

inline Vec2D operator*(Vec2D lhs, double rhs) {
    return lhs *= rhs;
}

inline Vec2D operator*(double lhs, Vec2D rhs) {
    return rhs *= lhs;
}

}  // namespace geom
