"""Synchronize the headers bundled beside each sketch."""
from pathlib import Path
import argparse

ROOT = Path(__file__).resolve().parents[1]
DRIVER = ('Hv5812Board.h', 'Hv5812Transport.h', 'Hv5812Panel.h')
SKETCHES = {'RawGrid': DRIVER, 'TextDisplay': DRIVER + ('VfdAscii.h', 'VfdDisplay.h'),
            'VfdInvaders': DRIVER + ('VfdAscii.h',)}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    stale = []
    for sketch, names in SKETCHES.items():
        for name in names:
            source = ROOT / 'libraries/Hv5812Vfd/src' / name
            target = ROOT / 'examples' / sketch / name
            if target.exists() and target.read_bytes() == source.read_bytes(): continue
            stale.append(str(target.relative_to(ROOT)))
            if not args.check: target.write_bytes(source.read_bytes())
    if args.check and stale: raise SystemExit('Headers differ: ' + ', '.join(stale))
    print('Headers match' if args.check else 'Headers synchronized')

if __name__ == '__main__': main()
