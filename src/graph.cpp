
#include "graph.h"
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <cmath>

using namespace std;

Graph::Graph(int numberOfNodes) {
    if (numberOfNodes <= 0) {
        throw invalid_argument("Number of nodes must be positive.");
    }

    adjacencyList.resize(numberOfNodes);
}

int Graph::nodeCount() const {
    return (int)adjacencyList.size();
}

void Graph::addRoad(int source, int destination, double distanceKm,
                    bool bidirectional) {
    if (source < 0 || destination < 0 ||
        source >= nodeCount() || destination >= nodeCount()) {
        throw out_of_range("Invalid location.");
    }

    if (source == destination || distanceKm <= 0 ||
        !isfinite(distanceKm)) {
        throw invalid_argument("Invalid road distance.");
    }

    adjacencyList[source].push_back({destination, distanceKm, false});

    if (bidirectional) {
        adjacencyList[destination].push_back({source, distanceKm, false});
    }
}

bool Graph::blockRoad(int source, int destination, bool bidirectional) {
    if (source < 0 || destination < 0 ||
        source >= nodeCount() || destination >= nodeCount()) {
        return false;
    }

    bool changed = false;

    for (auto& road : adjacencyList[source]) {
        if (road.destination == destination && !road.isBlocked) {
            road.isBlocked = true;
            changed = true;
        }
    }

    if (bidirectional) {
        for (auto& road : adjacencyList[destination]) {
            if (road.destination == source && !road.isBlocked) {
                road.isBlocked = true;
                changed = true;
            }
        }
    }

    return changed;
}

bool Graph::unblockRoad(int source, int destination, bool bidirectional) {
    if (source < 0 || destination < 0 ||
        source >= nodeCount() || destination >= nodeCount()) {
        return false;
    }

    bool changed = false;

    for (auto& road : adjacencyList[source]) {
        if (road.destination == destination && road.isBlocked) {
            road.isBlocked = false;
            changed = true;
        }
    }

    if (bidirectional) {
        for (auto& road : adjacencyList[destination]) {
            if (road.destination == source && road.isBlocked) {
                road.isBlocked = false;
                changed = true;
            }
        }
    }

    return changed;
}

Graph::RouteResult Graph::findShortestRoute(
    int source, int destination) const {

    RouteResult result;

    if (source < 0 || destination < 0 ||
        source >= nodeCount() || destination >= nodeCount()) {
        return result;
    }

    int n = nodeCount();
    double INF = numeric_limits<double>::infinity();

    vector<double> distance(n, INF);
    vector<int> parent(n, -1);

    typedef pair<double, int> Node;

    priority_queue<Node, vector<Node>, greater<Node>> minHeap;

    distance[source] = 0;
    minHeap.push({0, source});

    while (!minHeap.empty()) {
        double currentDistance = minHeap.top().first;
        int currentNode = minHeap.top().second;
        minHeap.pop();

        if (currentDistance > distance[currentNode]) {
            continue;
        }

        if (currentNode == destination) {
            break;
        }

        for (const Edge& road : adjacencyList[currentNode]) {
            if (road.isBlocked) {
                continue;
            }

            double newDistance = currentDistance + road.distanceKm;

            if (newDistance < distance[road.destination]) {
                distance[road.destination] = newDistance;
                parent[road.destination] = currentNode;

                minHeap.push({newDistance, road.destination});
            }
        }
    }

    if (distance[destination] == INF) {
        return result;
    }

    result.found = true;
    result.totalDistanceKm = distance[destination];

    int node = destination;

    while (node != -1) {
        result.path.push_back(node);
        node = parent[node];
    }

    reverse(result.path.begin(), result.path.end());

    return result;
}
