# ParkSmart

**ParkSmart** includes a browser-based parking management interface and a C++17 console edition. The browser app offers a live dashboard, parking floor, vehicle directory, activity log, and reports. It stores data in the current browser on the current device; it does not connect to a cloud service or share records between users.

## Start the graphical interface

Run these commands in PowerShell from the project folder:

```powershell
py -m http.server 8000
```

Open [http://localhost:8000](http://localhost:8000) in your browser. Keep the PowerShell window open while using the app; press **Ctrl+C** there to stop the local server. Python's built-in HTTP server is used only to serve the static app—no packages or internet connection are required.

Alternatively, open `index.html` directly in a browser. Using the local server is recommended for consistent browser storage behavior.

### What the browser app does

- Registers Cars, Bikes, EVs, and Trucks; assigns the first free space across three levels, with five spaces per level.
- Adds arrivals to a first-in, first-out queue when all 15 spaces are occupied, then admits the next waiting vehicle after a checkout.
- Searches the vehicle directory, shows current parking spaces, and provides check-out actions.
- Calculates checkout fees at the displayed hourly rates, with a one-hour minimum.
- Keeps arrival, queue, and checkout history and displays current facility reports.
- Saves changes in the browser's local storage, so data survives reloads in that browser. Use the top-right reset button to clear local demo data.

This is a useful local single-device application, not a multi-user production service. Browser data is not backed up, synced, or protected by user accounts. For a shared production deployment, connect the interface to a secured backend and persistent database.

## Build and run the C++ console edition

The browser app above does not require compiling C++. The original C++17 console application remains available:

```sh
g++ main.cpp -std=c++17 -o parksmart
./parksmart
```

On Windows with MinGW:

```powershell
g++ main.cpp -std=c++17 -o parksmart.exe
.\parksmart.exe
```

No third-party C++ libraries are required. Parking is allocated to the first available slot in level/slot order.

## Interface pages

The browser dashboard shows current parked, available, and waiting counts. Use the sidebar to move between these pages:

| Page | Activities |
|---|---|
| Parking operations | Park a vehicle, check out a vehicle, view the parking layout, and check available slots |
| Vehicle directory | Search vehicles, view occupied slots, and sort parked vehicles |
| Activity & history | View the waiting queue and vehicle entry/exit histories |
| Reports & tools | View parking statistics, run the traffic simulation, and review data-structure complexity |

The browser app saves records in the current browser's local storage. The C++ console edition uses its interactive terminal menus and keeps its data in memory for the current run.

## Project explanation

ParkSmart models a parking facility with three levels and five slots per level. When a vehicle arrives, the application validates its details, searches the occupancy matrix, and either parks it in the first free slot or appends it to a FIFO waiting queue. Removing an active vehicle clears its slot, calculates a fee, records the exit on a LIFO stack, and automatically parks the first waiting vehicle if one exists.

The application keeps active-vehicle information synchronized across a vector and a map. Entry history is retained for the whole run, while a deque retains the latest ten entries. Search supports vehicle number, owner name, and type. Sorting is performed on a copy of the active list so the underlying parking state is unchanged.

## Data structures

| Structure | Use in ParkSmart |
|---|---|
| `int parking[3][5]` | Required two-dimensional occupancy array (`0` free, `1` occupied). |
| `Vehicle` structure | Vehicle number, owner, type, one-based level and slot, and entry hour. |
| `LEVEL_PREFIXES[3]` | One-dimensional array maps level indexes to the `A`, `B`, and `C` slot prefixes. |
| `vector<Vehicle>` | Active vehicles and sortable copies. |
| `queue<Vehicle>` | FIFO waiting line when the garage is full. |
| `stack<Vehicle>` | Exited vehicles, with the latest exit at `top()`. |
| `priority_queue<PriorityRequest>` | Traffic demonstration, prioritizing emergency, then EV, then regular requests. |
| `map<string, Vehicle>` | Vehicle-number lookup for active vehicles. |
| `set<string>` | Unique types encountered while parking vehicles. |
| `deque<string>` | The ten most recent vehicle entries. |

The waiting-queue interface uses `push`, `pop`, `front`, and `empty`. The exit stack uses `push`, `pop`, `top`, and `empty`. The exit-history screen copies the stack to show its contents without changing the stored history.

## Main algorithms

### Parking allocation

1. Read and validate the number, owner, and type.
2. Reject a number already active or queued.
3. Scan the 2D occupancy array row by row.
4. Mark the first available cell occupied and record its one-based coordinates.
5. If no cell is free, append the vehicle to the waiting queue.

### Vehicle removal

1. Find the active vehicle.
2. Compute elapsed whole hours from its recorded entry hour; the minimum billable period is one hour.
3. Calculate the fee using the vehicle type's hourly rate.
4. Free the matrix cell, remove the vehicle from the active vector and map, and push it onto the exit stack.
5. If the waiting queue is non-empty, dequeue and park its front vehicle.

### Traffic priority

The priority-queue comparator makes the smaller numeric priority rank appear first: `1` emergency, `2` EV, and `3` regular. The traffic simulation is illustrative and does not change live parking records.

## Pricing configuration

The C++ hourly rates are initialized in `hourlyRates` in `main.cpp`; browser app rates are in `RATES` in `app.js`: Car Rs. 20, Bike Rs. 10, EV Rs. 25, and Truck Rs. 40. Change the values in the relevant edition to configure pricing. Checkout rounds elapsed time up to whole hours, with a one-hour minimum charge.

## Input validation and behavior

- Vehicle number and owner name cannot be blank.
- Vehicle numbers already active or waiting are rejected.
- Vehicle types are case-insensitively validated against Car, Bike, EV, and Truck.
- Invalid menu, search, and sort choices are reported without terminating the program.
- Empty waiting queues and exit histories are handled.
- A vehicle not found for removal is reported without changing state.
- Parking coordinates are assigned internally, so users cannot enter an invalid level or slot.
- If the system clock moves backward relative to entry time, removal is cancelled rather than charging a negative duration.
- Invalid numeric input is consumed as text and does not leave the input stream in a failed state.

## Time complexity

Let `n` be the number of active vehicles, `w` the waiting-queue length, `h` the entry-history size, and `x` the retained exit-history size. The garage dimensions are fixed at 3 by 5.

| Operation | Time complexity | Notes |
|---|---:|---|
| Parking search / first-free-slot scan | O(n) in the general layout size | Scans levels × slots; at most 15 checks in this configuration. |
| Vehicle insertion into vector | O(1) average | `push_back`; occasional reallocation may be O(n). |
| Duplicate check including waiting list | O(w) | The queue is copied and scanned to detect duplicate pending numbers. |
| Queue enqueue / dequeue / front | O(1) | STL FIFO queue operations. |
| Stack push / pop / top | O(1) | STL LIFO stack operations. |
| Map search / insertion / erase | O(log n) | `std::map` is an ordered tree. |
| Search by owner or type | O(n) | Checks active vehicle records. |
| Sort active vehicles | O(n log n) | STL `sort` on a copy. |
| Show entry history | O(h) | Prints all entries. |
| Show exit history | O(x) | Prints a copy of the stack. |

## Space complexity

| Storage | Space complexity |
|---|---:|
| Parking matrix | O(levels × slots), fixed at 15 cells |
| Active vector and vehicle map | O(n) |
| Waiting queue | O(w) |
| Exit stack | O(x) |
| Entry vector and recent deque | O(h) + O(1) (deque capped at ten entries) |
| Unique vehicle type set | O(k), where `k` is at most four in this application |
| Total runtime storage | O(n + w + x + h + levels × slots) |

## Sample session

This example opens the parking page, parks a car, checks the free-slot count, and exits. Typed input is shown after each prompt.

```text
+------------------------------------------------------------------+
| PARKSMART                                      SMART PARKING    |
+------------------------------------------------------------------+
  DASHBOARD
  Your parking operations, organized in one place.
  PARKED 0/15    FREE 15    WAITING 0
+------------------------------------------------------------------+

  OVERVIEW
  0 vehicles parked   |   15 slots available   |   0 vehicles waiting

  PAGES
  1  Parking operations       Arrivals, checkout, and live slots
  2  Vehicle directory        Search, browse, and sort vehicles
  3  Activity & history       Waiting line and entry/exit records
  4  Reports & tools          Statistics, traffic, and analysis
  5  Exit ParkSmart
  Open a page: 1

+------------------------------------------------------------------+
| PARKSMART                                      SMART PARKING    |
+------------------------------------------------------------------+
  PARKING OPERATIONS
  Manage arrivals, departures, and the parking floor.
  PARKED 0/15    FREE 15    WAITING 0
+------------------------------------------------------------------+

  1  Park a vehicle
  2  Check out a vehicle
  3  View parking layout
  4  Check available slots
  5  Back to dashboard
  Select an activity: 1

========== PARK VEHICLE ==========
Vehicle Number: TS09AB1234
Owner Name: Sai
Vehicle Type (Car/Bike/EV/Truck): Car

Vehicle parked successfully!

Vehicle Number : TS09AB1234
Level          : 1
Slot           : A1

+------------------------------------------------------------------+
| PARKSMART                                      SMART PARKING    |
+------------------------------------------------------------------+
  PARKING OPERATIONS
  Manage arrivals, departures, and the parking floor.
  PARKED 1/15    FREE 14    WAITING 0
+------------------------------------------------------------------+

  1  Park a vehicle
  2  Check out a vehicle
  3  View parking layout
  4  Check available slots
  5  Back to dashboard
  Select an activity: 4

Level 1 available: 4
Level 2 available: 5
Level 3 available: 5

Total Slots     : 15
Occupied Slots  : 1
Available Slots : 14

+------------------------------------------------------------------+
| PARKSMART                                      SMART PARKING    |
+------------------------------------------------------------------+
  PARKING OPERATIONS
  Manage arrivals, departures, and the parking floor.
  PARKED 1/15    FREE 14    WAITING 0
+------------------------------------------------------------------+

  1  Park a vehicle
  2  Check out a vehicle
  3  View parking layout
  4  Check available slots
  5  Back to dashboard
  Select an activity: 5

+------------------------------------------------------------------+
| PARKSMART                                      SMART PARKING    |
+------------------------------------------------------------------+
  DASHBOARD
  Your parking operations, organized in one place.
  PARKED 1/15    FREE 14    WAITING 0
+------------------------------------------------------------------+

  OVERVIEW
  1 vehicles parked   |   14 slots available   |   0 vehicles waiting

  PAGES
  1  Parking operations       Arrivals, checkout, and live slots
  2  Vehicle directory        Search, browse, and sort vehicles
  3  Activity & history       Waiting line and entry/exit records
  4  Reports & tools          Statistics, traffic, and analysis
  5  Exit ParkSmart
  Open a page: 5

Thank you for using ParkSmart.
```

Removing a vehicle immediately after parking reports zero elapsed hours and bills the one-hour minimum.

## 25 viva questions and answers

1. **What is ParkSmart?**  
   A console-based C++17 simulation for managing parking slots, vehicles, waiting traffic, and histories.
2. **Why is the parking area a 2D array?**  
   Rows represent levels and columns represent slots on each level.
3. **What do 0 and 1 mean in the occupancy matrix?**  
   Zero means free; one means occupied.
4. **Why use a structure for a vehicle?**  
   It groups related attributes of one vehicle into a single record.
5. **Why use one-based level and slot values in `Vehicle`?**  
   They are easier for users to read; array access subtracts one.
6. **How does the application choose a slot?**  
   It scans level by level and takes the first cell marked free.
7. **Which data structure stores active vehicles?**  
   A vector, which supports iteration and sorting.
8. **Why is the waiting line a queue?**  
   FIFO preserves the order in which vehicles began waiting.
9. **What do `front()` and `empty()` do on a queue?**  
   `front()` reads the next vehicle; `empty()` checks whether there are any waiting vehicles.
10. **Why use a stack for exits?**  
    It naturally exposes the most recently exited vehicle first.
11. **What does LIFO mean?**  
    Last In, First Out.
12. **Why use a map?**  
    It associates a vehicle number key with vehicle data for ordered lookup.
13. **What is the complexity of `std::map` lookup?**  
    O(log n), because the ordered map is tree-based.
14. **What does the set store?**  
    Unique vehicle types observed in the current run.
15. **Why use a deque for recent entries?**  
    It supports efficient insertion and removal at both ends.
16. **How is the recent-entry deque bounded?**  
    It drops the oldest item when it grows beyond ten entries.
17. **What does the priority queue demonstrate?**  
    Serving emergency requests before EV requests and regular traffic.
18. **How is priority encoded?**  
    A smaller rank number represents higher priority: 1, 2, then 3.
19. **What is the complexity of sorting?**  
    O(n log n) for the STL `sort` operation.
20. **Why sort a copy of the active vector?**  
    To display another ordering without changing the application's live vehicle ordering.
21. **How are duplicate numbers checked?**  
    The active map and a scan of a copy of the waiting queue are checked.
22. **What happens when a parked vehicle is removed and someone is waiting?**  
    Its slot is freed, then the front waiting vehicle is automatically allocated a slot.
23. **How is the parking fee calculated?**  
    Billable hours times the configured hourly rate for the vehicle type, with a one-hour minimum.
24. **What is the overall storage complexity?**  
    O(n + w + x + h + levels × slots), for active vehicles, waiters, exits, entries, and slots.
25. **What is one limitation of this implementation?**  
    State is kept only in memory and is lost when the application exits.

## One-minute presentation script

Good morning. My project is ParkSmart, a C++17 console-based smart parking and traffic-flow manager. It models three parking levels with five slots each using a two-dimensional array. A vehicle structure stores its number, owner, type, location, and entry time. The program allocates the first free slot, handles duplicate and invalid input, and places arrivals into a FIFO waiting queue when parking is full. When a vehicle exits, its parking fee is calculated and the first waiting vehicle is automatically served. I used a vector for active vehicles, a map for number-based lookup, a stack for exit history, a deque for recent entries, a set for unique types, and a priority queue to demonstrate emergency-first traffic handling. The application also supports searching, sorting, statistics, and complexity analysis. This project shows how core data structures can solve a practical smart-city problem. Thank you.

## Five-minute presentation script

Good morning. My project is ParkSmart, a multi-level parking slot allocator and traffic-flow manager implemented as a C++17 console application.

The objective is to simulate an urban parking facility and demonstrate data structures from the C++ curriculum in a practical system. The parking facility has three levels and five spaces per level. Its occupancy is stored in a two-dimensional integer array: zero means a free slot and one means an occupied slot. The application allocates the first free location in level and slot order.

Vehicle information is grouped in a structure containing the vehicle number, owner name, vehicle type, parking level, slot, and entry time. Before parking, the system checks that required values are present, the type is one of Car, Bike, EV, or Truck, and the number is not already active or waiting.

Several STL containers each serve a clear purpose. A vector stores active vehicles and provides the records used for sorting and statistics. A map links vehicle numbers to vehicle data. A queue handles vehicles waiting for a free slot in first-in, first-out order. When a parked vehicle leaves, the slot is released and the first waiting vehicle is automatically parked. A stack stores exits so the latest departure can be shown first, demonstrating last-in, first-out behavior. A deque keeps the ten most recent entries, and a set stores unique vehicle types.

The traffic simulation includes an entrance queue and a priority queue. The priority queue orders emergency requests first, EV requests second, and regular traffic third. This is useful in a smart-city context because urgent requests can be handled before ordinary traffic. The demonstration is separate from live parking state.

Users can park or remove vehicles, display the layout, search by number, owner, or type, inspect available and occupied spaces, view waiting and entry/exit histories, sort by several fields, view statistics, simulate traffic, and read the complexity analysis. Pricing is configured through hourly rates in the source code. The system uses whole elapsed hours and applies a one-hour minimum fee.

For performance, map lookup is O(log n), sorting is O(n log n), and queue and stack operations are O(1). A slot scan is linear in the number of parking cells, which is only fifteen in this example. The main limitation is that the data is in memory only, so it does not persist after exit. Future improvements could add file or database storage, a graphical interface, sensor integration, dynamic rates, reservations, and configurable EV or accessible spaces.

In conclusion, ParkSmart combines arrays, structures, algorithms, and STL containers to model a realistic parking workflow while remaining small enough to explain and extend. Thank you.

## Future enhancements

- Persist vehicle and history data in files or a database.
- Make the number of levels and spaces configurable.
- Add an interactive pricing configuration screen and payment records.
- Add reservations, accessible spaces, and EV charging-slot constraints.
- Connect entry/exit sensors or a license-plate recognition service.
- Provide a graphical or web dashboard and real-time occupancy updates.
- Model traffic arrival rates and measure queue wait times.

## Project limitations

- This is a single-process simulation; data is not saved after exit.
- Pricing is configured in source code and uses a simplified whole-hour model.
- The traffic and priority-queue simulation is illustrative rather than connected to live arrivals.
- All parking spaces are treated as compatible with every supported vehicle type.
- The facility has a fixed capacity of 15 slots.
- There is no authentication, payment integration, or concurrent multi-terminal access.
