"""
Writing generated source files, for any importer: assembler data lines, and
a set of files written all or none.
"""
import os


def db(values, comment=None):
    """A db line of bytes, in hex, with a comment after it if given."""
    line = '\tdb ' + ', '.join(f'${v:02X}' for v in values)
    return line + f'  ; {comment}' if comment else line


def write_all(out_dir, files):
    """Write every file ({name: text}) to a temporary name, then rename them all: nothing is half written."""
    os.makedirs(out_dir, exist_ok=True)
    temps = []
    try:
        for name, text in files.items():
            temp = os.path.join(out_dir, name + '.tmp')
            temps.append(temp)
            with open(temp, 'w') as f:
                f.write(text)
        for name in files:
            os.replace(os.path.join(out_dir, name + '.tmp'), os.path.join(out_dir, name))
    finally:
        for temp in temps:
            if os.path.exists(temp):
                os.remove(temp)
