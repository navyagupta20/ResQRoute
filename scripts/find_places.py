
import json
import time
import urllib.request
import urllib.parse
import os

OUTPUT_FILE = "data/places.json"

# Smaller areas to reduce server timeouts.
AREAS = [
    (31.60, 74.80, 31.65, 74.85),
    (31.60, 74.85, 31.65, 74.90),
    (31.60, 74.90, 31.65, 74.95),
    (31.65, 74.80, 31.70, 74.85),
    (31.65, 74.85, 31.70, 74.90),
    (31.65, 74.90, 31.70, 74.95),
    (31.55, 74.80, 31.60, 74.90),
    (31.55, 74.90, 31.60, 74.98),
]

SERVERS = [
    "https://overpass-api.de/api/interpreter",
    "https://overpass.kumi.systems/api/interpreter",
]

# Important destinations for a city routing application.
FEATURES = [
    'node["name"]["amenity"~"hospital|clinic|bus_station"]',
    'way["name"]["amenity"~"hospital|clinic|bus_station"]',
    'node["name"]["railway"~"station|halt"]',
    'way["name"]["railway"~"station|halt"]',
    'node["name"]["tourism"~"attraction|museum"]',
    'way["name"]["tourism"~"attraction|museum"]',
    'node["name"]["place"~"suburb|neighbourhood|locality"]',
    'node["name"]["shop"="mall"]',
    'way["name"]["shop"="mall"]',
    'node["name"]["public_transport"="station"]',
    'way["name"]["public_transport"="station"]',
]


def download_area(bounds):
    south, west, north, east = bounds

    selectors = "\n".join(
        f'{feature}({south},{west},{north},{east});'
        for feature in FEATURES
    )

    query = f"""
    [out:json][timeout:20];
    (
        {selectors}
    );
    out center tags;
    """

    data = urllib.parse.urlencode({"data": query}).encode()

    for server in SERVERS:
        try:
            request = urllib.request.Request(
                server,
                data=data,
                headers={"User-Agent": "ResQRoute/1.0"}
            )

            with urllib.request.urlopen(request, timeout=30) as response:
                result = json.loads(response.read().decode())
                return result.get("elements", [])

        except Exception as error:
            print(f"  Server unavailable: {error}")

    return []


def main():
    os.makedirs("data", exist_ok=True)

    places = []
    seen = set()

    for number, bounds in enumerate(AREAS, start=1):
        print(f"Searching city area {number}/{len(AREAS)}...")

        elements = download_area(bounds)

        for item in elements:
            tags = item.get("tags", {})
            name = tags.get("name", "").strip()

            if not name:
                continue

            latitude = item.get("lat")
            longitude = item.get("lon")

            if "center" in item:
                latitude = item["center"]["lat"]
                longitude = item["center"]["lon"]

            if latitude is None or longitude is None:
                continue

            key = (name.lower(), round(latitude, 5), round(longitude, 5))

            if key in seen:
                continue

            seen.add(key)

            if tags.get("amenity") in ("hospital", "clinic"):
                category = "hospital"
            elif tags.get("railway") in ("station", "halt"):
                category = "railway_station"
            elif tags.get("amenity") == "bus_station":
                category = "bus_station"
            elif tags.get("tourism") in ("attraction", "museum"):
                category = "landmark"
            elif tags.get("shop") == "mall":
                category = "shopping"
            else:
                category = tags.get(
                    "place",
                    tags.get("public_transport", "place")
                )

            places.append({
                "name": name,
                "latitude": latitude,
                "longitude": longitude,
                "type": category,
                "osm_id": item["id"]
            })

        print(f"  Total places collected: {len(places)}")
        time.sleep(1)

    if not places:
        print("\nNo places downloaded. Keep the existing places.json.")
        return

    # Save results only after collecting new data.
    with open(OUTPUT_FILE, "w", encoding="utf-8") as file:
        json.dump(places, file, indent=2, ensure_ascii=False)

    print(f"\nSaved {len(places)} named destinations.")
    print(f"Output: {OUTPUT_FILE}")

    for place in places[:30]:
        print(f"- {place['name']} ({place['type']})")


if __name__ == "__main__":
    main()
