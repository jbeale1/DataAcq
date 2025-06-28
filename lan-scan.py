#!/usr/bin/python

# Scan a local network for devices, resolve their hostnames, and display MAC addresses.
# record historical data of devices seen on the network.
# J.Beale 2025-06-27

import subprocess
import ipaddress
import re
import json
import os, sys
from concurrent.futures import ThreadPoolExecutor, as_completed
import datetime
import socket
import netifaces

SUBNET = "192.168.1.0/24"  # Change this to your local network subnet
TIMEOUT = 1  # seconds
MAX_WORKERS = 64
CACHE_DIR = os.path.expanduser("~/.cache/lan_scan")
CACHE_FILE = os.path.join(CACHE_DIR, "hosts.json")
HISTORY_FILE = os.path.join(CACHE_DIR, "history.json")

# Add known MAC address → label mappings here (uppercase, colon-separated)
manual_labels = {
    "AA:BB:CC:DD:EE:FF": "Sample_Device_1",
    "11:22:33:44:55:66": "Sample_Device_2",
}


def load_history():
    if os.path.exists(HISTORY_FILE):
        with open(HISTORY_FILE, "r") as f:
            raw = json.load(f)
            # Convert lists to sets for internal use
            for mac, entry in raw.items():
                entry["ips"] = set(entry.get("ips", []))
                entry["hostnames"] = set(entry.get("hostnames", []))
            return raw
    return {}

def save_history(history):
    os.makedirs(CACHE_DIR, exist_ok=True)
    with open(HISTORY_FILE, "w") as f:
        json.dump(history, f, indent=2)

def ping_host(ip):
    try:
        subprocess.run(
            ["ping", "-c", "1", "-W", str(TIMEOUT), str(ip)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=True
        )
        return True
    except subprocess.CalledProcessError:
        return False

def resolve_hostname(ip):
    try:
        result = subprocess.run(
            ["avahi-resolve-address", str(ip)],
            capture_output=True,
            text=True,
            timeout=2
        )
        if result.returncode == 0:
            return result.stdout.strip().split()[1]
    except Exception:
        pass
    return None

def get_mac(ip):
    try:
        arp_out = subprocess.check_output(["ip", "neigh", "show", str(ip)], text=True)
        match = re.search(r"lladdr\s+([0-9a-fA-F:]{17})", arp_out)
        if match:
            return match.group(1).upper()
    except subprocess.CalledProcessError:
        pass
    return None

def scan_ip(ip):
    if ping_host(ip):
        hostname = resolve_hostname(ip)
        mac = get_mac(ip)
        label = None

        # prioritize manual labels if available
        if mac and re.match(r"^([0-9A-F]{2}:){5}[0-9A-F]{2}$", mac):
            if mac in manual_labels:
                label = manual_labels[mac]
            elif hostname:
                label = hostname
            else:
                label = mac
        elif hostname:
            label = hostname
        else:
            label = "[no hostname]"

        return str(ip), {"label": label, "mac": mac}
    return None

def load_previous_hosts():
    if os.path.exists(CACHE_FILE):
        with open(CACHE_FILE, "r") as f:
            return json.load(f)
    return {}

def save_current_hosts(hosts):
    os.makedirs(CACHE_DIR, exist_ok=True)
    with open(CACHE_FILE, "w") as f:
        json.dump(hosts, f, indent=2)

def get_primary_local_mac():
    """Get the MAC address of the interface used for the default route."""
    gws = netifaces.gateways()
    default_iface = None
    if 'default' in gws and netifaces.AF_INET in gws['default']:
        default_iface = gws['default'][netifaces.AF_INET][1]
    if default_iface:
        addrs = netifaces.ifaddresses(default_iface)
        mac = addrs.get(netifaces.AF_LINK, [{}])[0].get('addr', None)
        if mac:
            return mac.upper()
    return "N/A"

def get_local_ip():
    """Get the LAN IP address of the machine."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

def get_mac_for_ip(ip):
    """Get the MAC address for the interface with the given IP."""
    for iface in netifaces.interfaces():
        addrs = netifaces.ifaddresses(iface)
        if netifaces.AF_INET in addrs:
            for link in addrs[netifaces.AF_INET]:
                if link.get('addr') == ip:
                    mac = addrs.get(netifaces.AF_LINK, [{}])[0].get('addr', None)
                    if mac:
                        return mac.upper()
    return "N/A"


# =======================================
# Main function to run the LAN scan
# If "--history" argument is provided, it will print the historical device record
# Otherwise, it will perform a scan of the specified subnet and display current devices
def main():

    if "--history" in sys.argv:
        history = load_history()
        print("\n Historical device record:")
        # Set column widths
        mac_w = 17
        ips_w = 14        # suitable for 192.168.1.xxx 
        hostnames_w = 32  # adjust as needed for your hostnames
        last_seen_w = 19

        # Print header
        print(f"{'MAC':<{mac_w}} {'IPs':<{ips_w}} {'Hostnames':<{hostnames_w}} {'Last seen':<{last_seen_w}}")
        print("-" * (mac_w + ips_w + hostnames_w + last_seen_w + 3))

        for mac in sorted(history):
            entry = history[mac]
            ips = ", ".join(entry.get("ips", []))
            hostnames = ", ".join(entry.get("hostnames", []))
            last_seen = entry.get("last_seen", "never")
            print(f"{mac:<{mac_w}} {ips:<{ips_w}} {hostnames:<{hostnames_w}} {last_seen:<{last_seen_w}}")
        return

    if "--erase-history" in sys.argv:
        if os.path.exists(HISTORY_FILE):
            os.remove(HISTORY_FILE)
            print("History file erased.")
        else:
            print("No history file to erase.")
        return
    
    network = ipaddress.IPv4Network(SUBNET, strict=False)
    results = {}

    with ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        futures = {executor.submit(scan_ip, ip): ip for ip in network.hosts()}
        for future in as_completed(futures):
            result = future.result()
            if result:
                ip, data = result
                results[ip] = data

    # Get local IP and MAC before using them
    local_ip = get_local_ip()
    local_mac = get_mac_for_ip(local_ip)
    if not re.match(r"^([0-9A-F]{2}:){5}[0-9A-F]{2}$", str(local_mac)):
        local_mac = get_primary_local_mac()

    # print(f"DEBUG  ::  Local machine: IP {local_ip}  MAC {local_mac}")
    # Add local machine to results if not already present
    if local_ip not in results:
        results[local_ip] = {
            "label": "[this machine]",
            "mac": local_mac
        }

    previous = load_previous_hosts()
    current_ips = set(results.keys())
    previous_ips = set(previous.keys())

    known = unknown = 0
    for ip in sorted(results, key=lambda x: tuple(map(int, x.split('.')))):
        label = results[ip]['label']
        mac = results[ip]['mac']
        # If this is the local machine, use the real MAC
        if ip == local_ip:
            mac_display = local_mac if local_mac else "N/A"
        else:
            mac_display = mac if mac else "N/A"
        print(f"{ip:<15} {mac_display:<17} → {label}")
        if re.match(r"^([0-9A-F]{2}:){5}[0-9A-F]{2}$", label) or label == "[no hostname]":
            unknown += 1
        else:
            known += 1

    total = known + unknown
    print(f"\nTotal hosts: {total} | Known names: {known} | Unknown/MAC-only: {unknown}")

    if new_hosts := current_ips - previous_ips:
        print("\n New hosts since last scan:")
        for ip in sorted(new_hosts, key=lambda x: tuple(map(int, x.split('.')))):
            data = results[ip]
            mac = data['mac'] if data['mac'] else "N/A"
            print(f"  {ip:<15} {mac:<17} → {data['label']}")

    if missing_hosts := previous_ips - current_ips:
        print("\n Hosts not seen this time (previously found):")
        for ip in sorted(missing_hosts, key=lambda x: tuple(map(int, x.split('.')))):
            data = previous[ip]
            if isinstance(data, dict):
                mac = data.get('mac', "N/A")
                label = data.get('label', "N/A")
            else:
                mac = "N/A"
                label = data
            print(f"  {ip:<15} {mac:<17} → {label}")

    # Save current scan results to cache
    save_current_hosts(results)

    # Update persistent MAC/IP/hostname history
    history = load_history()
    now_str = datetime.datetime.now().isoformat(timespec='seconds')
    for ip, data in results.items():
        mac = data.get("mac")
        hostname = data.get("label")
        # Only add real MAC addresses to history
        if mac and re.match(r"^([0-9A-F]{2}:){5}[0-9A-F]{2}$", mac):
            entry = history.setdefault(mac, {"ips": set(), "hostnames": set(), "last_seen": now_str})
            entry["ips"].add(ip)
            if hostname and hostname != "[no hostname]":
                entry["hostnames"].add(hostname)
            entry["last_seen"] = now_str  # Update last seen time

    # Ensure local host is in history if it has a valid MAC
    if re.match(r"^([0-9A-F]{2}:){5}[0-9A-F]{2}$", str(local_mac)):
        entry = history.setdefault(local_mac, {"ips": set(), "hostnames": set(), "last_seen": now_str})
        entry["ips"].add(local_ip)
        entry["hostnames"].add(socket.gethostname())
        entry["last_seen"] = now_str

    # Convert sets to sorted lists before saving as JSON
    for mac, entry in history.items():
        entry["ips"] = sorted(entry["ips"])
        entry["hostnames"] = sorted(entry["hostnames"])

    save_history(history)


    # Show local machine's MAC address
    #print(f"This machine: IP {local_ip}  Hostname {socket.gethostname()}")
    
if __name__ == "__main__":
    main()
