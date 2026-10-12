
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse, parse_qs
import json
import subprocess

ROOT = Path(__file__).resolve().parent
ROUTE_PROGRAM = ROOT / "route_api.exe"


class ResQRouteHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(ROOT), **kwargs)

    def send_json(self, status, data):
        body = json.dumps(data).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        parsed = urlparse(self.path)

        if parsed.path != "/api/route":
            return super().do_GET()

        params = parse_qs(parsed.query)

        try:
            start = int(params.get("start", [""])[0])
            destination = int(params.get("destination", [""])[0])

            if start < 1 or destination < 1:
                raise ValueError("Destination numbers must be positive.")

            if not ROUTE_PROGRAM.exists():
                self.send_json(500, {
                    "error": "route_api.exe not found. Compile it first."
                })
                return

            result = subprocess.run(
                [str(ROUTE_PROGRAM), str(start), str(destination)],
                cwd=str(ROOT),
                capture_output=True,
                text=True,
                timeout=60
            )

            if result.returncode != 0:
                self.send_json(400, {
                    "error": result.stderr.strip() or "Route calculation failed."
                })
                return

            # The C++ program returns one JSON object.
            route = json.loads(result.stdout)
            self.send_json(200, route)

        except (ValueError, IndexError):
            self.send_json(400, {
                "error": "Provide valid start and destination numbers."
            })

        except subprocess.TimeoutExpired:
            self.send_json(504, {
                "error": "Route calculation timed out. Please try again."
            })

        except json.JSONDecodeError:
            self.send_json(500, {
                "error": "The C++ program returned invalid JSON."
            })

        except Exception as error:
            self.send_json(500, {"error": str(error)})


if __name__ == "__main__":
    server = ThreadingHTTPServer(("127.0.0.1", 5500), ResQRouteHandler)
    print("ResQRoute server running at http://127.0.0.1:5500/frontend/")
    print("Keep this terminal open while using the website.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
    finally:
        server.server_close()
