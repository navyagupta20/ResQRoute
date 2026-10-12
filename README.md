# ResQRoute

**A C++-based emergency route finder using Dijkstra's shortest-path algorithm.**

ResQRoute identifies the shortest available route between selected destinations in Amritsar using OpenStreetMap road-network data. It also supports road closures and route recalculation when a road becomes unavailable.

## Features

- **Shortest-Path Routing:** Finds routes using Dijkstra's algorithm.
- **Priority Queue Optimization:** Uses a min-heap to efficiently select the next node.
- **Road Network Modeling:** Represents locations and road connections using a graph.
- **Road Closure Simulation:** Blocks a road segment and recalculates the route.
- **Route Recalculation:** Finds an alternative path when the original route becomes unavailable.
- **Named Destinations:** Supports selecting destinations such as hospitals, railway stations, and landmarks.
- **Real-World Map Data:** Uses OpenStreetMap road-network data for the Amritsar area.

## Tech Stack

- **Language:** C++
- **Core Concepts:** Graphs, Dijkstra's Algorithm, Priority Queues, Greedy Algorithms
- **Data Structures:** Adjacency Lists, Min-Heap, Vectors
- **Tools:** GCC, Git, GitHub
- **Data Source:** OpenStreetMap

## How It Works

1. Loads map nodes, road connections, and named destinations.
2. Maps selected destinations to nearby nodes in the road network.
3. Runs Dijkstra's algorithm to find the shortest available path.
4. Displays the estimated route distance and path.
5. Allows a road segment to be blocked.
6. Recalculates the route to find an alternative path when possible.

## Algorithm

Dijkstra's algorithm calculates the shortest path in a weighted graph with non-negative edge weights.

A min-heap helps process the closest unvisited node efficiently.

**Time Complexity:** \(O((V + E)\log V)\) using an adjacency list and binary min-heap, where \(V\) is the number of vertices and \(E\) is the number of edges.

**Space Complexity:** \(O(V + E)\) for the graph and auxiliary data structures.

## Project Structure

```text
ResQRoute/
├── src/
│   ├── main.cpp
│   ├── graph.cpp
│   └── graph.h
├── data/
│   ├── nodes.csv
│   ├── roads.csv
│   ├── places.csv
│   └── places.json
├── scripts/
├── tests/
├── .gitignore
└── README.md
```

## Run Locally

### Prerequisites

- A C++ compiler supporting C++17
- Python 3 for optional data-processing scripts

## Current Limitations

- Route distances are estimates, not verified driving distances.
- Live traffic, emergency dispatch, and real-time road closures are not implemented.
- Road-network coverage is limited to the downloaded map area.
- Destination matching depends on the available map data.

## Future Improvements

- Interactive map visualization of the selected route.
- More robust handling of one-way roads and road closures.
- Expanded automated tests for routing edge cases.
- Improved route instructions and destination matching.

## Data Attribution

Road-network data is sourced from [OpenStreetMap](https://www.openstreetmap.org/). OpenStreetMap data is available under the Open Database License (ODbL). See the [OpenStreetMap Copyright and License page](https://www.openstreetmap.org/copyright) for details.

---

**ResQRoute — shortest paths, smarter rerouting.**
