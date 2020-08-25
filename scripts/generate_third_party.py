#!/usr/bin/env python3

import argparse
import subprocess
from pathlib import Path
from shutil import rmtree
from urllib.parse import quote

import call_wrapper

def generate():
    cur_dir = Path(__file__).parent.absolute()
    root_dir = cur_dir.parent

    output_file = root_dir/'THIRD_PARTY_NOTICES.md'
    output_file.unlink(missing_ok=True)

    with open(root_dir/'.license_index.txt') as f:
        index = [l.strip().split(': ') for l in f.readlines() if l.strip()]

    licenses = [f for f in (root_dir/'component'/'licenses').glob('*.txt')]

    if set([l[0] for l in index]) != set([l.stem for l in licenses]):
        raise RuntimeError('License file mismatch:\n'
                           f'Index: {set([l for l in index[0]]) - set([l.stem for l in licenses])}\n'
                           f'Files: {set([l.stem for l in licenses]) - set([l for l in index[0]])}')

    with open(output_file, 'w') as output:
        output.write('Spotify Integration uses third-party libraries or other resources that may\n')
        output.write('be distributed under licenses different than the Spotify Integration software.\n')
        output.write('')
        output.write('The linked notices are provided for information only.\n')
        output.write('\n')
        for (dep_name, license) in index:
            output.write(f'- [{dep_name} - {license}](component/licenses/{quote(dep_name)})\n')

    print(f'Generated file: {output_file}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate MD license file base on the folder with licenses')

    args = parser.parse_args()

    call_wrapper.final_call_decorator(
        'Generating MD license file',
        'Generating MD license file: success',
        'Generating MD license file: failure!'
    )(
    generate
    )(
    )
