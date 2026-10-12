
const map = L.map("map").setView([31.634, 74.862], 13);

L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
    maxZoom: 19,
    attribution:
        '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
}).addTo(map);

const startPoint = document.getElementById("startPoint");
const endPoint = document.getElementById("endPoint");
const findRouteButton = document.getElementById("findRoute");
const message = document.getElementById("message");

const distanceElement = document.getElementById("distance");
const segmentsElement = document.getElementById("segments");
const routeStatusElement = document.getElementById("routeStatus");
const systemStatusElement = document.getElementById("systemStatus");

let places = [];
let currentRoute = null;
let routeMarkers = [];
let placeMarkers = [];

function parseCSV(text) {
    const rows = [];
    let row = [];
    let value = "";
    let insideQuotes = false;

    for (let i = 0; i < text.length; i++) {
        const char = text[i];

        if (char === '"') {
            if (insideQuotes && text[i + 1] === '"') {
                value += '"';
                i++;
            } else {
                insideQuotes = !insideQuotes;
            }
        } else if (char === "," && !insideQuotes) {
            row.push(value);
            value = "";
        } else if (
            (char === "\n" || char === "\r") &&
            !insideQuotes
        ) {
            if (char === "\r" && text[i + 1] === "\n") {
                i++;
            }

            row.push(value);

            if (row.some(cell => cell.trim() !== "")) {
                rows.push(row);
            }

            row = [];
            value = "";
        } else {
            value += char;
        }
    }

    if (value.length > 0 || row.length > 0) {
        row.push(value);
        rows.push(row);
    }

    if (rows.length < 2) return [];

    const headers = rows[0].map(header => header.trim());

    return rows.slice(1).map(cells => {
        const item = {};

        headers.forEach((header, index) => {
            item[header] = (cells[index] || "").trim();
        });

        return item;
    });
}

function escapeHTML(value) {
    return String(value).replace(/[&<>"']/g, character => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#039;"
    })[character]);
}

function setMessage(text) {
    message.textContent = text;
}

function clearRoute() {
    if (currentRoute) {
        map.removeLayer(currentRoute);
        currentRoute = null;
    }

    routeMarkers.forEach(marker => map.removeLayer(marker));
    routeMarkers = [];

    distanceElement.innerHTML = '&mdash; <small>km</small>';
    segmentsElement.textContent = "—";
    routeStatusElement.textContent = "Waiting for route";
}

function addPlaceMarker(place) {
    const marker = L.circleMarker(
        [place.latitude, place.longitude],
        {
            radius: 5,
            color: "#ffffff",
            weight: 1.5,
            fillColor: "#8f2945",
            fillOpacity: 0.9
        }
    ).addTo(map);

    marker.bindPopup(`
        <strong>${escapeHTML(place.name)}</strong><br>
        <span>${escapeHTML(place.type || "Destination")}</span>
    `);

    placeMarkers.push(marker);
}

function populateDestinations() {
    places.forEach((place, index) => {
        const label = `${place.name} (${place.type || "destination"})`;

        // The C++ API uses destination numbers starting at 1.
        const destinationNumber = String(index + 1);

        startPoint.add(new Option(label, destinationNumber));
        endPoint.add(new Option(label, destinationNumber));
    });
}

function getSelectedPlace(selectElement) {
    const number = Number(selectElement.value);

    if (selectElement.value === "" || !Number.isInteger(number)) {
        return null;
    }

    return places[number - 1] || null;
}

async function loadPlaces() {
    try {
        systemStatusElement.textContent = "Loading data";
        setMessage("Loading destinations...");

        const response = await fetch("/data/places.csv");

        if (!response.ok) {
            throw new Error("Could not load data/places.csv.");
        }

        const rows = parseCSV(await response.text());

        places = rows
            .map(row => ({
                name: row.name,
                latitude: Number(row.latitude),
                longitude: Number(row.longitude),
                type: row.type || "destination"
            }))
            .filter(place =>
                place.name &&
                Number.isFinite(place.latitude) &&
                Number.isFinite(place.longitude)
            );

        if (places.length === 0) {
            throw new Error("No valid destinations were found.");
        }

        populateDestinations();
        places.forEach(addPlaceMarker);

        const markerBounds = L.latLngBounds(
            places.map(place => [place.latitude, place.longitude])
        );

        if (markerBounds.isValid()) {
            map.fitBounds(markerBounds, { padding: [30, 30] });
        }

        systemStatusElement.textContent = `${places.length} destinations`;
        setMessage(`Loaded ${places.length} destinations. Choose your route.`);
    } catch (error) {
        console.error(error);
        systemStatusElement.textContent = "Data loading failed";
        setMessage(error.message);
    }
}

async function findRoute() {
    const start = getSelectedPlace(startPoint);
    const end = getSelectedPlace(endPoint);

    if (!start || !end) {
        setMessage("Please choose both starting and ending destinations.");
        return;
    }

    if (startPoint.value === endPoint.value) {
        setMessage("Choose two different destinations.");
        return;
    }

    clearRoute();
    findRouteButton.disabled = true;
    setMessage("Calculating the shortest road route with C++...");
    routeStatusElement.textContent = "Calculating";

    try {
        const params = new URLSearchParams({
            start: startPoint.value,
            destination: endPoint.value
        });

        const response = await fetch(`/api/route?${params.toString()}`);
        const result = await response.json();

        if (!response.ok) {
            throw new Error(result.error || "Route calculation failed.");
        }

        if (!result.found || !Array.isArray(result.path) ||
            result.path.length === 0) {
            throw new Error("No road route was found between these destinations.");
        }

        // Leaflet expects [latitude, longitude] pairs.
        const coordinates = result.path.map(node => [
            Number(node.latitude),
            Number(node.longitude)
        ]);

        currentRoute = L.polyline(coordinates, {
            color: "#8f2945",
            weight: 6,
            opacity: 0.95,
            lineCap: "round",
            lineJoin: "round"
        }).addTo(map);

        const startMarker = L.circleMarker(coordinates[0], {
            radius: 8,
            color: "#ffffff",
            weight: 2,
            fillColor: "#18865b",
            fillOpacity: 1
        }).addTo(map);

        const endMarker = L.circleMarker(
            coordinates[coordinates.length - 1],
            {
                radius: 8,
                color: "#ffffff",
                weight: 2,
                fillColor: "#8f2945",
                fillOpacity: 1
            }
        ).addTo(map);

        routeMarkers = [startMarker, endMarker];

        startMarker.bindPopup(`<strong>Start:</strong> ${escapeHTML(result.start)}`);
        endMarker.bindPopup(`<strong>Destination:</strong> ${escapeHTML(result.destination)}`);

        map.fitBounds(currentRoute.getBounds(), {
            padding: [45, 45]
        });

        distanceElement.innerHTML =
            `${Number(result.distanceKm).toFixed(2)} <small>km</small>`;

        segmentsElement.textContent = String(coordinates.length);
        routeStatusElement.textContent = "Shortest road route";
        setMessage(
            `Route calculated: ${result.start} to ${result.destination}.`
        );
    } catch (error) {
        console.error(error);
        clearRoute();
        routeStatusElement.textContent = "Route unavailable";
        setMessage(error.message);
    } finally {
        findRouteButton.disabled = false;
    }
}

findRouteButton.addEventListener("click", findRoute);

loadPlaces();
