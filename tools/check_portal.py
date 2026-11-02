#!/usr/bin/env python3
"""Check authentication and status on a flashed ESP32; standard library only."""

import argparse
import base64
import getpass
import json
import secrets
import urllib.error
import urllib.parse
import urllib.request


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def request(opener, url, method="GET", headers=None):
    req = urllib.request.Request(url, method=method, headers=headers or {})
    try:
        response = opener.open(req, timeout=15)
    except urllib.error.HTTPError as error:
        response = error
    with response:
        return response.code, response.headers, response.read()


def digest_opener(base, username, password):
    manager = urllib.request.HTTPPasswordMgrWithDefaultRealm()
    manager.add_password(None, base, username, password)
    return urllib.request.build_opener(
        urllib.request.ProxyHandler({}), NoRedirect(),
        urllib.request.HTTPDigestAuthHandler(manager),
    )


def check(base, username, password):
    anonymous = urllib.request.build_opener(
        urllib.request.ProxyHandler({}), NoRedirect(),
    )
    for path in ("/", "/api/status", "/missing"):
        code, headers, _ = request(anonymous, base + path)
        assert code == 401, (path, code)
        assert headers.get("WWW-Authenticate", "").startswith("Digest ")
        assert headers.get("Cache-Control") == "no-store"

    wrong = digest_opener(base, username, secrets.token_hex(32))
    assert request(wrong, base + "/api/status")[0] == 401
    basic = base64.b64encode(f"{username}:{password}".encode()).decode()
    assert request(anonymous, base + "/", headers={"Authorization": "Basic " + basic})[0] == 401

    authenticated = digest_opener(base, username, password)
    code, headers, body = request(authenticated, base + "/")
    assert code == 200, code
    assert "text/html" in headers.get("Content-Type", "")
    assert b"ESP32 Adblock" in body
    code, headers, body = request(authenticated, base + "/api/status")
    assert code == 200, code
    assert headers.get("Cache-Control") == "no-store"
    assert "application/json" in headers.get("Content-Type", "")
    status = json.loads(body)
    for key in ("queries", "blocked", "forwarded", "rejected", "blocklist_entries",
                "uptime_seconds", "free_heap"):
        assert type(status[key]) is int and status[key] >= 0, key
    for key in ("wifi_connected", "dns_enabled", "blocklist_ready"):
        assert type(status[key]) is bool, key
    assert isinstance(status["last_domain"], str)
    assert request(authenticated, base + "/missing")[0] == 404
    assert request(authenticated, base + "/api/status", method="POST")[0] == 404
    print("PASS: login required, wrong credentials rejected, dashboard and status accessible.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("url", help="Device URL, e.g. http://192.168.1.100")
    parser.add_argument("--username", default="admin")
    args = parser.parse_args()
    base = args.url.rstrip("/")
    url = urllib.parse.urlsplit(base)
    if (url.scheme != "http" or not url.hostname or url.username or url.password
            or url.path or url.query or url.fragment):
        parser.error("Use an HTTP device URL with no path or embedded credentials")
    check(base, args.username, getpass.getpass("Portal password: "))


if __name__ == "__main__":
    main()
