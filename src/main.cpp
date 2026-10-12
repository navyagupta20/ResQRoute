
#include "graph.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include <iomanip>
#include <algorithm>

using namespace std;

const double EARTH_RADIUS_KM = 6371.0;

struct MapNode {
    int index;
    long long osmId;
    double latitude;
    double longitude;
};

struct Place {
    string name;
    double latitude;
    double longitude;
    string type;
    int nearestNode = -1;
    double snapDistanceKm = 0.0;
};

double calculateDistance(double lat1, double lon1,
                         double lat2, double lon2) {
    const double PI = acos(-1.0);

    lat1 *= PI / 180.0;
    lon1 *= PI / 180.0;
    lat2 *= PI / 180.0;
    lon2 *= PI / 180.0;

    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;

    double a = sin(dLat / 2) * sin(dLat / 2)
             + cos(lat1) * cos(lat2)
             * sin(dLon / 2) * sin(dLon / 2);

    a = max(0.0, min(1.0, a));

    return 2.0 * EARTH_RADIUS_KM
           * atan2(sqrt(a), sqrt(1.0 - a));
}

vector<MapNode> loadNodes() {
    vector<MapNode> nodes;
    ifstream file("data/nodes.csv");

    if (!file.is_open()) {
        cerr << "Error: Cannot open data/nodes.csv\n";
        return nodes;
    }

    string line;
    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string indexText, osmText, latText, lonText;

        if (!getline(ss, indexText, ',')) continue;
        if (!getline(ss, osmText, ',')) continue;
        if (!getline(ss, latText, ',')) continue;
        if (!getline(ss, lonText, ',')) continue;

        try {
            MapNode node;
            node.index = stoi(indexText);
            node.osmId = stoll(osmText);
            node.latitude = stod(latText);
            node.longitude = stod(lonText);
            nodes.push_back(node);
        } catch (const exception&) {
            cerr << "Warning: Skipping invalid map node.\n";
        }
    }

    return nodes;
}

vector<Place> loadPlaces() {
    vector<Place> places;
    ifstream file("data/places.csv");

    if (!file.is_open()) {
        cerr << "Error: Cannot open data/places.csv\n";
        return places;
    }

    string line;
    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string name, latText, lonText, type;

        if (!getline(ss, name, ',')) continue;
        if (!getline(ss, latText, ',')) continue;
        if (!getline(ss, lonText, ',')) continue;
        if (!getline(ss, type, ',')) type = "place";

        try {
            Place place;
            place.name = name;
            place.latitude = stod(latText);
            place.longitude = stod(lonText);
            place.type = type;
            places.push_back(place);
        } catch (const exception&) {
            cerr << "Warning: Skipping invalid destination: "
                 << name << '\n';
        }
    }

    return places;
}

bool loadRoads(Graph& graph) {
    ifstream file("data/roads.csv");

    if (!file.is_open()) {
        cerr << "Error: Cannot open data/roads.csv\n";
        return false;
    }

    string line;
    getline(file, line);

    int roadsLoaded = 0;
    int roadsSkipped = 0;

    while (getline(file, line)) {
        stringstream ss(line);
        string sourceText, destinationText, distanceText;
        string highway, roadName, onewayText;

        if (!getline(ss, sourceText, ',') ||
            !getline(ss, destinationText, ',') ||
            !getline(ss, distanceText, ',')) {
            roadsSkipped++;
            continue;
        }

        getline(ss, highway, ',');
        getline(ss, roadName, ',');
        if (!getline(ss, onewayText, ',')) onewayText = "0";

        try {
            int source = stoi(sourceText);
            int destination = stoi(destinationText);
            double distanceKm = stod(distanceText);
            bool isOneWay = (onewayText == "1");

            if (source < 0 || destination < 0 ||
                source >= graph.nodeCount() ||
                destination >= graph.nodeCount() ||
                source == destination ||
                distanceKm <= 0.0 ||
                !isfinite(distanceKm)) {
                roadsSkipped++;
                continue;
            }

            graph.addRoad(
                source, destination, distanceKm, !isOneWay
            );

            roadsLoaded++;
        } catch (const exception&) {
            roadsSkipped++;
        }
    }

    cout << "Road segments loaded: " << roadsLoaded << '\n';

    if (roadsSkipped > 0) {
        cout << "Invalid road records skipped: "
             << roadsSkipped << '\n';
    }

    return roadsLoaded > 0;
}

int nearestNode(const Place& place,
                const vector<MapNode>& nodes,
                double& nearestDistance) {
    int bestNode = -1;
    nearestDistance = numeric_limits<double>::infinity();

    for (const MapNode& node : nodes) {
        double distance = calculateDistance(
            place.latitude, place.longitude,
            node.latitude, node.longitude
        );

        if (distance < nearestDistance) {
            nearestDistance = distance;
            bestNode = node.index;
        }
    }

    return bestNode;
}

void displayPlaces(const vector<Place>& places) {
    cout << "\n========== AVAILABLE DESTINATIONS ==========\n";

    for (size_t i = 0; i < places.size(); i++) {
        cout << setw(3) << i + 1 << ". "
             << places[i].name
             << " [" << places[i].type << "]\n";
    }

    cout << "=============================================\n";
}

int selectPlace(const vector<Place>& places,
                const string& prompt) {
    int choice;

    while (true) {
        cout << prompt;

        if (cin >> choice &&
            choice >= 1 &&
            choice <= static_cast<int>(places.size())) {
            return choice - 1;
        }

        cout << "Invalid choice. Enter a number from 1 to "
             << places.size() << ".\n";

        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

void displayRoute(const Graph::RouteResult& route,
                  const Place& sourcePlace,
                  const Place& destinationPlace) {
    cout << "\n============== ROUTE RESULTS ==============\n";

    if (!route.found) {
        cout << "No route found between these destinations.\n";
        cout << "A road closure may have disconnected the route.\n";
        cout << "===========================================\n";
        return;
    }

    cout << fixed << setprecision(2);
    cout << "Start: " << sourcePlace.name << '\n';
    cout << "Destination: " << destinationPlace.name << '\n';
    cout << "Estimated road-network distance: "
         << route.totalDistanceKm << " km\n";
    cout << "Road-network nodes in route: "
         << route.path.size() << '\n';

    cout << "\nRoute sequence:\n";

    for (size_t i = 0; i < route.path.size(); i++) {
        cout << route.path[i];

        if (i + 1 < route.path.size()) cout << " -> ";
        if ((i + 1) % 12 == 0 && i + 1 < route.path.size()) {
            cout << '\n';
        }
    }

    cout << "\n\nStart snap distance: "
         << sourcePlace.snapDistanceKm << " km\n";
    cout << "Destination snap distance: "
         << destinationPlace.snapDistanceKm << " km\n";
    cout << "\nNote: This prototype does not use live traffic "
            "or emergency dispatch.\n";
    cout << "===========================================\n";
}

int main() {
    cout << "========================================\n";
    cout << "          RESQROUTE ENGINE\n";
    cout << "   Emergency Shortest-Route Finder\n";
    cout << "========================================\n";

    vector<MapNode> nodes = loadNodes();
    vector<Place> places = loadPlaces();

    cout << "\nMap nodes loaded: " << nodes.size() << '\n';
    cout << "Named destinations loaded: " << places.size() << '\n';

    if (nodes.empty() || places.empty()) {
        cerr << "Map nodes or named destinations are missing.\n";
        return 1;
    }

    Graph roadMap(static_cast<int>(nodes.size()));

    if (!loadRoads(roadMap)) {
        cerr << "Road network could not be loaded.\n";
        return 1;
    }

    cout << "\nMatching destinations to the road network...\n";

    for (Place& place : places) {
        place.nearestNode = nearestNode(
            place, nodes, place.snapDistanceKm
        );
    }

    cout << "Destination matching completed.\n";

    // Keep the original selected pair so route recalculation is easy.
    int sourceChoice = -1;
    int destinationChoice = -1;

    while (true) {
        cout << "\n=============== MAIN MENU ===============\n";
        cout << "1. Find shortest route\n";
        cout << "2. Show available destinations\n";
        cout << "3. Block a road using route node IDs\n";
        cout << "4. Unblock a road using route node IDs\n";
        cout << "5. Recalculate last route\n";
        cout << "6. Exit\n";
        cout << "Choose an option: ";

        int option;

        if (!(cin >> option)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Please enter a valid number.\n";
            continue;
        }

        if (option == 6) {
            cout << "\nThank you for using ResQRoute.\n";
            break;
        }

        if (option == 2) {
            displayPlaces(places);
            continue;
        }

        if (option == 1) {
            displayPlaces(places);

            sourceChoice = selectPlace(
                places, "\nEnter the START destination number: "
            );

            destinationChoice = selectPlace(
                places, "Enter the END destination number: "
            );

            if (sourceChoice == destinationChoice) {
                cout << "Start and destination are the same.\n";
                continue;
            }

            if (places[sourceChoice].snapDistanceKm > 2.0 ||
                places[destinationChoice].snapDistanceKm > 2.0) {
                cout << "A destination is too far from the mapped "
                        "road network.\n";
                continue;
            }

            Graph::RouteResult result = roadMap.findShortestRoute(
                places[sourceChoice].nearestNode,
                places[destinationChoice].nearestNode
            );

            displayRoute(
                result, places[sourceChoice], places[destinationChoice]
            );

            continue;
        }

        if (option == 3 || option == 4) {
            int sourceNode, destinationNode;

            cout << "Enter the first road node ID: ";
            cin >> sourceNode;

            cout << "Enter the connected road node ID: ";
            cin >> destinationNode;

            bool changed;

            if (option == 3) {
                changed = roadMap.blockRoad(
                    sourceNode, destinationNode, true
                );

                if (changed) {
                    cout << "Road blocked successfully.\n";
                } else {
                    cout << "Could not block that road. "
                            "Check the node IDs and connection.\n";
                }
            } else {
                changed = roadMap.unblockRoad(
                    sourceNode, destinationNode, true
                );

                if (changed) {
                    cout << "Road unblocked successfully.\n";
                } else {
                    cout << "Could not unblock that road. "
                            "Check the node IDs and connection.\n";
                }
            }

            continue;
        }

        if (option == 5) {
            if (sourceChoice == -1 || destinationChoice == -1) {
                cout << "Find a route first using option 1.\n";
                continue;
            }

            Graph::RouteResult result = roadMap.findShortestRoute(
                places[sourceChoice].nearestNode,
                places[destinationChoice].nearestNode
            );

            cout << "\nRoute recalculated using current road closures.\n";

            displayRoute(
                result, places[sourceChoice], places[destinationChoice]
            );

            continue;
        }

        cout << "Invalid option. Choose a number from 1 to 6.\n";
    }

    return 0;
}
