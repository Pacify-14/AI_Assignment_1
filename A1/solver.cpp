#include "solver.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <random>
#include <queue>
#include <unordered_set>
#include <cmath>
#include <limits>

using namespace std;

class HybridSolver {
private:
    const ProblemData* problem;
    mutable mt19937 rng;
    
    // Tabu Search Parameters
    queue<string> tabuList;
    static const int TABU_SIZE = 100;
    
    // Simulated Annealing Parameters
    static constexpr double INITIAL_TEMP = 10000.0;
    static constexpr double COOLING_RATE = 0.98;
    static constexpr double MIN_TEMP = 0.1;
    
    // Algorithm control
    chrono::steady_clock::time_point startTime;
    double timeLimitSeconds;
    
public:
    HybridSolver(const ProblemData& prob) : problem(&prob), rng(chrono::steady_clock::now().time_since_epoch().count()) {
        timeLimitSeconds = prob.time_limit_minutes * 60.0 - 2.0; // Leave 2 second buffer
        startTime = chrono::steady_clock::now();
    }
    
    bool timeLeft() const {
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - startTime).count() / 1000.0;
        return elapsed < timeLimitSeconds;
    }
    
    double calculateTripDistance(const Trip& trip, int helicopterId) const {
        if (trip.drops.empty()) return 0.0;
        
        Point homeCity = problem->cities[problem->helicopters[helicopterId].home_city_id - 1];
        Point current = homeCity;
        double totalDist = 0.0;
        
        for (const auto& drop : trip.drops) {
            Point villageLoc = problem->villages[drop.village_id - 1].coords;
            totalDist += distance(current, villageLoc);
            current = villageLoc;
        }
        totalDist += distance(current, homeCity);
        return totalDist;
    }
    
    double calculateTripWeight(const Trip& trip) const {
        return trip.dry_food_pickup * problem->packages[0].weight +
               trip.perishable_food_pickup * problem->packages[1].weight +
               trip.other_supplies_pickup * problem->packages[2].weight;
    }
    
    bool isValidSolution(const Solution& solution) const {
        vector<double> helicopterTotalDist(problem->helicopters.size(), 0.0);
        
        for (const auto& plan : solution) {
            int heliIdx = plan.helicopter_id - 1;
            
            for (const auto& trip : plan.trips) {
                // Check weight constraint
                if (calculateTripWeight(trip) > problem->helicopters[heliIdx].weight_capacity + 1e-9) {
                    return false;
                }
                
                // Check trip distance constraint
                double tripDist = calculateTripDistance(trip, heliIdx);
                if (tripDist > problem->helicopters[heliIdx].distance_capacity + 1e-9) {
                    return false;
                }
                
                helicopterTotalDist[heliIdx] += tripDist;
                
                // Check drop consistency
                int totalDry = 0, totalPerish = 0, totalOther = 0;
                for (const auto& drop : trip.drops) {
                    totalDry += drop.dry_food;
                    totalPerish += drop.perishable_food;
                    totalOther += drop.other_supplies;
                }
                
                if (totalDry > trip.dry_food_pickup || 
                    totalPerish > trip.perishable_food_pickup ||
                    totalOther > trip.other_supplies_pickup) {
                    return false;
                }
            }
            
            // Check total distance constraint
            if (helicopterTotalDist[heliIdx] > problem->d_max + 1e-9) {
                return false;
            }
        }
        return true;
    }
    
    double evaluateSolution(const Solution& solution) const {
        if (!isValidSolution(solution)) {
            return -numeric_limits<double>::infinity();
        }
        
        vector<double> foodDelivered(problem->villages.size() + 1, 0.0);
        vector<double> otherDelivered(problem->villages.size() + 1, 0.0);
        double totalValue = 0.0;
        double totalCost = 0.0;
        
        for (const auto& plan : solution) {
            int heliIdx = plan.helicopter_id - 1;
            
            for (const auto& trip : plan.trips) {
                if (!trip.drops.empty()) {
                    double tripDist = calculateTripDistance(trip, heliIdx);
                    totalCost += problem->helicopters[heliIdx].fixed_cost + 
                                problem->helicopters[heliIdx].alpha * tripDist;
                }
                
                for (const auto& drop : trip.drops) {
                    int villageId = drop.village_id;
                    const auto& village = problem->villages[villageId - 1];
                    
                    // Food value calculation with capping
                    double maxFoodNeeded = village.population * 9.0;
                    double foodRoomLeft = max(0.0, maxFoodNeeded - foodDelivered[villageId]);
                    double foodInThisDrop = drop.dry_food + drop.perishable_food;
                    double effectiveFoodThisDrop = min(foodInThisDrop, foodRoomLeft);
                    
                    double effectivePerishable = min((double)drop.perishable_food, effectiveFoodThisDrop);
                    totalValue += effectivePerishable * problem->packages[1].value;
                    
                    double remainingEffectiveFood = effectiveFoodThisDrop - effectivePerishable;
                    double effectiveDry = min((double)drop.dry_food, remainingEffectiveFood);
                    totalValue += effectiveDry * problem->packages[0].value;
                    
                    foodDelivered[villageId] += foodInThisDrop;
                    
                    // Other supplies value calculation with capping
                    double maxOtherNeeded = village.population * 1.0;
                    double otherRoomLeft = max(0.0, maxOtherNeeded - otherDelivered[villageId]);
                    double effectiveOther = min((double)drop.other_supplies, otherRoomLeft);
                    totalValue += effectiveOther * problem->packages[2].value;
                    
                    otherDelivered[villageId] += drop.other_supplies;
                }
            }
        }
        
        return totalValue - totalCost;
    }
    
Solution generateInitialSolution() const {
    Solution solution;

    // Initialize helicopter plans
    for (const auto& helicopter : problem->helicopters) {
        HelicopterPlan plan;
        plan.helicopter_id = helicopter.id;
        solution.push_back(plan);
    }

    // Track how much each village has already received
    vector<int> foodDelivered(problem->villages.size() + 1, 0);
    vector<int> otherDelivered(problem->villages.size() + 1, 0);

    for (const auto& village : problem->villages) {
        int bestHeli = 0;
        double minDist = numeric_limits<double>::infinity();

        // Pick the nearest helicopter
        for (int h = 0; h < problem->helicopters.size(); h++) {
            Point heliHome = problem->cities[problem->helicopters[h].home_city_id - 1];
            double dist = distance(heliHome, village.coords);
            if (dist < minDist) {
                minDist = dist;
                bestHeli = h;
            }
        }

        // Compute remaining demand
        int remainingFood = max(0, village.population * 9 - foodDelivered[village.id]);
        int remainingOther = max(0, village.population - otherDelivered[village.id]);

        if (remainingFood == 0 && remainingOther == 0) continue;

        const auto& heli = problem->helicopters[bestHeli];
        double availableWeight = heli.weight_capacity;

        int dryFood = 0, perishableFood = 0, otherSupplies = 0;

        // Always allocate 'other' supplies first (up to population cap)
        if (remainingOther > 0) {
            int canTake = min(remainingOther, (int)(availableWeight / problem->packages[2].weight));
            otherSupplies = canTake;
            remainingOther -= canTake;
            availableWeight -= canTake * problem->packages[2].weight;
        }

        if (availableWeight > 1e-9 && remainingFood > 0) {
            // Compare ratio for dry vs perishable
            double ratioDry = problem->packages[0].value / problem->packages[0].weight;
            double ratioPerish = problem->packages[1].value / problem->packages[1].weight;

            // Allocate to the type with higher ratio first
            if (ratioPerish >= ratioDry) {
                int canTakePerish = min(remainingFood, (int)(availableWeight / problem->packages[1].weight));
                perishableFood = canTakePerish;
                remainingFood -= canTakePerish;
                availableWeight -= canTakePerish * problem->packages[1].weight;

                // Then dry
                if (availableWeight > 1e-9 && remainingFood > 0) {
                    int canTakeDry = min(remainingFood, (int)(availableWeight / problem->packages[0].weight));
                    dryFood = canTakeDry;
                    availableWeight -= canTakeDry * problem->packages[0].weight;
                }
            } else {
                // Allocate dry first
                int canTakeDry = min(remainingFood, (int)(availableWeight / problem->packages[0].weight));
                dryFood = canTakeDry;
                remainingFood -= canTakeDry;
                availableWeight -= canTakeDry * problem->packages[0].weight;

                // Then perishable
                if (availableWeight > 1e-9 && remainingFood > 0) {
                    int canTakePerish = min(remainingFood, (int)(availableWeight / problem->packages[1].weight));
                    perishableFood = canTakePerish;
                    availableWeight -= canTakePerish * problem->packages[1].weight;
                }
            }
        }

        if (dryFood > 0 || perishableFood > 0 || otherSupplies > 0) {
            Trip trip;
            trip.dry_food_pickup = dryFood;
            trip.perishable_food_pickup = perishableFood;
            trip.other_supplies_pickup = otherSupplies;

            Drop drop;
            drop.village_id = village.id;
            drop.dry_food = dryFood;
            drop.perishable_food = perishableFood;
            drop.other_supplies = otherSupplies;

            trip.drops.push_back(drop);
            solution[bestHeli].trips.push_back(trip);

            // Update delivered quantities
            foodDelivered[village.id] += dryFood + perishableFood;
            otherDelivered[village.id] += otherSupplies;
        }
    }

    return solution;
}

    
    string solutionToString(const Solution& solution) const {
        string result;
        for (const auto& plan : solution) {
            result += "H" + to_string(plan.helicopter_id) + ":";
            for (const auto& trip : plan.trips) {
                result += "T(" + to_string(trip.dry_food_pickup) + "," + 
                         to_string(trip.perishable_food_pickup) + "," +
                         to_string(trip.other_supplies_pickup) + ")[";
                for (const auto& drop : trip.drops) {
                    result += to_string(drop.village_id) + ",";
                }
                result += "]";
            }
            result += "|";
        }
        return result;
    }
    
    void addToTabu(const string& state) {
        tabuList.push(state);
        while (tabuList.size() > TABU_SIZE) {
            tabuList.pop();
        }
    }
    
    bool isTabu(const string& state) const {
        queue<string> temp = tabuList;
        while (!temp.empty()) {
            if (temp.front() == state) return true;
            temp.pop();
        }
        return false;
    }
    
    vector<Solution> generateNeighbors(const Solution& current) const {
        vector<Solution> neighbors;
        
        // Neighbor 1: Village reassignment
        for (int h1 = 0; h1 < current.size() && neighbors.size() < 15; h1++) {
            for (int h2 = h1 + 1; h2 < current.size() && neighbors.size() < 15; h2++) {
                if (!current[h1].trips.empty() && !current[h2].trips.empty()) {
                    for (int t1 = 0; t1 < current[h1].trips.size() && neighbors.size() < 15; t1++) {
                        for (int t2 = 0; t2 < current[h2].trips.size() && neighbors.size() < 15; t2++) {
                            if (!current[h1].trips[t1].drops.empty() && 
                                !current[h2].trips[t2].drops.empty()) {
                                Solution neighbor = current;
                                
                                // Swap villages between helicopters
                                swap(neighbor[h1].trips[t1].drops[0], neighbor[h2].trips[t2].drops[0]);
                                
                                // Update pickup amounts accordingly
                                neighbor[h1].trips[t1].dry_food_pickup = neighbor[h1].trips[t1].drops[0].dry_food;
                                neighbor[h1].trips[t1].perishable_food_pickup = neighbor[h1].trips[t1].drops[0].perishable_food;
                                neighbor[h1].trips[t1].other_supplies_pickup = neighbor[h1].trips[t1].drops[0].other_supplies;
                                
                                neighbor[h2].trips[t2].dry_food_pickup = neighbor[h2].trips[t2].drops[0].dry_food;
                                neighbor[h2].trips[t2].perishable_food_pickup = neighbor[h2].trips[t2].drops[0].perishable_food;
                                neighbor[h2].trips[t2].other_supplies_pickup = neighbor[h2].trips[t2].drops[0].other_supplies;
                                
                                neighbors.push_back(neighbor);
                            }
                        }
                    }
                }
            }
        }
        
        // Neighbor 2: Package type conversion (dry -> perishable when beneficial)
        for (int h = 0; h < current.size() && neighbors.size() < 25; h++) {
            for (int t = 0; t < current[h].trips.size() && neighbors.size() < 25; t++) {
                const auto& trip = current[h].trips[t];
                if (trip.dry_food_pickup > 0) {
                    Solution neighbor = current;
                    
                    // Convert some dry food to perishable food
                    int convertAmount = min(trip.dry_food_pickup, 10);
                    neighbor[h].trips[t].dry_food_pickup -= convertAmount;
                    neighbor[h].trips[t].perishable_food_pickup += convertAmount;
                    
                    // Update drops accordingly
                    for (auto& drop : neighbor[h].trips[t].drops) {
                        int canConvert = min(drop.dry_food, convertAmount);
                        drop.dry_food -= canConvert;
                        drop.perishable_food += canConvert;
                        convertAmount -= canConvert;
                        if (convertAmount == 0) break;
                    }
                    
                    neighbors.push_back(neighbor);
                }
            }
        }
        
        // Neighbor 3: Merge trips if possible
        for (int h = 0; h < current.size() && neighbors.size() < 35; h++) {
            if (current[h].trips.size() >= 2) {
                for (int t1 = 0; t1 < current[h].trips.size() - 1 && neighbors.size() < 35; t1++) {
                    for (int t2 = t1 + 1; t2 < current[h].trips.size() && neighbors.size() < 35; t2++) {
                        Solution neighbor = current;
                        
                        // Try to merge trip t2 into trip t1
                        Trip& trip1 = neighbor[h].trips[t1];
                        const Trip& trip2 = neighbor[h].trips[t2];
                        
                        double combinedWeight = calculateTripWeight(trip1) + calculateTripWeight(trip2);
                        if (combinedWeight <= problem->helicopters[h].weight_capacity) {
                            trip1.dry_food_pickup += trip2.dry_food_pickup;
                            trip1.perishable_food_pickup += trip2.perishable_food_pickup;
                            trip1.other_supplies_pickup += trip2.other_supplies_pickup;
                            
                            trip1.drops.insert(trip1.drops.end(), trip2.drops.begin(), trip2.drops.end());
                            neighbor[h].trips.erase(neighbor[h].trips.begin() + t2);
                            
                            neighbors.push_back(neighbor);
                        }
                    }
                }
            }
        }
        
        return neighbors;
    }
    
    Solution simulatedAnnealingWithTabu() {
        Solution current = generateInitialSolution();
        Solution best = current;
        double bestScore = evaluateSolution(best);
        double temperature = INITIAL_TEMP;
        
        uniform_real_distribution<double> prob(0.0, 1.0);
        
        while (temperature > MIN_TEMP && timeLeft()) {
            vector<Solution> neighbors = generateNeighbors(current);
            if (neighbors.empty()) break;
            
            // Filter out tabu neighbors
            vector<Solution> validNeighbors;
            for (const auto& neighbor : neighbors) {
                string neighborStr = solutionToString(neighbor);
                if (!isTabu(neighborStr)) {
                    validNeighbors.push_back(neighbor);
                }
            }
            
            if (validNeighbors.empty()) {
                validNeighbors = neighbors; // Accept tabu if no other options
            }
            
            // Select a neighbor
            Solution selected = validNeighbors[rng() % validNeighbors.size()];
            double selectedScore = evaluateSolution(selected);
            
            double delta = selectedScore - evaluateSolution(current);
            
            // Accept or reject based on simulated annealing icriteria
            if (delta > 0 || prob(rng) < exp(delta / temperature)) {
                current = selected;
                addToTabu(solutionToString(current));
                
                if (selectedScore > bestScore) {
                    best = current;
                    bestScore = selectedScore;
                }
            }
            
            temperature *= COOLING_RATE;
        }
        
        return best;
    }
    
    Solution hybridSearchWithRestarts() {
        Solution bestOverall = generateInitialSolution();
        double bestScore = evaluateSolution(bestOverall);
        
        int restart = 0;
        int maxRestarts = 10;
        
        while (restart < maxRestarts && timeLeft()) {
            // Clear tabu list for new restart
            while (!tabuList.empty()) tabuList.pop();
            
            Solution result = simulatedAnnealingWithTabu();
            double resultScore = evaluateSolution(result);
            
            if (resultScore > bestScore) {
                bestOverall = result;
                bestScore = resultScore;
            }
            
            restart++;
        }
        
        return bestOverall;
    }
};

Solution solve(const ProblemData& problem) {
    cout << "Starting Hybrid SA+Tabu+Restart solver..." << endl;
    
    HybridSolver solver(problem);
    Solution solution = solver.hybridSearchWithRestarts();
    
    cout << "Solver finished. Final score: " << solver.evaluateSolution(solution) << endl;
    return solution;
}