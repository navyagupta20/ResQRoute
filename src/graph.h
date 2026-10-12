
#ifndef RESQROUTE_GRAPH_H
#define RESQROUTE_GRAPH_H

#include <vector>
#include <string>
#include <utility>
#include <limits>

class Graph {
public:
    struct Edge {
        int destination;
        double distanceKm;
        bool isBlocked;
    };

    struct RouteResult {
        bool found = false;
        double totalDistanceKm = 0.0;
        std::vector<int> path;
    };

private:
    std::vector<std::vector<Edge>> adjacencyList;

public:
    explicit Graph(int numberOfNodes);

    int nodeCount() const;

    void addRoad(int source, int destination, double distanceKm,
                 bool bidirectional = true);

    bool blockRoad(int source, int destination,
                   bool bidirectional = true);

    bool unblockRoad(int source, int destination,
                     bool bidirectional = true);

    RouteResult findShortestRoute(int source, int destination) const;
};

#endif
