#ifndef FUEL_MANAGER_H
#define FUEL_MANAGER_H

/**
 * @brief FuelManager manages the vehicle's fuel-related state and constraints.
 */
class FuelManager {
private:
    double tank_capacity;
    double fuel_step; // The incremental unit size for purchasing fuel

public:
    FuelManager(double capacity = 50.0, double step = 1.0)
        : tank_capacity(capacity), fuel_step(step) {}

    double get_tank_capacity() const { return tank_capacity; }
    double get_fuel_step() const { return fuel_step; }

    /**
     * @brief Checks if travel is possible with the current fuel level.
     */
    bool can_travel(double current_fuel, double consumption) const {
        return current_fuel >= consumption;
    }

    /**
     * @brief Deducts fuel consumption from current fuel.
     */
    double travel_remaining_fuel(double current_fuel, double consumption) const {
        return current_fuel - consumption;
    }

    /**
     * @brief Calculates how much fuel can be bought in the current step.
     * Prevents exceeding the tank capacity.
     */
    double get_refuel_amount(double current_fuel) const {
        if (current_fuel >= tank_capacity) {
            return 0.0;
        }
        double remaining = tank_capacity - current_fuel;
        return (remaining < fuel_step) ? remaining : fuel_step;
    }

    /**
     * @brief Calculates cost of purchasing a given amount of fuel.
     */
    double calculate_refuel_cost(double amount, double fuel_price) const {
        return amount * fuel_price;
    }
};

#endif // FUEL_MANAGER_H
