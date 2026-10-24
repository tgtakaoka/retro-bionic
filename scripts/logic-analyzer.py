#!/usr/bin/env python3
"""Logic analyzer interface, and the one implementation we have.

A LogicAnalyzer exports the capture it left open as a CSV, its `Channel N`
columns relabeled per a checked-in samples/<target>/bench/channels.toml
set -- nothing board- or architecture-specific, and no live dependency
on any particular analyzer's own preset format at report time.

    logic-analyzer.py CHANNELS.toml SET_NAME

exports the capture the last start_capture() left open that way and
prints the CSV's path. A capture keeps at most MAX_WINDOW seconds, and
one that never triggers is stopped and never exported: an untriggered
capture's export once filled the disk.
Environment: BIONIC_RUN_DIR (default /tmp/bionic-bench) for exported
files, BIONIC_LOGIC_MCP (default http://127.0.0.1:10530) for Logic 2's
MCP server, BIONIC_LOGIC_CONFIG for its config file holding presets.
"""
import csv
import json
import os
import re
import shutil
import sys
import tomllib
import urllib.request
from abc import ABC, abstractmethod
from importlib.machinery import SourceFileLoader

RUN_DIR = os.environ.get('BIONIC_RUN_DIR', '/tmp/bionic-bench')
os.makedirs(RUN_DIR, exist_ok=True)
# The longest capture, before plus after the trigger, in seconds.
MAX_WINDOW = 10e-3
# How long finish_capture() waits for the trigger, in seconds.
TRIGGER_WAIT = 60
bench = SourceFileLoader('bench', os.path.join(os.path.dirname(
        os.path.abspath(__file__)), 'bionic-bench.py')).load_module()


def analyzer():
    """(name, attributes) of the logic analyzer to use: BIONIC_LOGIC names
    one, else the one this session holds, else the only one. Each is a
    resource added with `bionic-bench.py add NAME kind=logic mcp=URL`; exits if
    another session holds it."""
    found = bench.resources(kind='logic')
    if not found:
        return None, {}
    name = bench.pick(list(found), 'BIONIC_LOGIC')
    if name is None:
        sys.exit('more than one logic analyzer: lock one '
                 '(`scripts/bionic-bench.py lock NAME`) or set BIONIC_LOGIC')
    bench.check(name)
    return name, found.get(name, {})


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
        """Export the capture left open as a CSV, `Channel N` columns
        renamed per channels_path's [set_name] table; returns the CSV path."""

    @abstractmethod
    def preset_channels(self, preset):
        """Channel names from a capture preset, as {channel index: name}."""

    @abstractmethod
    def start_capture(self, channels, trigger, edge='rise', before=20e-6,
                      after=200e-6, glitch=None):
        """Arm a capture of |channels| that triggers on |trigger|'s |edge|
        ('rise' or 'fall') and keeps |before|/|after| seconds around it,
        ignoring trigger pulses shorter than |glitch| seconds, after closing
        the capture the last one left open. Returns a handle for
        finish_capture()."""

    @abstractmethod
    def finish_capture(self, handle, directory):
        """Wait for the capture and export its channels as a CSV with
        `Channel N` columns; returns the CSV path. A capture that never
        triggers is stopped instead and raises RuntimeError. The capture
        stays open for a look until the next start_capture()."""


class SaleaeLogicPro16(LogicAnalyzer):
    """Talks to the Logic 2 desktop app's MCP server."""

    def __init__(self, mcp_url=None, config_path=None):
        self.mcp_url = mcp_url or os.environ.get('BIONIC_LOGIC_MCP')
        if not self.mcp_url:
            self.mcp_url = analyzer()[1].get('mcp', 'http://127.0.0.1:10530')
        self.config_path = config_path or os.environ.get(
                'BIONIC_LOGIC_CONFIG', os.path.expanduser('~/.config/Logic/config.json'))

    def _mcp(self, name, args, timeout=120):
        req = urllib.request.Request(
                self.mcp_url, method='POST',
                headers={'Content-Type': 'application/json',
                         'Accept': 'application/json, text/event-stream'},
                data=json.dumps({'jsonrpc': '2.0', 'id': 1, 'method': 'tools/call',
                                 'params': {'name': name, 'arguments': args}}).encode())
        with urllib.request.urlopen(req, timeout=timeout) as r:
            body = r.read().decode()
        for line in body.splitlines():
            if line.startswith('data: '):
                body = line[6:]
        return json.loads(body)

    def export_capture(self, channels_path, set_name, directory=os.path.join(RUN_DIR, 'logic')):
        # Never probe ids: a wrong one may be someone's long session.
        opened = self._open()
        if opened is None:
            sys.exit('no capture left open by start_capture')
        path = self._export(opened['id'], opened['channels'], directory)
        _relabel(path, load_channels(channels_path, set_name))
        print('capture id %d -> %s' % (opened['id'], path))
        return path

    def _export(self, handle, channels, directory):
        shutil.rmtree(directory, ignore_errors=True)
        self._call('export_raw_data_csv', {
            'captureId': handle, 'directory': directory, 'analogDownsampleRatio': 1,
            'logicChannels': {'digitalChannels': channels}})
        return os.path.join(directory, 'digital.csv')

    def _call(self, name, args, timeout=120):
        try:
            res = self._mcp(name, args, timeout)
        except (OSError, ValueError) as e:
            # Logic 2 away, a timeout or a garbled reply: callers handle
            # RuntimeError, so a recording goes on without its capture.
            raise RuntimeError('%s: %s' % (name, e)) from e
        text = json.dumps(res)
        if '"isError": true' in text:
            raise RuntimeError('%s: %s' % (name, text[:300]))
        return text

    # The capture left open, {"id": n, "channels": [...]}, so the next
    # start can close it and an export knows what to take.
    OPEN_ID = os.path.join(RUN_DIR, 'logic-open-capture.json')

    def _open(self):
        try:
            return json.load(open(self.OPEN_ID))
        except (OSError, ValueError):
            return None

    def _close_open(self):
        opened = self._open()
        if opened is None:
            return
        try:
            self._call('close_capture', {'captureId': opened['id']})
        except RuntimeError:
            pass                        # closed by hand in the app
        os.remove(self.OPEN_ID)

    def start_capture(self, channels, trigger, edge='rise', before=20e-6,
                      after=200e-6, glitch=None):
        analyzer()
        if before + after > MAX_WINDOW:
            raise RuntimeError('start_capture: %g s window over the %g s limit'
                               % (before + after, MAX_WINDOW))
        self._close_open()
        # The 3.3V threshold: the default one reads ringing as extra pulses.
        device = {'digitalSampleRate': 100000000, 'digitalThresholdVolts': 3.3,
                  'logicChannels': {'digitalChannels': sorted(set(channels) | {trigger})}}
        if glitch:
            device['glitchFilters'] = [{'channelIndex': trigger, 'pulseWidthSeconds': glitch}]
        text = self._call('start_capture', {
            'captureConfiguration': {'digitalCaptureMode': {
                'triggerChannelIndex': trigger,
                'triggerType': {'rise': 1, 'fall': 2}[edge],
                'afterTriggerSeconds': after,
                'trimDataSeconds': before + after}},
            'logicDeviceConfiguration': device})
        m = re.search(r'captureId\\*"?\s*:\s*(\d+)', text)
        if not m:
            raise RuntimeError('start_capture: no capture id in %s' % text[:300])
        handle = int(m.group(1))
        with open(self.OPEN_ID, 'w') as f:
            json.dump({'id': handle,
                       'channels': device['logicChannels']['digitalChannels']}, f)
        return handle

    def finish_capture(self, handle, directory):
        try:
            self._call('wait_capture', {'captureId': handle}, timeout=TRIGGER_WAIT)
        except RuntimeError:
            # Untriggered, it would grow until stopped: stop, never export.
            try:
                self._call('stop_capture', {'captureId': handle})
            except RuntimeError:
                pass
            raise
        opened = self._open()
        channels = opened['channels'] if opened and opened['id'] == handle else None
        if channels is None:
            raise RuntimeError('finish_capture: capture %d not the one left open' % handle)
        return self._export(handle, channels, directory)

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
