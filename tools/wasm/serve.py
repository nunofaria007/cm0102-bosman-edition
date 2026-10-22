"""Serve the browser build locally.

Usage:
    python tools/wasm/serve.py [--directory build-wasm] [--page cm0102.html]
                               [--host 127.0.0.1] [--port 8000]
"""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit


class WasmRequestHandler(SimpleHTTPRequestHandler):
    index_page = "/cm0102_recomp.html"

    def do_GET(self):
        if urlsplit(self.path).path == "/":
            self.path = self.index_page
        super().do_GET()


def main():
    parser = argparse.ArgumentParser(description="Serve the CM 01/02 browser build.")
    parser.add_argument("--directory", type=Path, default=Path("build-recomp-wasm"),
                        help="build output directory (default: build-recomp-wasm)")
    parser.add_argument("--page", choices=("cm0102.html", "cm0102_recomp.html"),
                        default="cm0102_recomp.html",
                        help="HTML target to serve (default: cm0102_recomp.html)")
    parser.add_argument("--host", default="127.0.0.1",
                        help="interface to listen on (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8000,
                        help="port to listen on (default: 8000)")
    args = parser.parse_args()

    directory = args.directory.resolve()
    page = directory / args.page
    if not page.is_file():
        parser.error(f"{page} not found; build the selected target first")

    WasmRequestHandler.index_page = f"/{args.page}"
    handler = partial(WasmRequestHandler, directory=str(directory))
    try:
        with ThreadingHTTPServer((args.host, args.port), handler) as server:
            print(f"Serving {page} at http://{args.host}:{args.port}/")
            server.serve_forever()
    except OSError as exc:
        parser.error(f"cannot serve on {args.host}:{args.port}: {exc}")


if __name__ == "__main__":
    main()
