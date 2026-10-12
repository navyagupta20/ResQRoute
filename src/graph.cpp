
#include "graph.h"

#include <queue>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <cmath>
#include <limits>

using namespace std;

Graph::Graph(int numberOfNodes) {
    if (numberOfNodes <= 0) {
        throw invalid_argument(
            "Number of nodes must be positive."
        );
    }

    adjacencyList.resize(numberOfNodes);
}

int Graph::nodeCount() const {
    return static_cast<int>(adjacencyList.size());
}

void Graph::addRoad(
    int source,
    int destination,
    double distanceKm,
    bool bidirectional
) {
    if (source < 0 || destination < 0 ||
        source >= nodeCount() ||
        destination >= nodeCount()) {
        throw out_of_range("Invalid road node ID.");
    }

    if (source == destination ||
        !isfinite(distanceKm) ||
        distanceKm <= 0.0) {
        throw invalid_argument(
            "Road must connect different nodes "
            "and have a positive finite distance."
        );
    }

    // Add the permitted direction.
    adjacencyList[source].push_back({
        destination,
        distanceKm,
        false
    });

    // Add the reverse direction only for two-way roads.
    if (bidirectional) {
        adjacencyList[destination].push_back({
            source,
            distanceKm,
            false
        });
    }
}

bool Graph::blockRoad(
    int source,
    int destination,
    bool bidirectional
) {
    if (source < 0 || destination < 0 ||
        source >= nodeCount() ||
        destination >= nodeCount() ||
        source == destination) {
        return false;
    }

    bool changed = false;

    // Block travel from source to destination.
    for (auto& road : adjacencyList[source]) {
        if (road.destination == destination &&
            !road.isBlocked) {
            road.isBlocked = true;
            changed = true;
        }
    }

    // Optionally block travel in the reverse direction.
    if (bidirectional) {
        for (auto& road : adjacencyList[destination]) {
            if (road.destination == source &&
                !road.isBlocked) {
                road.isBlocked = true;
                changed = true;
            }
        }
    }

    return changed;
}

bool Graph::unblockRoad(
    int source,
    int destination,
    bool bidirectional
) {
    if (source < 0 || destination < 0 ||
        source >= nodeCount() ||
        destination >= nodeCount() ||
        source == destination) {
        return false;
    }

    bool changed = false;

    // Restore travel from source to destination.
    for (auto& road : adjacencyList[source]) {
        if (road.destination == destination &&
            road.isBlocked) {
            road.isBlocked = false;
            changed = true;
        }
    }

    // Optionally restore the reverse direction.
    if (bidirectional) {
        for (auto& road : adjacencyList[destination]) {
            if (road.destination == source &&
                road.isBlocked) {
                road.isBlocked = false;
                changed = true;
            }
        }
    }

    return changed;
}

Graph::RouteResult Graph::findShortestRoute(
    int source,
    int destination
) const {
    RouteResult result;

    if (source < 0 || destination < 0 ||
        source >= nodeCount() ||
        destination >= nodeCount()) {
        return result;
    }

    const int n = nodeCount();

    const double INF =
        numeric_limits<double>::infinity();

    vector<double> distance(n, INF);
    vector<int> parent(n, -1);

    using Node = pair<double, int>;

    priority_queue<
        Node,
        vector<Node>,
        greater<Node>
    > minHeap;

    distance[source] = 0.0;
    minHeap.push({0.0, source});

    while (!minHeap.empty()) {
        double currentDistance = minHeap.top().first;
        int currentNode = minHeap.top().second;

        minHeap.pop();

        // Ignore outdated priority-queue entries.
        if (currentDistance > distance[currentNode]) {
            continue;
        }

        if (currentNode == destination) {
            break;
        }

        for (const Edge& road : adjacencyList[currentNode]) {
            // Closed roads cannot be used.
            if (road.isBlocked) {
                continue;
            }

            double newDistance =
                currentDistance + road.distanceKm;

            if (newDistance < distance[road.destination]) {
                distance[road.destination] = newDistance;
                parent[road.destination] = currentNode;

                minHeap.push({
                    newDistance,
                    road.destination
                });
            }
        }
    }

    // No path exists.
    if (!isfinite(distance[destination])) {
        return result;
    }

    result.found = true;
    result.totalDistanceKm = distance[destination];

    // Reconstruct the route from destination to source.
    int node = destination;

    while (node != -1) {
        result.path.push_back(node);
        node = parent[node];
    }

    reverse(
        result.path.begin(),
        result.path.end()
    );

    return result;
}
