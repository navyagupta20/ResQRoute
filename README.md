# ResQRoute

A C++-based emergency route finder using Dijkstra's shortest-path algorithm.

ResQRoute identifies the shortest available route between selected destinations in Amritsar using OpenStreetMap road-network data. It also supports simulated road closures and route recalculation.

## Features

- **Shortest-Path Routing:** Finds routes using Dijkstra's algorithm.
- **Priority Queue Optimization:** Uses a min-heap to efficiently select the next node.
- **Road Network Modeling:** Represents locations and road connections using a graph.
- **Road Closure Simulation:** Blocks a road segment and recalculates the route.
- **Route Recalculation:** Finds an alternative path when the original route becomes unavailable.
- **Named Destinations:** Supports destinations such as hospitals, railway stations, and landmarks.
- **Real-World Map Data:** Uses OpenStreetMap road-network data for the Amritsar area.

## Tech Stack

- **Language:** C++
- **Algorithms:** Dijkstra's Shortest-Path Algorithm
- **Data Structures:** Graphs, Adjacency Lists, Min-Heap, Vectors
- **Core Concepts:** Graph Traversal, Greedy Algorithms, Shortest-Path Optimization
- **Tools:** GCC, Git, GitHub
- **Data Source:** OpenStreetMap

## How It Works

1. Loads map nodes, road connections, and named destinations.
2. Maps selected destinations to nearby nodes in the road network.
3. Runs Dijkstra's algorithm to find the shortest available path.
4. Displays the estimated route distance and path.
5. Allows a road segment to be blocked.
6. Recalculates the route to find an alternative path when possible.

## Algorithm Complexity

Dijkstra's algorithm uses an adjacency list and a min-heap to efficiently find the shortest path in a weighted graph with non-negative edge weights.

- **Time Complexity:** \(O((V + E)\log V)\)
- **Space Complexity:** \(O(V + E)\)

Where:
- \(V\) represents the number of vertices (nodes).
- \(E\) represents the number of edges (road connections).

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
│   ├── add_landmarks.py
│   ├── convert_places.py
│   ├── download_osm.py
│   └── find_places.py
├── tests/
│   └── test_graph.cpp
├── .gitignore
└── README.md
```

## Run Locally

### Prerequisites

- A C++ compiler supporting C++17
- Python 3 for optional data-processing scripts

## Data Attribution

Road-network data is sourced from [OpenStreetMap](https://www.openstreetmap.org/), which is available under the Open Database License (ODbL). See the [OpenStreetMap Copyright and License page](https://www.openstreetmap.org/copyright) for details.

---

**ResQRoute — shortest paths, smarter rerouting.**