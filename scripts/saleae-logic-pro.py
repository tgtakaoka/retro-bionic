#!/usr/bin/env python3
"""Saleae Logic Pro / Logic 2 specific tooling -- not part of the generic
LogicAnalyzer interface in logic-analyzer.py.

  load-preset PRESET [SET_NAME]   dump a Logic 2 preset's channel labels
                                   as a TOML table named SET_NAME (default
                                   PRESET), to redirect or append into
                                   samples/<target>/bench/channels.toml;
                                   fill in each channel's empty pin
  load-sal SAL [SET_NAME]         the same from a saved capture, with its
                                   trigger and Simple Parallel decoders
  name-sal SAL CHANNELS.toml SET_NAME OUT
                                   copy SAL to OUT with SET_NAME's channel
                                   names, and its Simple Parallel decoders
                                   if SAL has none
  arm CHANNELS.toml SET_NAME       start a capture of SET_NAME's channels
                                   on its trigger, closing the last one
  save CHANNELS.toml SET_NAME OUT  wait for it, add SET_NAME's Simple
                                   Parallel decoders, save it as OUT, name
                                   its channels and open OUT in Logic 2

The preset is read from Logic 2's config file, $BIONIC_LOGIC_CONFIG
(default ~/.config/Logic/config.json).
"""
import json
import os
import sys
import tomllib
import zipfile
from importlib.machinery import SourceFileLoader

HERE = os.path.dirname(os.path.abspath(__file__))
la = SourceFileLoader('la', os.path.join(HERE, 'logic-analyzer.py')).load_module()


def _toml_string(s):
    return '"%s"' % s.replace('\\', '\\\\').replace('"', '\\"')


def load_preset_toml(preset, set_name=None):
    """A Logic 2 preset's channel labels, as a TOML table."""
    channels = la.SaleaeLogicPro16().preset_channels(preset)
    lines = ['[%s]' % (set_name or preset)]
    for ch, name in sorted(channels.items()):
        lines.append('%2d = { pin = "", signal = %s }' % (ch, _toml_string(name)))
    return '\n'.join(lines) + '\n'


EDGES = ['rise', 'fall', 'dual']    # Simple Parallel's Clock State values
ROW_HEIGHT = 50                     # a digital row's height, in pixels


def _sal_meta(path):
    with zipfile.ZipFile(path) as z:
        return json.loads(z.read('meta.json'))


def _digital(row):
    ch = row.get('channel', {})
    return row.get('type') == 'channel' and ch.get('type') == 'Digital'


def load_sal_toml(path, set_name):
    """A saved capture's channel names, trigger and Simple Parallel
    decoders, as a TOML table; pins are left for the leads to fill in."""
    data = _sal_meta(path)['data']
    names = {r['channel']['deviceChannel']: r['name']
             for r in data.get('rowsSettings', []) if _digital(r) and r.get('name')}
    lines = ['[%s]' % set_name]
    for ch, name in sorted(names.items()):
        lines.append('%2d = { pin = "", signal = %s }' % (ch, _toml_string(name)))
    t = data.get('captureSettings', {}).get('digitalTriggerSettings', {})
    ch = t.get('selectedChannel', {}).get('deviceChannel')
    edge = {'Rising': 'rise', 'Falling': 'fall'}.get(t.get('type', {}).get('name'))
    if ch in names and edge:
        lines.append('trigger = { signal = %s, edge = "%s" }' % (_toml_string(names[ch]), edge))
    decoders = []
    for a in data.get('analyzers', []):
        if a.get('type') != 'Simple Parallel':
            continue
        st = {x['title']: x['setting'].get('value') for x in a['settings']}
        bits = [st['D%d' % i] for i in range(16) if st.get('D%d' % i) is not None]
        if st.get('Clock') not in names or any(b not in names for b in bits):
            continue
        decoders.append('  { data = [%s], clock = %s, edge = "%s" },' % (
            ', '.join(_toml_string(names[b]) for b in bits),
            _toml_string(names[st['Clock']]), EDGES[st.get('Clock State') or 0]))
    if decoders:
        lines += ['parallel = ['] + decoders + [']']
    return '\n'.join(lines) + '\n'


def _parallel_analyzer(n, data, clock, edge):
    setting = [{'title': 'D%d' % i, 'tooltip': 'D%d' % i, 'disabled': False,
                'setting': dict({'type': 'Channel', 'channelRequired': False},
                                **({'value': data[i]} if i < len(data) else {}))}
               for i in range(16)]
    setting.append({'title': 'Clock', 'tooltip': 'Clock', 'disabled': False,
                    'setting': {'type': 'Channel', 'channelRequired': True, 'value': clock}})
    setting.append({'title': 'Clock State', 'disabled': False, 'tooltip':
                    'Define whether the data is valid on Clock rising or falling edge',
                    'setting': {'type': 'NumberList', 'value': EDGES.index(edge), 'options': [
                        {'dropdownText': t, 'dropdownTooltip': '', 'value': v}
                        for v, t in enumerate(['Rising edge', 'Falling edge', 'Dual edge'])]}})
    return {'color': '#F69898', 'displayRadix': 'Hexadecimal',
            'name': 'Simple Parallel' + (' [%d]' % n if n else ''), 'settings': setting,
            'showInDataTable': True, 'streamToTerminal': True,
            'type': 'Simple Parallel', 'nodeId': 10000 + n}


def name_sal(path, channels_path, set_name, out):
    """Write path's capture to out with set_name's channel names, rows in
    its `display` order (the rest in channel order), and its Simple
    Parallel decoders unless the capture has decoders already. Every
    other entry of the zip is copied byte for byte; path is left as it
    is. No Logic 2 API names channels."""
    names = la.load_channels(channels_path, set_name)
    _, parallel = la.load_capture(channels_path, set_name)
    with open(channels_path, 'rb') as f:
        display = tomllib.load(f).get(set_name, {}).get('display', [])
    rank = {s: n for n, s in enumerate(display)}
    meta = _sal_meta(path)
    data = meta['data']
    rows = data.get('rowsSettings', [])
    for r in rows:
        if _digital(r):
            r['name'] = names.get(r['channel']['deviceChannel'], r.get('name', ''))
            r['height'] = ROW_HEIGHT
    digital = sorted((r for r in rows if _digital(r)),
                     key=lambda r: (rank.get(r['name'], len(rank)), r['channel']['deviceChannel']))
    data['rowsSettings'] = digital + [r for r in rows if not _digital(r)]
    if not data.get('analyzers'):
        data['analyzers'] = [_parallel_analyzer(n, *p) for n, p in enumerate(parallel)]
    with zipfile.ZipFile(path) as src, zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as dst:
        for item in src.infolist():
            body = json.dumps(meta).encode() if item.filename == 'meta.json' else src.read(item)
            dst.writestr(item, body)


def save_named(logic, handle, channels_path, set_name, out):
    """Add set_name's decoders to the capture, save it as out with its
    channel names, and show the saved file in Logic 2."""
    logic.add_parallel(handle, la.load_capture(channels_path, set_name)[1])
    raw = out + '.unnamed'
    logic.save_capture(handle, raw)
    name_sal(raw, channels_path, set_name, out)
    os.remove(raw)
    logic.show_saved(out)
    return out


def main():
    a = sys.argv[1:]
    if a and a[0] == 'load-preset' and len(a) > 1:
        print(load_preset_toml(a[1], a[2] if len(a) > 2 else None), end='')
    elif a and a[0] == 'load-sal' and len(a) > 1:
        name = a[2] if len(a) > 2 else os.path.splitext(os.path.basename(a[1]))[0]
        print(load_sal_toml(a[1], name), end='')
    elif a and a[0] == 'name-sal' and len(a) == 5:
        name_sal(*a[1:])
    elif a and a[0] == 'arm' and len(a) == 3:
        trigger, _ = la.load_capture(a[1], a[2])
        if trigger is None:
            sys.exit('[%s] has no trigger' % a[2])
        channels = list(la.load_channels(a[1], a[2]))
        print(la.SaleaeLogicPro16().start_capture(channels, **trigger))
    elif a and a[0] == 'save' and len(a) == 4:
        logic = la.SaleaeLogicPro16()
        opened = logic._open()
        if opened is None:
            sys.exit('no capture left open by arm')
        logic.wait_capture(opened['id'])
        print(save_named(logic, opened['id'], a[1], a[2], a[3]))
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
