#include <algorithm>
#include <cctype>
#include <chrono>
#include <deque>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <vector>

using namespace std;

struct Vehicle {
    string vehicleNumber;
    string ownerName;
    string vehicleType;
    int level = 0;
    int slot = 0;
    int entryTime = 0;
};

struct PriorityRequest {
    Vehicle vehicle;
    int priority = 3;
};

struct RequestOrder {
    bool operator()(const PriorityRequest& left, const PriorityRequest& right) const {
        if (left.priority != right.priority) {
            return left.priority > right.priority;
        }
        return left.vehicle.vehicleNumber > right.vehicle.vehicleNumber;
    }
};

class ParkSmart {
private:
    static constexpr int LEVELS = 3;
    static constexpr int SLOTS_PER_LEVEL = 5;
    static constexpr char LEVEL_PREFIXES[LEVELS] = {'A', 'B', 'C'};

    int parking[LEVELS][SLOTS_PER_LEVEL]{};
    vector<Vehicle> vehicles;
    queue<Vehicle> waitingQueue;
    stack<Vehicle> exitHistory;
    map<string, Vehicle> vehicleMap;
    set<string> vehicleTypes;
    deque<string> recentEntries;
    vector<string> entryHistory;
    map<string, double> hourlyRates{
        {"Car", 20.0}, {"Bike", 10.0}, {"EV", 25.0}, {"Truck", 40.0}};

    static string trim(const string& text) {
        const size_t first = text.find_first_not_of(" \t\r\n");
        if (first == string::npos) {
            return "";
        }
        const size_t last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    static string lowercase(string text) {
        transform(text.begin(), text.end(), text.begin(),
                  [](unsigned char character) { return static_cast<char>(tolower(character)); });
        return text;
    }

    static string readLine(const string& prompt) {
        cout << prompt;
        string value;
        getline(cin, value);
        return trim(value);
    }

    static int currentHour() {
        const auto now = chrono::system_clock::now().time_since_epoch();
        return static_cast<int>(chrono::duration_cast<chrono::hours>(now).count());
    }

    static int totalSlots() {
        return LEVELS * SLOTS_PER_LEVEL;
    }

    static string slotName(int level, int slot) {
        return string(1, LEVEL_PREFIXES[level - 1]) + to_string(slot);
    }

    int findAvailableSlot(Vehicle& vehicle) {
        for (int level = 0; level < LEVELS; ++level) {
            for (int slot = 0; slot < SLOTS_PER_LEVEL; ++slot) {
                if (parking[level][slot] == 0) {
                    parking[level][slot] = 1;
                    vehicle.level = level + 1;
                    vehicle.slot = slot + 1;
                    vehicle.entryTime = currentHour();
                    return 1;
                }
            }
        }
        return 0;
    }

    bool isDuplicate(const string& vehicleNumber) const {
        if (vehicleMap.find(vehicleNumber) != vehicleMap.end()) {
            return true;
        }
        queue<Vehicle> pending = waitingQueue;
        while (!pending.empty()) {
            if (pending.front().vehicleNumber == vehicleNumber) {
                return true;
            }
            pending.pop();
        }
        return false;
    }

    static bool validVehicleType(const string& type) {
        return type == "Car" || type == "Bike" || type == "EV" || type == "Truck";
    }

    static void printVehicle(const Vehicle& vehicle) {
        cout << "Vehicle Number : " << vehicle.vehicleNumber << '\n'
             << "Owner          : " << vehicle.ownerName << '\n'
             << "Type           : " << vehicle.vehicleType << '\n'
             << "Level          : " << vehicle.level << '\n'
             << "Slot           : " << slotName(vehicle.level, vehicle.slot) << '\n';
    }

    void registerParkedVehicle(const Vehicle& vehicle) {
        vehicles.push_back(vehicle);
        vehicleMap[vehicle.vehicleNumber] = vehicle;
        vehicleTypes.insert(vehicle.vehicleType);
        entryHistory.push_back(vehicle.vehicleNumber);
        recentEntries.push_back(vehicle.vehicleNumber);
        if (recentEntries.size() > 10) {
            recentEntries.pop_front();
        }
    }

    void displayParked(const Vehicle& vehicle) const {
        cout << "\nVehicle parked successfully!\n\n"
             << "Vehicle Number : " << vehicle.vehicleNumber << '\n'
             << "Level          : " << vehicle.level << '\n'
             << "Slot           : " << slotName(vehicle.level, vehicle.slot) << "\n";
    }

    void addToWaitingQueue(const Vehicle& vehicle) {
        waitingQueue.push(vehicle);
        cout << "\nParking Full!\n\nVehicle added to waiting queue.\n";
    }

    void serveWaitingVehicle() {
        if (waitingQueue.empty()) {
            return;
        }
        Vehicle nextVehicle = waitingQueue.front();
        waitingQueue.pop();
        if (findAvailableSlot(nextVehicle)) {
            registerParkedVehicle(nextVehicle);
            cout << "\nWaiting vehicle " << nextVehicle.vehicleNumber
                 << " has been parked automatically at Level " << nextVehicle.level
                 << ", Slot " << slotName(nextVehicle.level, nextVehicle.slot) << ".\n";
        } else {
            waitingQueue.push(nextVehicle);
            cout << "\nUnable to serve the waiting queue: no slot is available.\n";
        }
    }

    Vehicle* findVehicle(const string& vehicleNumber) {
        auto found = find_if(vehicles.begin(), vehicles.end(), [&](const Vehicle& vehicle) {
            return vehicle.vehicleNumber == vehicleNumber;
        });
        return found == vehicles.end() ? nullptr : &(*found);
    }

    void printSearchResults(const vector<Vehicle>& results) const {
        if (results.empty()) {
            cout << "\nNo matching active vehicle was found.\n";
            return;
        }
        cout << "\n========== SEARCH RESULTS ==========\n";
        for (const Vehicle& vehicle : results) {
            cout << '\n';
            printVehicle(vehicle);
        }
    }

public:
    void parkVehicle() {
        cout << "\n========== PARK VEHICLE ==========\n";
        Vehicle vehicle;
        vehicle.vehicleNumber = readLine("Vehicle Number: ");
        vehicle.ownerName = readLine("Owner Name: ");
        string enteredType = readLine("Vehicle Type (Car/Bike/EV/Truck): ");

        if (vehicle.vehicleNumber.empty() || vehicle.ownerName.empty()) {
            cout << "\nVehicle number and owner name cannot be empty.\n";
            return;
        }
        if (isDuplicate(vehicle.vehicleNumber)) {
            cout << "\nError: this vehicle number is already parked or waiting.\n";
            return;
        }

        const string normalizedType = lowercase(enteredType);
        if (normalizedType == "car") vehicle.vehicleType = "Car";
        else if (normalizedType == "bike") vehicle.vehicleType = "Bike";
        else if (normalizedType == "ev") vehicle.vehicleType = "EV";
        else if (normalizedType == "truck") vehicle.vehicleType = "Truck";
        else {
            cout << "\nError: invalid vehicle type. Choose Car, Bike, EV, or Truck.\n";
            return;
        }

        if (findAvailableSlot(vehicle)) {
            registerParkedVehicle(vehicle);
            displayParked(vehicle);
        } else {
            addToWaitingQueue(vehicle);
        }
    }

    void removeVehicle() {
        cout << "\n========== REMOVE VEHICLE ==========\n";
        const string number = readLine("Vehicle Number: ");
        Vehicle* vehicle = findVehicle(number);
        if (vehicle == nullptr) {
            cout << "\nVehicle not found in the parking area.\n";
            return;
        }

        const Vehicle removed = *vehicle;
        cout << "\nVehicle found.\n\n"
             << "Vehicle Number : " << removed.vehicleNumber << '\n'
             << "Level          : " << removed.level << '\n'
             << "Slot           : " << slotName(removed.level, removed.slot) << "\n\n";

        const int elapsedHours = currentHour() - removed.entryTime;
        if (elapsedHours < 0) {
            cout << "Error: system time moved backwards; removal was cancelled.\n";
            return;
        }
        const int billableHours = max(1, elapsedHours);
        const double fee = billableHours * hourlyRates.at(removed.vehicleType);
        parking[removed.level - 1][removed.slot - 1] = 0;
        vehicleMap.erase(removed.vehicleNumber);
        vehicles.erase(remove_if(vehicles.begin(), vehicles.end(), [&](const Vehicle& item) {
            return item.vehicleNumber == removed.vehicleNumber;
        }), vehicles.end());
        exitHistory.push(removed);

        cout << "Vehicle removed successfully.\n"
             << "Parking Duration : " << elapsedHours << " hour(s)\n"
             << "Billable Hours   : " << billableHours << '\n'
             << "Parking Fee      : Rs. " << fixed << setprecision(2) << fee << "\n";
        serveWaitingVehicle();
    }

    void displayParking() const {
        cout << "\n============== PARKING AREA ==============\n";
        for (int level = 0; level < LEVELS; ++level) {
            cout << "\nLevel " << level + 1 << "\n";
            for (int slot = 0; slot < SLOTS_PER_LEVEL; ++slot) {
                cout << left << setw(7) << slotName(level + 1, slot + 1);
                if (parking[level][slot] == 0) {
                    cout << "[FREE]  ";
                } else {
                    const auto found = find_if(vehicles.begin(), vehicles.end(),
                        [&](const Vehicle& vehicle) {
                            return vehicle.level == level + 1 && vehicle.slot == slot + 1;
                        });
                    cout << '[' << (found == vehicles.end() ? "OCCUPIED" : found->vehicleType)
                         << "]  ";
                }
            }
            cout << '\n';
        }
    }

    void searchVehicle() const {
        cout << "\n========== SEARCH VEHICLE ==========\n"
             << "1. Vehicle number\n2. Owner name\n3. Vehicle type\n";
        const string choice = readLine("Search by: ");
        const string query = readLine("Search text: ");
        vector<Vehicle> results;
        if (choice == "1") {
            auto found = vehicleMap.find(query);
            if (found != vehicleMap.end()) {
                results.push_back(found->second);
            }
        } else if (choice == "2" || choice == "3") {
            const string normalizedQuery = lowercase(query);
            for (const Vehicle& vehicle : vehicles) {
                const string field = lowercase(choice == "2" ? vehicle.ownerName : vehicle.vehicleType);
                if (field.find(normalizedQuery) != string::npos) {
                    results.push_back(vehicle);
                }
            }
        } else {
            cout << "\nInvalid search choice.\n";
            return;
        }
        printSearchResults(results);
    }

    void showAvailableSlots() const {
        int occupied = 0;
        for (int level = 0; level < LEVELS; ++level) {
            int availableOnLevel = 0;
            for (int slot = 0; slot < SLOTS_PER_LEVEL; ++slot) {
                if (parking[level][slot] == 0) {
                    ++availableOnLevel;
                } else {
                    ++occupied;
                }
            }
            cout << "Level " << level + 1 << " available: " << availableOnLevel << '\n';
        }
        cout << "\nTotal Slots     : " << totalSlots()
             << "\nOccupied Slots  : " << occupied
             << "\nAvailable Slots : " << totalSlots() - occupied << '\n';
    }

    void showOccupiedSlots() const {
        cout << "\n========== OCCUPIED SLOTS ==========\n";
        if (vehicles.empty()) {
            cout << "No vehicles are currently parked.\n";
            return;
        }
        for (const Vehicle& vehicle : vehicles) {
            cout << "Level " << vehicle.level << ", Slot "
                 << slotName(vehicle.level, vehicle.slot) << " : "
                 << vehicle.vehicleNumber << " (" << vehicle.vehicleType
                 << ", " << vehicle.ownerName << ")\n";
        }
    }

    void displayWaitingQueue() const {
        cout << "\n========== WAITING QUEUE ==========\n";
        if (waitingQueue.empty()) {
            cout << "The waiting queue is empty.\n";
            return;
        }
        queue<Vehicle> pending = waitingQueue;
        int position = 1;
        while (!pending.empty()) {
            cout << position++ << ". " << pending.front().vehicleNumber
                 << " (" << pending.front().vehicleType << ")\n";
            pending.pop();
        }
        cout << "Queue front: " << waitingQueue.front().vehicleNumber << '\n';
    }

    void displayEntryHistory() const {
        cout << "\n========== ENTRY HISTORY ==========\n";
        if (entryHistory.empty()) {
            cout << "No vehicle entries recorded.\n";
            return;
        }
        for (size_t index = 0; index < entryHistory.size(); ++index) {
            cout << index + 1 << ". " << entryHistory[index] << '\n';
        }
        cout << "\nRecent entries (deque, newest first):\n";
        for (auto iterator = recentEntries.rbegin(); iterator != recentEntries.rend(); ++iterator) {
            cout << "- " << *iterator << '\n';
        }
    }

    void displayExitHistory() const {
        cout << "\n========== EXIT HISTORY ==========\n"
             << "Stack follows LIFO (Last In, First Out).\n";
        if (exitHistory.empty()) {
            cout << "No vehicle has exited yet.\n";
            return;
        }
        stack<Vehicle> recentExits = exitHistory;
        int position = 1;
        while (!recentExits.empty()) {
            cout << position++ << ". " << recentExits.top().vehicleNumber
                 << " (" << recentExits.top().vehicleType << ")\n";
            recentExits.pop();
        }
        cout << "Most recently exited: " << exitHistory.top().vehicleNumber << '\n';
    }

    void sortVehicles() const {
        cout << "\n========== SORT VEHICLES ==========\n"
             << "1. Vehicle number\n2. Owner name\n3. Parking level\n4. Entry time\n";
        const string choice = readLine("Sort by: ");
        vector<Vehicle> sorted = vehicles;
        if (choice == "1") {
            sort(sorted.begin(), sorted.end(), [](const Vehicle& left, const Vehicle& right) {
                return left.vehicleNumber < right.vehicleNumber;
            });
        } else if (choice == "2") {
            sort(sorted.begin(), sorted.end(), [](const Vehicle& left, const Vehicle& right) {
                return lowercase(left.ownerName) < lowercase(right.ownerName);
            });
        } else if (choice == "3") {
            sort(sorted.begin(), sorted.end(), [](const Vehicle& left, const Vehicle& right) {
                return left.level != right.level ? left.level < right.level : left.slot < right.slot;
            });
        } else if (choice == "4") {
            sort(sorted.begin(), sorted.end(), [](const Vehicle& left, const Vehicle& right) {
                return left.entryTime < right.entryTime;
            });
        } else {
            cout << "\nInvalid sort choice.\n";
            return;
        }
        if (sorted.empty()) {
            cout << "\nNo parked vehicles to sort.\n";
            return;
        }
        for (const Vehicle& vehicle : sorted) {
            cout << vehicle.vehicleNumber << " | " << vehicle.ownerName
                 << " | " << vehicle.vehicleType << " | Level " << vehicle.level
                 << " | " << slotName(vehicle.level, vehicle.slot)
                 << " | Entry hour " << vehicle.entryTime << '\n';
        }
    }

    void showStatistics() const {
        map<string, int> counts{{"Car", 0}, {"Bike", 0}, {"EV", 0}, {"Truck", 0}};
        for (const Vehicle& vehicle : vehicles) {
            ++counts[vehicle.vehicleType];
        }
        const int occupied = static_cast<int>(vehicles.size());
        const double occupancyRate = totalSlots() == 0 ? 0.0
            : 100.0 * occupied / totalSlots();
        cout << "\n========== PARKING STATISTICS ==========\n"
             << "Total Levels       : " << LEVELS
             << "\nSlots Per Level    : " << SLOTS_PER_LEVEL
             << "\nTotal Slots        : " << totalSlots()
             << "\nOccupied Slots     : " << occupied
             << "\nAvailable Slots    : " << totalSlots() - occupied
             << "\nCars               : " << counts["Car"]
             << "\nBikes              : " << counts["Bike"]
             << "\nEVs                : " << counts["EV"]
             << "\nTrucks             : " << counts["Truck"]
             << fixed << setprecision(2)
             << "\nOccupancy Rate     : " << occupancyRate << "%\n";
        cout << "Unique Types       : ";
        if (vehicleTypes.empty()) {
            cout << "None";
        } else {
            for (auto iterator = vehicleTypes.begin(); iterator != vehicleTypes.end(); ++iterator) {
                if (iterator != vehicleTypes.begin()) {
                    cout << ", ";
                }
                cout << *iterator;
            }
        }
        cout << '\n';
        cout << "\nHourly rates: Car Rs. " << hourlyRates.at("Car")
             << ", Bike Rs. " << hourlyRates.at("Bike")
             << ", EV Rs. " << hourlyRates.at("EV")
             << ", Truck Rs. " << hourlyRates.at("Truck")
             << " (edit hourlyRates in main.cpp to configure).\n";
    }

    void trafficSimulation() const {
        cout << "\n========== TRAFFIC FLOW ==========\n"
             << "Vehicles approach in FIFO order; special requests use a priority queue.\n"
             << "Entrance\n   v\n";
        queue<string> approachingTraffic;
        approachingTraffic.push("Car");
        approachingTraffic.push("Car");
        approachingTraffic.push("Bike");
        approachingTraffic.push("EV");
        approachingTraffic.push("Car");
        queue<string> displayQueue = approachingTraffic;
        cout << "   ";
        while (!displayQueue.empty()) {
            cout << displayQueue.front();
            displayQueue.pop();
            if (!displayQueue.empty()) cout << " -> ";
        }
        cout << "\n   v\nParking Allocation\n\nProcessing traffic:\n";
        while (!approachingTraffic.empty()) {
            cout << "Vehicle at entrance: " << approachingTraffic.front()
                 << " -> ";
            approachingTraffic.pop();
            cout << "admitted to allocation queue.\n";
        }

        priority_queue<PriorityRequest, vector<PriorityRequest>, RequestOrder> requests;
        requests.push({Vehicle{"REG-001", "Regular Driver", "Car"}, 3});
        requests.push({Vehicle{"EV-001", "EV Driver", "EV"}, 2});
        requests.push({Vehicle{"EMS-001", "Emergency Unit", "Car"}, 1});
        cout << "\nSpecial request priority order (1=Emergency, 2=EV, 3=Regular):\n";
        while (!requests.empty()) {
            cout << requests.top().priority << " - "
                 << requests.top().vehicle.vehicleNumber << " ("
                 << (requests.top().priority == 1 ? "Emergency"
                     : requests.top().priority == 2 ? "EV" : "Regular")
                 << ")\n";
            requests.pop();
        }
        cout << "\nA queue models entrance FIFO flow; a priority queue can move "
                "emergency requests ahead of regular traffic.\n";
    }

    void complexityAnalysis() const {
        cout << "\n========== COMPLEXITY ANALYSIS ==========\n"
             << left << setw(30) << "Operation" << "Time Complexity\n"
             << string(48, '-') << '\n'
             << left << setw(30) << "Parking search" << "O(n)\n"
             << left << setw(30) << "Vehicle insertion (vector)" << "O(1) average\n"
             << left << setw(30) << "Queue enqueue / dequeue" << "O(1)\n"
             << left << setw(30) << "Stack push / pop" << "O(1)\n"
             << left << setw(30) << "Map search" << "O(log n)\n"
             << left << setw(30) << "Sorting active vehicles" << "O(n log n)\n"
             << left << setw(30) << "Parking layout display" << "O(levels * slots)\n"
             << left << setw(30) << "Space (active vehicles)" << "O(n)\n"
             << left << setw(30) << "Parking array" << "O(levels * slots)\n"
             << left << setw(30) << "History / waiting / exit" << "O(h + w + x)\n\n"
             << "n = active vehicles; h = recorded entries; w = waiting vehicles; "
                "x = exited vehicles retained.\n"
             << "The map is an ordered STL map, so lookup is O(log n). "
                "The slot scan examines at most 15 positions here.\n";
    }

    void showPageHeader(const string& page, const string& description) const {
        cout << "\n+------------------------------------------------------------------+\n"
             << "| PARKSMART                                      SMART PARKING    |\n"
             << "+------------------------------------------------------------------+\n"
             << "  " << page << "\n"
             << "  " << description << "\n"
             << "  PARKED " << vehicles.size() << "/" << totalSlots()
             << "    FREE " << totalSlots() - vehicles.size()
             << "    WAITING " << waitingQueue.size() << "\n"
             << "+------------------------------------------------------------------+\n";
    }

    int readMenuChoice(const string& prompt, int optionCount) const {
        const string choice = readLine(prompt);
        if (cin.eof()) {
            return -1;
        }

        const bool numericChoice = !choice.empty() &&
            all_of(choice.begin(), choice.end(), [](unsigned char character) {
                return isdigit(character) != 0;
            });
        if (!numericChoice) {
            cout << "\nPlease enter a number from 1 to " << optionCount << ".\n";
            return 0;
        }

        int value = 0;
        for (char digit : choice) {
            value = min(optionCount + 1, value * 10 + (digit - '0'));
        }
        if (value < 1 || value > optionCount) {
            cout << "\nPlease enter a number from 1 to " << optionCount << ".\n";
            return 0;
        }
        return value;
    }

    void showDashboard() const {
        showPageHeader("DASHBOARD", "Your parking operations, organized in one place.");
        cout << "\n  OVERVIEW\n"
             << "  " << vehicles.size() << " vehicles parked   |   "
             << totalSlots() - vehicles.size() << " slots available   |   "
             << waitingQueue.size() << " vehicles waiting\n"
             << "\n  PAGES\n"
             << "  1  Parking operations       Arrivals, checkout, and live slots\n"
             << "  2  Vehicle directory        Search, browse, and sort vehicles\n"
             << "  3  Activity & history       Waiting line and entry/exit records\n"
             << "  4  Reports & tools          Statistics, traffic, and analysis\n"
             << "  5  Exit ParkSmart\n";
    }

    bool runParkingPage() {
        while (true) {
            showPageHeader("PARKING OPERATIONS", "Manage arrivals, departures, and the parking floor.");
            cout << "\n  1  Park a vehicle\n"
                 << "  2  Check out a vehicle\n"
                 << "  3  View parking layout\n"
                 << "  4  Check available slots\n"
                 << "  5  Back to dashboard\n";
            const int choice = readMenuChoice("  Select an activity: ", 5);
            if (choice == -1) return false;
            switch (choice) {
                case 1: parkVehicle(); break;
                case 2: removeVehicle(); break;
                case 3: displayParking(); break;
                case 4: showAvailableSlots(); break;
                case 5: return true;
                default: break;
            }
        }
    }

    bool runVehiclePage() {
        while (true) {
            showPageHeader("VEHICLE DIRECTORY", "Find and review currently parked vehicles.");
            cout << "\n  1  Search vehicles\n"
                 << "  2  View all occupied slots\n"
                 << "  3  Sort parked vehicles\n"
                 << "  4  Back to dashboard\n";
            const int choice = readMenuChoice("  Select an activity: ", 4);
            if (choice == -1) return false;
            switch (choice) {
                case 1: searchVehicle(); break;
                case 2: showOccupiedSlots(); break;
                case 3: sortVehicles(); break;
                case 4: return true;
                default: break;
            }
        }
    }

    bool runActivityPage() {
        while (true) {
            showPageHeader("ACTIVITY & HISTORY", "Review queued arrivals and vehicle movement records.");
            cout << "\n  1  View waiting queue\n"
                 << "  2  View vehicle entry history\n"
                 << "  3  View vehicle exit history\n"
                 << "  4  Back to dashboard\n";
            const int choice = readMenuChoice("  Select an activity: ", 4);
            if (choice == -1) return false;
            switch (choice) {
                case 1: displayWaitingQueue(); break;
                case 2: displayEntryHistory(); break;
                case 3: displayExitHistory(); break;
                case 4: return true;
                default: break;
            }
        }
    }

    bool runReportsPage() {
        while (true) {
            showPageHeader("REPORTS & TOOLS", "Facility metrics and learning tools.");
            cout << "\n  1  Parking statistics\n"
                 << "  2  Traffic flow simulation\n"
                 << "  3  Data-structure complexity\n"
                 << "  4  Back to dashboard\n";
            const int choice = readMenuChoice("  Select an activity: ", 4);
            if (choice == -1) return false;
            switch (choice) {
                case 1: showStatistics(); break;
                case 2: trafficSimulation(); break;
                case 3: complexityAnalysis(); break;
                case 4: return true;
                default: break;
            }
        }
    }

    void run() {
        while (true) {
            showDashboard();
            const int choice = readMenuChoice("  Open a page: ", 5);
            if (choice == -1) {
                cout << "\nInput closed. Exiting ParkSmart.\n";
                return;
            }
            switch (choice) {
                case 1:
                    if (!runParkingPage()) {
                        cout << "\nInput closed. Exiting ParkSmart.\n";
                        return;
                    }
                    break;
                case 2:
                    if (!runVehiclePage()) {
                        cout << "\nInput closed. Exiting ParkSmart.\n";
                        return;
                    }
                    break;
                case 3:
                    if (!runActivityPage()) {
                        cout << "\nInput closed. Exiting ParkSmart.\n";
                        return;
                    }
                    break;
                case 4:
                    if (!runReportsPage()) {
                        cout << "\nInput closed. Exiting ParkSmart.\n";
                        return;
                    }
                    break;
                case 5:
                    cout << "\nThank you for using ParkSmart.\n";
                    return;
                default: break;
            }
        }
    }
};

int main() {
    ParkSmart system;
    system.run();
    return 0;
}
