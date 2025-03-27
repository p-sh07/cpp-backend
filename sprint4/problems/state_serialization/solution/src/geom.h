#pragma once

#include <compare>
#include <iostream>

namespace geom {

//======= Int coords =========
using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Offset {
    Dimension dx, dy;
};

struct Rectangle {
    Point position;
    Size size;
};

//======= Double coords =======
using Dimension2D = double;
using Coord2D = Dimension2D;

struct Vec2D {
    Vec2D() = default;
    Vec2D(double x, double y)
        : x(x)
        , y(y) {
    }

    Vec2D& operator*=(double scale) {
        x *= scale;
        y *= scale;
        return *this;
    }

    auto operator<=>(const Vec2D&) const = default;

    Dimension2D x = 0;
    Dimension2D y = 0;
};

struct Point2D {
    Point2D() = default;
    Point2D(double x, double y)
        : x(x)
        , y(y) {
    }
    Point2D(Point pt)
        : x(1.0 * pt.x)
        , y(1.0 * pt.y) {
    }

    auto operator<=>(const Point2D&) const = default;

    Coord2D x, y;

    Point2D& operator+=(const Vec2D& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
};

inline Vec2D operator*(Vec2D lhs, double rhs) {
    return lhs *= rhs;
}

inline Vec2D operator*(double lhs, Vec2D rhs) {
    return rhs *= lhs;
}

inline Point2D operator+(Point2D lhs, const Vec2D& rhs) {
    return lhs += rhs;
}

inline Point2D operator+(const Vec2D& lhs, Point2D rhs) {
    return rhs += lhs;
}

using Speed = Vec2D;

//== Op overloads
inline std::ostream& operator<<(std::ostream& os, const Point& pt)  {
    os << '(' << pt.x << ", " << pt.y << ')';
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const Point2D& pt)  {
    os << '{' << pt.x << ", " << pt.y << '}';
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const Speed& v) {
    os << '[' << v.x << ", " << v.y << ']';
    return os;
}

}  // namespace geom
