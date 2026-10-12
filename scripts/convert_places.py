
import csv
import json

with open("data/places.json", "r", encoding="utf-8") as file:
    places = json.load(file)

with open("data/places.csv", "w", newline="", encoding="utf-8") as file:
    writer = csv.writer(file)
    writer.writerow(["name", "latitude", "longitude", "type"])

    for place in places:
        name = place["name"].replace(",", " -")

        writer.writerow([
            name,
            place["latitude"],
            place["longitude"],
            place.get("type", "place")
        ])

print(f"Converted {len(places)} destinations to data/places.csv")
