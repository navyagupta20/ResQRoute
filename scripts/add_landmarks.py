
import json
import os

OUTPUT_FILE = "data/places.json"

# Coordinates are approximate landmark locations in Amritsar.
LANDMARKS = [
    {
        "name": "Golden Temple",
        "latitude": 31.6200,
        "longitude": 74.8765,
        "type": "landmark"
    },
    {
        "name": "Jallianwala Bagh",
        "latitude": 31.6206,
        "longitude": 74.8800,
        "type": "landmark"
    },
    {
        "name": "Durgiana Temple",
        "latitude": 31.6340,
        "longitude": 74.8723,
        "type": "landmark"
    },
    {
        "name": "Amritsar Junction Railway Station",
        "latitude": 31.6330,
        "longitude": 74.8656,
        "type": "railway_station"
    },
    {
        "name": "Amritsar ISBT Bus Stand",
        "latitude": 31.6350,
        "longitude": 74.8820,
        "type": "bus_station"
    },
    {
        "name": "Hall Bazaar",
        "latitude": 31.6260,
        "longitude": 74.8770,
        "type": "shopping"
    },
    {
        "name": "Partition Museum",
        "latitude": 31.6260,
        "longitude": 74.8790,
        "type": "landmark"
    },
    {
        "name": "Gobindgarh Fort",
        "latitude": 31.6300,
        "longitude": 74.8720,
        "type": "landmark"
    },
    {
        "name": "Guru Nanak Dev University",
        "latitude": 31.6340,
        "longitude": 74.8230,
        "type": "university"
    },
    {
        "name": "Sri Guru Ram Dass Jee International Airport",
        "latitude": 31.7096,
        "longitude": 74.7973,
        "type": "airport"
    }
]


def main():
    os.makedirs("data", exist_ok=True)

    places = []

    if os.path.exists(OUTPUT_FILE):
        with open(OUTPUT_FILE, "r", encoding="utf-8") as file:
            places = json.load(file)

    existing = {
        place["name"].strip().lower()
        for place in places
    }

    added = 0

    for landmark in LANDMARKS:
        if landmark["name"].lower() not in existing:
            landmark["osm_id"] = None
            places.append(landmark)
            added += 1

    with open(OUTPUT_FILE, "w", encoding="utf-8") as file:
        json.dump(places, file, indent=2, ensure_ascii=False)

    print(f"Added {added} city landmarks.")
    print(f"Total named destinations: {len(places)}")
    print(f"Saved to {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
