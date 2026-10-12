
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
#include <cctype>
#include <stdexcept>

using namespace std;

const double EARTH_RADIUS_KM = 6371.0;
const double MAX_SNAP_DISTANCE_KM = 2.0;

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

double calculateDistance(
    double lat1, double lon1,
    double lat2, double lon2
) {
    const double PI = 3.14159265358979323846;

    double dLat = (lat2 - lat1) * PI / 180.0;
    double dLon = (lon2 - lon1) * PI / 180.0;

    lat1 *= PI / 180.0;
    lat2 *= PI / 180.0;

    double a = sin(dLat / 2.0) * sin(dLat / 2.0)
        + cos(lat1) * cos(lat2)
        * sin(dLon / 2.0) * sin(dLon / 2.0);

    a = max(0.0, min(1.0, a));

    return EARTH_RADIUS_KM * 2.0
        * atan2(sqrt(a), sqrt(1.0 - a));
}

string trim(const string& text) {
    size_t first = text.find_first_not_of(" \t\r\n");

    if (first == string::npos) {
        return "";
    }

    size_t last = text.find_last_not_of(" \t\r\n");

    return text.substr(first, last - first + 1);
}

string toLower(string text) {
    transform(
        text.begin(), text.end(), text.begin(),
        [](unsigned char c) {
            return static_cast<char>(tolower(c));
        }
    );

    return text;
}

vector<MapNode> loadNodes() {
    vector<MapNode> nodes;

    ifstream file("data/nodes.csv");

    if (!file.is_open()) {
        throw runtime_error("Could not open data/nodes.csv");
    }

    string line;
    getline(file, line); // Skip header

    while (getline(file, line)) {
        if (trim(line).empty()) {
            continue;
        }

        stringstream ss(line);

        string indexText;
        string osmIdText;
        string latitudeText;
        string longitudeText;

        if (!getline(ss, indexText, ',') ||
            !getline(ss, osmIdText, ',') ||
            !getline(ss, latitudeText, ',') ||
            !getline(ss, longitudeText, ',')) {
            continue;
        }

        try {
            MapNode node;

            node.index = stoi(trim(indexText));
            node.osmId = stoll(trim(osmIdText));
            node.latitude = stod(trim(latitudeText));
            node.longitude = stod(trim(longitudeText));

            if (node.index < 0 ||
                !isfinite(node.latitude) ||
                !isfinite(node.longitude) ||
                node.latitude < -90.0 ||
                node.latitude > 90.0 ||
                node.longitude < -180.0 ||
                node.longitude > 180.0) {
                continue;
            }

            nodes.push_back(node);
        } catch (const exception&) {
            continue;
        }
    }

    if (nodes.empty()) {
        throw runtime_error("No valid nodes were loaded.");
    }

    return nodes;
}

vector<Place> loadPlaces() {
    vector<Place> places;

    ifstream file("data/places.csv");

    if (!file.is_open()) {
        throw runtime_error("Could not open data/places.csv");
    }

    string line;
    getline(file, line); // Skip header

    while (getline(file, line)) {
        if (trim(line).empty()) {
            continue;
        }

        stringstream ss(line);

        string name;
        string latitudeText;
        string longitudeText;
        string type;

        if (!getline(ss, name, ',') ||
            !getline(ss, latitudeText, ',') ||
            !getline(ss, longitudeText, ',') ||
            !getline(ss, type)) {
            continue;
        }

        try {
            Place place;

            place.name = trim(name);
            place.latitude = stod(trim(latitudeText));
            place.longitude = stod(trim(longitudeText));
            place.type = trim(type);

            if (place.name.empty() ||
                !isfinite(place.latitude) ||
                !isfinite(place.longitude) ||
                place.latitude < -90.0 ||
                place.latitude > 90.0 ||
                place.longitude < -180.0 ||
                place.longitude > 180.0) {
                continue;
            }

            places.push_back(place);
        } catch (const exception&) {
            continue;
        }
    }

    if (places.empty()) {
        throw runtime_error("No valid destinations were loaded.");
    }

    return places;
}

void loadRoads(Graph& roadMap) {
    ifstream file("data/roads.csv");

    if (!file.is_open()) {
        throw runtime_error("Could not open data/roads.csv");
    }

    string line;
    getline(file, line); // Skip header

    int loadedRoads = 0;

    while (getline(file, line)) {
        if (trim(line).empty()) {
            continue;
        }

        stringstream ss(line);

        string sourceText;
        string destinationText;
        string distanceText;
        string highway;
        string roadName;
        string onewayText;

        if (!getline(ss, sourceText, ',') ||
            !getline(ss, destinationText, ',') ||
            !getline(ss, distanceText, ',') ||
            !getline(ss, highway, ',') ||
            !getline(ss, roadName, ',') ||
            !getline(ss, onewayText)) {
            continue;
        }

        try {
            int source = stoi(trim(sourceText));
            int destination = stoi(trim(destinationText));
            double distanceKm = stod(trim(distanceText));

            string oneway = toLower(trim(onewayText));

            if (source < 0 || destination < 0 ||
                source == destination ||
                !isfinite(distanceKm) || distanceKm <= 0.0 ||
                source >= roadMap.nodeCount() ||
                destination >= roadMap.nodeCount()) {
                continue;
            }

            if (oneway == "-1") {
                // Reverse-only road: destination -> source.
                roadMap.addRoad(
                    destination, source, distanceKm, false
                );
            } else if (oneway == "1" ||
                       oneway == "yes" ||
                       oneway == "true") {
                // Forward-only road: source -> destination.
                roadMap.addRoad(
                    source, destination, distanceKm, false
                );
            } else {
                // Roads without a one-way restriction.
                roadMap.addRoad(
                    source, destination, distanceKm, true
                );
            }

            loadedRoads++;
        } catch (const exception&) {
            continue;
        }
    }

    cout << "Road segments loaded: " << loadedRoads << "\n";
}

int nearestNode(
    const Place& place,
    const vector<MapNode>& nodes,
    double& minimumDistance
) {
    int closestIndex = -1;
    minimumDistance = numeric_limits<double>::infinity();

    for (const MapNode& node : nodes) {
        double distance = calculateDistance(
            place.latitude, place.longitude,
            node.latitude, node.longitude
        );

        if (distance < minimumDistance) {
            minimumDistance = distance;
            closestIndex = node.index;
        }
    }

    if (minimumDistance > MAX_SNAP_DISTANCE_KM) {
        return -1;
    }

    return closestIndex;
}

void displayPlaces(const vector<Place>& places) {
    cout << "\n========== AVAILABLE DESTINATIONS ==========\n";

    for (size_t i = 0; i < places.size(); i++) {
        cout << setw(3) << i + 1 << ". "
             << places[i].name
             << " [" << places[i].type << "]";

        if (places[i].nearestNode == -1) {
            cout << " - Outside the supported road network";
        }

        cout << '\n';
    }
}

int readInteger(const string& prompt, int minimum, int maximum) {
    while (true) {
        cout << prompt;

        string input;
        getline(cin, input);

        try {
            size_t position = 0;
            int value = stoi(input, &position);

            if (position == input.size() &&
                value >= minimum &&
                value <= maximum) {
                return value;
            }
        } catch (const exception&) {
            // Show the validation message below.
        }

        cout << "Invalid input. Enter a number from "
             << minimum << " to " << maximum << ".\n";
    }
}

int selectPlace(const vector<Place>& places, const string& prompt) {
    while (true) {
        displayPlaces(places);

        int choice = readInteger(
            prompt, 1, static_cast<int>(places.size())
        );

        int index = choice - 1;

        if (places[index].nearestNode == -1) {
            cout << "This destination is outside the supported "
                 << "road network. Choose another destination.\n";
            continue;
        }

        cout << "Selected: " << places[index].name << '\n';

        return index;
    }
}

void displayRoute(
    const Graph::RouteResult& result,
    const Place& source,
    const Place& destination,
    const vector<MapNode>& nodes
) {
    if (!result.found) {
        cout << "\nNo route was found between "
             << source.name << " and " << destination.name << ".\n";
        return;
    }

    cout << "\n============== ROUTE RESULT ==============\n";
    cout << "Start: " << source.name << '\n';
    cout << "Destination: " << destination.name << '\n';

    cout << fixed << setprecision(2);
    cout << "Total network distance: "
         << result.totalDistanceKm << " km\n";

    cout << "Graph nodes in route: "
         << result.path.size() << '\n';

    cout << "\nRoute node IDs:\n";

    for (size_t i = 0; i < result.path.size(); i++) {
        cout << result.path[i];

        if (i + 1 < result.path.size()) {
            cout << " -> ";
        }

        if ((i + 1) % 12 == 0) {
            cout << '\n';
        }
    }

    cout << "\n==========================================\n";
}

int main() {
    try {
        cout << "==========================================\n";
        cout << "           ResQRoute\n";
        cout << " Emergency Route Planning System\n";
        cout << "==========================================\n";

        cout << "\nLoading road network...\n";

        vector<MapNode> nodes = loadNodes();
        vector<Place> places = loadPlaces();

        int maxNodeIndex = -1;

        for (const MapNode& node : nodes) {
            maxNodeIndex = max(maxNodeIndex, node.index);
        }

        Graph roadMap(maxNodeIndex + 1);

        loadRoads(roadMap);

        cout << "Map nodes loaded: " << nodes.size() << '\n';
        cout << "Matching destinations to the road network...\n";

        for (Place& place : places) {
            place.nearestNode = nearestNode(
                place, nodes, place.snapDistanceKm
            );
        }

        cout << "Destination matching completed.\n";

        Graph::RouteResult lastRoute;
        int lastSourceIndex = -1;
        int lastDestinationIndex = -1;

        while (true) {
            cout << "\n=============== MAIN MENU ===============\n";
            cout << "1. Find shortest route\n";
            cout << "2. Show available destinations\n";
            cout << "3. Block a road\n";
            cout << "4. Unblock a road\n";
            cout << "5. Recalculate last route\n";
            cout << "6. Exit\n";

            int option = readInteger("Choose an option: ", 1, 6);

            if (option == 1) {
                int sourceIndex = selectPlace(
                    places, "\nSelect starting destination number: "
                );

                int destinationIndex = selectPlace(
                    places, "\nSelect destination number: "
                );

                if (sourceIndex == destinationIndex) {
                    cout << "Start and destination must be different.\n";
                    continue;
                }

                lastSourceIndex = sourceIndex;
                lastDestinationIndex = destinationIndex;

                lastRoute = roadMap.findShortestRoute(
                    places[sourceIndex].nearestNode,
                    places[destinationIndex].nearestNode
                );

                displayRoute(
                    lastRoute,
                    places[sourceIndex],
                    places[destinationIndex],
                    nodes
                );
            } else if (option == 2) {
                displayPlaces(places);
            } else if (option == 3 || option == 4) {
                cout << "\nEnter the two connected node IDs "
                     << "for the road.\n";

                int sourceNode = readInteger(
                    "Source node ID: ", 0, roadMap.nodeCount() - 1
                );

                int destinationNode = readInteger(
                    "Destination node ID: ", 0, roadMap.nodeCount() - 1
                );

                if (sourceNode == destinationNode) {
                    cout << "The two node IDs must be different.\n";
                    continue;
                }

                cout << "\nChoose the direction:\n";
                cout << "1. Only " << sourceNode << " -> "
                     << destinationNode << '\n';
                cout << "2. Both directions\n";

                int directionChoice = readInteger(
                    "Your choice: ", 1, 2
                );

                bool bothDirections = (directionChoice == 2);

                bool changed = false;

                if (option == 3) {
                    changed = roadMap.blockRoad(
                        sourceNode,
                        destinationNode,
                        bothDirections
                    );

                    if (changed) {
                        cout << "Road closure applied successfully.\n";
                    } else {
                        cout << "No matching unblocked road was found "
                             << "in the selected direction.\n";
                    }
                } else {
                    changed = roadMap.unblockRoad(
                        sourceNode,
                        destinationNode,
                        bothDirections
                    );

                    if (changed) {
                        cout << "Road reopened successfully.\n";
                    } else {
                        cout << "No matching blocked road was found "
                             << "in the selected direction.\n";
                    }
                }

                if (changed) {
                    cout << "Choose option 5 to recalculate your "
                         << "previous route using the updated network.\n";
                }
            } else if (option == 5) {
                if (lastSourceIndex == -1 ||
                    lastDestinationIndex == -1) {
                    cout << "Find a route first using option 1.\n";
                    continue;
                }

                lastRoute = roadMap.findShortestRoute(
                    places[lastSourceIndex].nearestNode,
                    places[lastDestinationIndex].nearestNode
                );

                cout << "\nRecalculating route with current road "
                     << "closures...\n";

                displayRoute(
                    lastRoute,
                    places[lastSourceIndex],
                    places[lastDestinationIndex],
                    nodes
                );
            } else {
                cout << "\nThank you for using ResQRoute!\n";
                break;
            }
        }
    } catch (const exception& error) {
        cerr << "\nError: " << error.what() << '\n';
        cerr << "Check that data/nodes.csv, data/roads.csv, "
             << "and data/places.csv exist and are valid.\n";
        return 1;
    }

    return 0;
}
