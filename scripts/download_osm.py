
import csv
import math
import time
import urllib.request
import urllib.parse
import xml.etree.ElementTree as ET
from pathlib import Path

# Amritsar bounding box
SOUTH = 31.55
WEST = 74.78
NORTH = 31.70
EAST = 74.98


OUTPUT = Path("data")
OUTPUT.mkdir(exist_ok=True)

QUERY = f"""
[out:xml][timeout:90];
way["highway"]({SOUTH},{WEST},{NORTH},{EAST});
out body;
>;
out skel qt;
"""

URL = "https://overpass-api.de/api/interpreter"


def download_map():
    print("Downloading real road data for Amritsar...")

    request = urllib.request.Request(
        URL,
        data=urllib.parse.urlencode({"data": QUERY}).encode(),
        headers={"User-Agent": "ResQRoute/1.0"}
    )

    for attempt in range(3):
        try:
            with urllib.request.urlopen(request, timeout=120) as response:
                return ET.fromstring(response.read())

        except Exception as error:
            print(f"Attempt {attempt + 1} failed: {error}")

            if attempt < 2:
                time.sleep(3)

    raise RuntimeError(
        "Map download failed. Please run the script again later."
    )


def calculate_distance(lat1, lon1, lat2, lon2):
    radius = 6371.0

    lat1 = math.radians(lat1)
    lon1 = math.radians(lon1)
    lat2 = math.radians(lat2)
    lon2 = math.radians(lon2)

    dlat = lat2 - lat1
    dlon = lon2 - lon1

    a = (
        math.sin(dlat / 2) ** 2
        + math.cos(lat1) * math.cos(lat2)
        * math.sin(dlon / 2) ** 2
    )

    return 2 * radius * math.asin(math.sqrt(a))


def main():
    root = download_map()

    nodes = {}
    ways = []

    for element in root:
        if element.tag == "node":
            nodes[element.get("id")] = (
                float(element.get("lat")),
                float(element.get("lon"))
            )

        elif element.tag == "way":
            tags = {
                tag.get("k"): tag.get("v")
                for tag in element.findall("tag")
            }

            highway = tags.get("highway")

            if not highway:
                continue

            # Exclude roads not intended for motor vehicles
            excluded = {
                "footway", "steps", "cycleway", "path",
                "pedestrian", "bridleway", "corridor",
                "construction", "proposed"
            }

            if highway in excluded:
                continue

            node_ids = [
                node.get("ref")
                for node in element.findall("nd")
            ]

            ways.append({
                "nodes": node_ids,
                "highway": highway,
                "name": tags.get("name", "Unnamed road"),
                "oneway": tags.get("oneway", "no")
            })

    # Keep only nodes used by the selected roads
    used_ids = set()

    for way in ways:
        used_ids.update(
            node_id for node_id in way["nodes"]
            if node_id in nodes
        )

    # Assign compact integer IDs for the C++ graph
    ordered_ids = sorted(used_ids, key=int)
    node_index = {
        osm_id: index
        for index, osm_id in enumerate(ordered_ids)
    }

    with open(OUTPUT / "nodes.csv", "w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["index", "osm_id", "latitude", "longitude"])

        for osm_id in ordered_ids:
            lat, lon = nodes[osm_id]
            writer.writerow([node_index[osm_id], osm_id, lat, lon])

    road_count = 0

    with open(OUTPUT / "roads.csv", "w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow([
            "source", "destination", "distance_km",
            "highway", "name", "oneway"
        ])

        for way in ways:
            ids = [
                node_id for node_id in way["nodes"]
                if node_id in node_index
            ]

            pairs = list(zip(ids, ids[1:]))

            if way["oneway"] == "-1":
                pairs = [(b, a) for a, b in reversed(pairs)]

            is_oneway = way["oneway"] in {"yes", "1", "true", "-1"}

            for first, second in pairs:
                lat1, lon1 = nodes[first]
                lat2, lon2 = nodes[second]

                distance = calculate_distance(
                    lat1, lon1, lat2, lon2
                )

                if distance <= 0:
                    continue

                writer.writerow([
                    node_index[first],
                    node_index[second],
                    round(distance, 6),
                    way["highway"],
                    way["name"],
                    int(is_oneway)
                ])

                road_count += 1

    print("\nMap data downloaded successfully!")
    print(f"Locations: {len(ordered_ids)}")
    print(f"Road segments: {road_count}")
    print("Files saved in the data folder:")
    print("  data/nodes.csv")
    print("  data/roads.csv")
    print("\nOpenStreetMap data © OpenStreetMap contributors.")


if __name__ == "__main__":
    main()
