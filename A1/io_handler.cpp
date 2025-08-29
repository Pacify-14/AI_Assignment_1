#include "io_handler.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
using namespace std;

ProblemData readInputData(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Error: Could not open input file " + filename);
    }

    ProblemData data;
    string line;

    // Line 1: Processing time
    getline(file, line);
    stringstream(line) >> data.time_limit_minutes;

    // Line 2: DMax
    getline(file, line);
    stringstream(line) >> data.d_max;

    // Line 3: Package weights and values
    getline(file, line);
    stringstream pkg_ss(line);
    data.packages.resize(3);
    pkg_ss >> data.packages[0].weight >> data.packages[0].value;
    pkg_ss >> data.packages[1].weight >> data.packages[1].value;
    pkg_ss >> data.packages[2].weight >> data.packages[2].value;

    // Line 4: Cities
    getline(file, line);
    stringstream city_ss(line);
    int num_cities;
    city_ss >> num_cities;
    data.cities.resize(num_cities);
    for (int i = 0; i < num_cities; ++i) {
        city_ss >> data.cities[i].x >> data.cities[i].y;
    }

    // Line 5: Villages
    getline(file, line);
    stringstream village_ss(line);
    int num_villages;
    village_ss >> num_villages;
    data.villages.resize(num_villages);
    for (int i = 0; i < num_villages; ++i) {
        data.villages[i].id = i + 1;
        village_ss >> data.villages[i].coords.x >> data.villages[i].coords.y >> data.villages[i].population;
    }

    // Line 6: Helicopters
    getline(file, line);
    stringstream heli_ss(line);
    int num_helicopters;
    heli_ss >> num_helicopters;
    data.helicopters.resize(num_helicopters);
    for (int i = 0; i < num_helicopters; ++i) {
        data.helicopters[i].id = i + 1;
        heli_ss >> data.helicopters[i].home_city_id >> data.helicopters[i].weight_capacity >> data.helicopters[i].distance_capacity >> data.helicopters[i].fixed_cost >> data.helicopters[i].alpha;
    }

    return data;
}


void writeOutputData(const string& filename, const Solution& solution) {
    ofstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Error: Could not open output file " + filename);
    }

    for (const auto& plan : solution) {
        file << plan.helicopter_id << " " << plan.trips.size() << "\n";
        for (const auto& trip : plan.trips) {
            file << trip.dry_food_pickup << " " << trip.perishable_food_pickup << " " << trip.other_supplies_pickup << " " << trip.drops.size();
            for (const auto& drop : trip.drops) {
                file << " " << drop.village_id << " " << drop.dry_food << " " << drop.perishable_food << " " << drop.other_supplies;
            }
            file << "\n";
        }
        file << -1 << "\n";
    }
}