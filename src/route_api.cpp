
#include "graph.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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
};

string trim(const string& text) {
    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

// Supports quoted CSV fields, including road names containing commas.
vector<string> parseCSVLine(const string& line) {
    vector<string> fields;
    string field;
    bool insideQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '"') {
            if (insideQuotes && i + 1 < line.size()
                && line[i + 1] == '"') {
                field += '"';
                ++i;
            } else {
                insideQuotes = !insideQuotes;
            }
        } else if (c == ',' && !insideQuotes) {
            fields.push_back(trim(field));
            field.clear();
        } else {
            field += c;
        }
    }

    fields.push_back(trim(field));
    return fields;
}

string jsonEscape(const string& text) {
    string result;

    for (unsigned char c : text) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 32) {
                    result += ' ';
                } else {
                    result += static_cast<char>(c);
                }
        }
    }

    return result;
}

double calculateDistance(
    double lat1, double lon1,
    double lat2, double lon2
) {
    const double PI = 3.14159265358979323846;

    double dLat = (lat2 - lat1) * PI / 180.0;
    double dLon = (lon2 - lon1) * PI / 180.0;

    lat1 *= PI / 180.0;
    lat2 *= PI / 180.0;

    double a = sin(dLat / 2) * sin(dLat / 2)
        + cos(lat1) * cos(lat2)
        * sin(dLon / 2) * sin(dLon / 2);

    a = max(0.0, min(1.0, a));

    return EARTH_RADIUS_KM * 2.0
        * atan2(sqrt(a), sqrt(1.0 - a));
}

vector<MapNode> loadNodes() {
    ifstream file("data/nodes.csv");
    if (!file) throw runtime_error("Cannot open data/nodes.csv");

    vector<MapNode> nodes;
    string line;
    getline(file, line);

    while (getline(file, line)) {
        if (trim(line).empty()) continue;

        try {
            auto f = parseCSVLine(line);
            if (f.size() < 4) continue;

            MapNode n{
                stoi(f[0]),
                stoll(f[1]),
                stod(f[2]),
                stod(f[3])
            };

            if (n.index >= 0 &&
                isfinite(n.latitude) && isfinite(n.longitude) &&
                n.latitude >= -90 && n.latitude <= 90 &&
                n.longitude >= -180 && n.longitude <= 180) {
                nodes.push_back(n);
            }
        } catch (...) {
            continue;
        }
    }

    if (nodes.empty()) throw runtime_error("No valid map nodes found.");
    return nodes;
}

vector<Place> loadPlaces() {
    ifstream file("data/places.csv");
    if (!file) throw runtime_error("Cannot open data/places.csv");

    vector<Place> places;
    string line;
    getline(file, line);

    while (getline(file, line)) {
        if (trim(line).empty()) continue;

        try {
            auto f = parseCSVLine(line);
            if (f.size() < 4) continue;

            Place p;
            p.name = f[0];
            p.latitude = stod(f[1]);
            p.longitude = stod(f[2]);
            p.type = f[3];

            if (!p.name.empty() &&
                isfinite(p.latitude) && isfinite(p.longitude)) {
                places.push_back(p);
            }
        } catch (...) {
            continue;
        }
    }

    if (places.empty()) throw runtime_error("No valid places found.");
    return places;
}

void loadRoads(Graph& graph) {
    ifstream file("data/roads.csv");
    if (!file) throw runtime_error("Cannot open data/roads.csv");

    string line;
    getline(file, line);

    while (getline(file, line)) {
        if (trim(line).empty()) continue;

        try {
            auto f = parseCSVLine(line);
            if (f.size() < 6) continue;

            int source = stoi(f[0]);
            int destination = stoi(f[1]);
            double distance = stod(f[2]);
            string oneway = f[5];

            transform(oneway.begin(), oneway.end(), oneway.begin(),
                [](unsigned char c) { return static_cast<char>(tolower(c)); });

            if (source < 0 || destination < 0 ||
                source >= graph.nodeCount() ||
                destination >= graph.nodeCount() ||
                source == destination ||
                !isfinite(distance) || distance <= 0) {
                continue;
            }

            if (oneway == "-1") {
                graph.addRoad(destination, source, distance, false);
            } else if (oneway == "1" ||
                       oneway == "yes" ||
                       oneway == "true") {
                graph.addRoad(source, destination, distance, false);
            } else {
                graph.addRoad(source, destination, distance, true);
            }
        } catch (...) {
            continue;
        }
    }
}

int nearestNode(const Place& place, const vector<MapNode>& nodes) {
    int closest = -1;
    double bestDistance = numeric_limits<double>::infinity();

    for (const auto& node : nodes) {
        double distance = calculateDistance(
            place.latitude, place.longitude,
            node.latitude, node.longitude
        );

        if (distance < bestDistance) {
            bestDistance = distance;
            closest = node.index;
        }
    }

    return bestDistance <= MAX_SNAP_DISTANCE_KM ? closest : -1;
}

int main(int argc, char* argv[]) {
    // Place numbers are 1-based, matching the website dropdown.
    if (argc != 3) {
        cerr << "Usage: route_api.exe <start-place-number> "
                "<destination-place-number>\n";
        return 1;
    }

    try {
        int startNumber = stoi(argv[1]);
        int destinationNumber = stoi(argv[2]);

        vector<MapNode> nodes = loadNodes();
        vector<Place> places = loadPlaces();

        if (startNumber < 1 ||
            destinationNumber < 1 ||
            startNumber > static_cast<int>(places.size()) ||
            destinationNumber > static_cast<int>(places.size())) {
            throw runtime_error("Destination number is out of range.");
        }

        // Graph indexes correspond to the CSV node index column.
        int numberOfNodes = 0;
        for (const auto& node : nodes) {
            numberOfNodes = max(numberOfNodes, node.index + 1);
        }

        Graph graph(numberOfNodes);
        loadRoads(graph);

        for (auto& place : places) {
            place.nearestNode = nearestNode(place, nodes);
        }

        const Place& start = places[startNumber - 1];
        const Place& destination = places[destinationNumber - 1];

        if (start.nearestNode < 0 || destination.nearestNode < 0) {
            throw runtime_error(
                "A selected destination is outside the supported road network."
            );
        }

        Graph::RouteResult route = graph.findShortestRoute(
            start.nearestNode,
            destination.nearestNode
        );

        cout << fixed << setprecision(6);
        cout << "{";

        cout << "\"found\":" << (route.found ? "true" : "false") << ",";
        cout << "\"start\":\"" << jsonEscape(start.name) << "\",";
        cout << "\"destination\":\""
             << jsonEscape(destination.name) << "\",";

        cout << "\"distanceKm\":" << route.totalDistanceKm << ",";
        cout << "\"path\":[";

        if (route.found) {
            bool first = true;

            // Build a direct lookup by graph node index.
            vector<const MapNode*> nodeByIndex(numberOfNodes, nullptr);
            for (const auto& node : nodes) {
                nodeByIndex[node.index] = &node;
            }

            for (int nodeId : route.path) {
                if (nodeId < 0 ||
                    nodeId >= static_cast<int>(nodeByIndex.size()) ||
                    nodeByIndex[nodeId] == nullptr) {
                    continue;
                }

                const MapNode& node = *nodeByIndex[nodeId];

                if (!first) cout << ",";
                first = false;

                cout << "{"
                     << "\"nodeId\":" << node.index << ","
                     << "\"latitude\":" << node.latitude << ","
                     << "\"longitude\":" << node.longitude
                     << "}";
            }
        }

        cout << "]}\n";
        return 0;

    } catch (const exception& e) {
        cerr << e.what() << "\n";
        return 1;
    }
}
