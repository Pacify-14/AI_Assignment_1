#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <numeric>
#include <algorithm>

#include "structures.h"
#include "io_handler.h" 

using namespace std;

/**
 * @brief Main verification and scoring function.
 * @return The final objective score, or -1.0 if any constraints are violated.
 */
double verifyAndCalculateScore(const string& input_file_path, const string& output_file_path) {
    ProblemData data = readInputData(input_file_path);
    
    ifstream outfile(output_file_path);
    if (!outfile.is_open()) {
        throw runtime_error("Error: Could not open output file " + output_file_path);
    }

    bool constraint_violated = false;
    vector<double> food_delivered(data.villages.size() + 1, 0.0);
    vector<double> other_delivered(data.villages.size() + 1, 0.0);
    vector<double> village_values(data.villages.size() + 1, 0.0);
    vector<double> helicopter_total_distances(data.helicopters.size() + 1, 0.0);
    double total_trip_cost = 0.0;
    string line;

    // cout << fixed << setprecision(4);

    int line_num = 0;
    while (getline(outfile, line)) {
        line_num++;
        if (line.empty() || line.find_first_not_of(" \t\n\v\f\r") == string::npos) continue;

        stringstream ss(line);
        int helicopter_id;
        ss >> helicopter_id;
        if (helicopter_id == -1) continue;

        int num_trips;
        ss >> num_trips;

        if (helicopter_id <= 0 || helicopter_id > data.helicopters.size()) {
            cerr << "Error (Line " << line_num << "): Invalid helicopter ID " << helicopter_id << endl;
            return -1.0;
        }
        const auto& helicopter = data.helicopters[helicopter_id - 1];
        Point home_city_coords = data.cities[helicopter.home_city_id - 1];

        for (int i = 0; i < num_trips; ++i) {
            if (!getline(outfile, line)) {
                cerr << "Error: Unexpected end of file for helicopter " << helicopter_id << "." << endl;
                return -1.0;
            }
            line_num++;

            stringstream trip_ss(line);
            int d, p, o, num_villages_in_trip;
            trip_ss >> d >> p >> o >> num_villages_in_trip;

            double trip_weight = (d * data.packages[0].weight) + (p * data.packages[1].weight) + (o * data.packages[2].weight);
            if (trip_weight > helicopter.weight_capacity + 1e-9) {
                cout << "*** WARNING: Heli " << helicopter_id << ", Trip " << i + 1 << " exceeds weight capacity (" << trip_weight << " > " << helicopter.weight_capacity << ")." << endl;
                constraint_violated = true;
            }

            Point current_location = home_city_coords;
            double trip_distance = 0.0;
            int total_d_dropped = 0, total_p_dropped = 0, total_o_dropped = 0;

            for (int j = 0; j < num_villages_in_trip; ++j) {
                int village_id, vd, vp, vo;
                trip_ss >> village_id >> vd >> vp >> vo;
                total_d_dropped += vd; total_p_dropped += vp; total_o_dropped += vo;

                if (village_id <= 0 || village_id > data.villages.size()) {
                    cerr << "Error (Line " << line_num << "): Invalid village ID " << village_id << endl;
                    return -1.0;
                }
                const auto& village = data.villages[village_id - 1];
                
                // Value Capping Logic
                double max_food_needed = village.population * 9.0;
                double food_room_left = max(0.0, max_food_needed - food_delivered[village_id]);
                double food_in_this_drop = vd + vp;
                double effective_food_this_drop = min(food_in_this_drop, food_room_left);
                double effective_vp = min((double)vp, effective_food_this_drop);
                double value_from_p = effective_vp * data.packages[1].value;
                double remaining_effective_food = effective_food_this_drop - effective_vp;
                double effective_vd = min((double)vd, remaining_effective_food);
                double value_from_d = effective_vd * data.packages[0].value;
                village_values[village_id] += value_from_p + value_from_d;

                double max_other_needed = village.population * 1.0;
                double other_room_left = max(0.0, max_other_needed - other_delivered[village_id]);
                double effective_vo = min((double)vo, other_room_left);
                village_values[village_id] += effective_vo * data.packages[2].value;

                food_delivered[village_id] += food_in_this_drop;
                other_delivered[village_id] += vo;
                
                trip_distance += distance(current_location, village.coords);
                current_location = village.coords;
            }

            if (total_d_dropped > d || total_p_dropped > p || total_o_dropped > o) {
                 cout << "*** WARNING: Heli " << helicopter_id << ", Trip " << i + 1 << " drops more packages than picked up." << endl;
                 constraint_violated = true;
            }

            trip_distance += distance(current_location, home_city_coords);
            if (trip_distance > helicopter.distance_capacity + 1e-9) {
                cout << "*** WARNING: Heli " << helicopter_id << ", Trip " << i + 1 << " exceeds trip distance capacity (" << trip_distance << " > " << helicopter.distance_capacity << ")." << endl;
                constraint_violated = true;
            }
            
            helicopter_total_distances[helicopter_id] += trip_distance;
            double trip_cost = (num_villages_in_trip > 0) ? (helicopter.fixed_cost + (helicopter.alpha * trip_distance)) : 0;
            total_trip_cost += trip_cost;
        }

        if (helicopter_total_distances[helicopter_id] > data.d_max + 1e-9) {
             cout << "*** WARNING: Heli " << helicopter_id << " exceeds DMax (" << helicopter_total_distances[helicopter_id] << " > " << data.d_max << ")." << endl;
             constraint_violated = true;
        }

        getline(outfile, line);
        line_num++;
    }
    
    double total_value = accumulate(village_values.begin(), village_values.end(), 0.0);
    double final_score = total_value - total_trip_cost;
    
    cout << "\n--- Final Calculation ---" << endl;
    cout << "Total Value Gained: " << total_value << endl;
    cout << "Total Trip Cost   : " << total_trip_cost << endl;
    cout << "Objective Score   = " << total_value << " - " << total_trip_cost << " = " << final_score << endl;

    if (constraint_violated) {
        cout << "\n*** WARNING: CONSTRAINTS VIOLATED. Score is invalid. ***" << endl;
        return -1.0;
    }
    cout << "\n--- All constraints satisfied. ---" << endl;
    return final_score;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_filename> <output_filename>" << endl;
        return 1;
    }
    try {
        double score = verifyAndCalculateScore(argv[1], argv[2]);
        cout << "\n----------------------------------------\n" << "FINAL SCORE: " << score << "\n----------------------------------------" << endl;
    } catch (const exception& e) {
        cerr << "An error occurred: " << e.what() << endl;
        return 1;
    }
    return 0;
}