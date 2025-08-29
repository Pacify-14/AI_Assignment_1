#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <vector>
#include <cmath> // For sqrt and pow
using namespace std;

// --- GEOMETRIC & ENTITY STRUCTURES ---

struct Point {
    double x, y;
};

// --- UTILITY FUNCTIONS ---

/**
 * @brief Calculates the Euclidean distance between two points.
 */
inline double distance(const Point& p1, const Point& p2) {
    return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}


// --- PROBLEM & SOLUTION STRUCTURES (remaining definitions are the same) ---

struct PackageInfo {
    double weight, value;
};

struct Village {
    int id;
    Point coords;
    int population;
};

struct Helicopter {
    int id;
    int home_city_id;
    double weight_capacity;
    double distance_capacity;
    double fixed_cost; // F
    double alpha;
};

struct ProblemData {
    double time_limit_minutes;
    double d_max;
    vector<PackageInfo> packages;
    vector<Point> cities;
    vector<Village> villages;
    vector<Helicopter> helicopters;
};

struct Drop {
    int village_id;
    int dry_food;
    int perishable_food;
    int other_supplies;
};

struct Trip {
    int dry_food_pickup;
    int perishable_food_pickup;
    int other_supplies_pickup;
    vector<Drop> drops;
};

struct HelicopterPlan {
    int helicopter_id;
    vector<Trip> trips;
};

using Solution = vector<HelicopterPlan>;

#endif // STRUCTURES_H