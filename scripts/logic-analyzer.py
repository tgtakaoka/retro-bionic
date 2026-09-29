#!/usr/bin/env python3
"""Logic analyzer interface, and the one implementation we have.

A LogicAnalyzer exports the newest capture as a CSV, its `Channel N`
columns relabeled per a checked-in samples/<target>/bench/channels.toml
set -- nothing board- or architecture-specific, and no live dependency
on any particular analyzer's own preset format at report time.
"""
import csv
import json
import os
import sys
import tomllib
import urllib.request
from abc import ABC, abstractmethod

RUN_DIR = os.environ.get('BIONIC_RUN_DIR', '/tmp/bionic-bench')
os.makedirs(RUN_DIR, exist_ok=True)


def load_channels(path, name):
    """{channel index: label} from channels.toml's [name] table."""
    with open(path, 'rb') as f:
        table = tomllib.load(f).get(name, {})
    return {int(k): v for k, v in table.items()}


def _relabel(path, channels):
    """Rewrite `Channel N` header columns in a captured CSV in place."""
    rows = list(csv.reader(open(path)))
    header = [channels.get(int(c.split()[-1]), c) if c.startswith('Channel ') else c
              for c in rows[0]]
    with open(path, 'w', newline='') as f:
        csv.writer(f).writerows([header] + rows[1:])


class LogicAnalyzer(ABC):
    @abstractmethod
    def export_capture(self, channels_path, set_name, directory=os.path.join(RUN_DIR, 'logic')):
        """Export the newest capture as a CSV, `Channel N` columns
        renamed per channels_path's [set_name] table; returns the CSV path."""

    @abstractmethod
    def preset_channels(self, preset):
        """Channel names from a capture preset, as {channel index: name}."""


class SaleaeLogicPro16(LogicAnalyzer):
    """Talks to the Logic 2 desktop app's MCP server."""

    def __init__(self, mcp_url=None, config_path=None):
        self.mcp_url = mcp_url or os.environ.get(
                'BIONIC_LOGIC_MCP', 'http://127.0.0.1:10530')
        self.config_path = config_path or os.environ.get(
                'BIONIC_LOGIC_CONFIG', os.path.expanduser('~/.config/Logic/config.json'))

    def _mcp(self, name, args):
        req = urllib.request.Request(
                self.mcp_url, method='POST',
                headers={'Content-Type': 'application/json',
                         'Accept': 'application/json, text/event-stream'},
                data=json.dumps({'jsonrpc': '2.0', 'id': 1, 'method': 'tools/call',
                                 'params': {'name': name, 'arguments': args}}).encode())
        with urllib.request.urlopen(req, timeout=120) as r:
            body = r.read().decode()
        for line in body.splitlines():
            if line.startswith('data: '):
                body = line[6:]
        return json.loads(body)

    def export_capture(self, channels_path, set_name, directory=os.path.join(RUN_DIR, 'logic')):
        for cid in range(60, -1, -1):              # newest id wins
            res = self._mcp('export_raw_data_csv',
                            {'captureId': cid, 'directory': directory,
                             'analogDownsampleRatio': 1})
            if 'does not exist' not in json.dumps(res):
                path = os.path.join(directory, 'digital.csv')
                _relabel(path, load_channels(channels_path, set_name))
                print('capture id %d -> %s' % (cid, path))
                return path
        sys.exit('no Logic 2 capture found')

    def preset_channels(self, preset):
        """Channel names from a Logic 2 preset, as {channel index: name}.

        Logic exports a CSV headed "Channel N", never the names, and the MCP has
        no way to load a preset -- but the preset file holds the mapping, so read
        it and label the columns from it.

        Treat the result as a starting point, not as truth: the file records how
        the probes were named, not where they are now. A lead moved on the bench
        leaves the preset stale, and a mislabelled trace is worse than an
        unlabelled one. Confirm against which channels actually carry activity.
        """
        try:
            cfg = json.load(open(self.config_path))
        except (OSError, ValueError):
            return {}
        for p in cfg.get('presets', []):
            if p.get('presetName') != preset:
                continue
            out = {}
            for row in p.get('rowsSettings', []):
                if row.get('type') == 'channel' and row.get('name'):
                    ch = row.get('channel', {}).get('deviceChannel')
                    if ch is not None:
                        out.setdefault(int(ch), row['name'])
            return out
        return {}


def main():
    a = sys.argv[1:]
    if len(a) >= 2:
        print(SaleaeLogicPro16().export_capture(a[0], a[1]))
    else:
        sys.exit('usage: %s CHANNELS.toml SET_NAME' % os.path.basename(sys.argv[0]))


if __name__ == '__main__':
    main()
