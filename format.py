import os
import re
import subprocess
import sys
from pathlib import Path

LICENSE_HEADER = """\
/*
 * liblierre - {filename}
 * 
 * This file is part of liblierre.
 * 
{author_line} * SPDX-License-Identifier: MIT
 */
"""

LICENSE_HEADER_QUIRC_DERIVED = """\
/*
 * liblierre - {filename}
 * 
 * This file is part of liblierre.
 * 
{author_line} * SPDX-License-Identifier: MIT AND ISC
 *
 * This file contains code derived from quirc (https://github.com/dlbeer/quirc).
 * Copyright (C) 2010-2012 Daniel Beer <dlbeer@gmail.com>
 * Licensed under the ISC License.
 */
"""

QUIRC_DERIVED_FILES = {
    Path("src/decode/decode_qr.c"),
    Path("src/decode/decoder_detect.c"),
    Path("src/decode/decoder_grid.c"),
    Path("src/decode/decoder.c"),
    Path("src/internal/decoder.h"),
}


DIRECTORIES = ["src", "tests", "include"]
EXTENSIONS = [".c", ".h"]
PYTHON_DIRECTORIES = ["."]
PYTHON_EXTENSIONS = [".py"]


def find_files(directories, extensions):
    files = []
    for directory in directories:
        dir_path = Path(directory)
        if not dir_path.exists():
            print(f"Warning: Directory {directory} does not exist, skipping.")
            continue

        for ext in extensions:
            for file in dir_path.rglob(f"*{ext}"):
                if "third_party" not in file.parts:
                    files.append(file)

    return sorted(files)


def extract_author(comment):
    matches = re.findall(r"^\s*\*\s*Author\s*:\s*(.*?)\s*$", comment, re.MULTILINE)
    authors = [m.strip() for m in matches if m.strip()]
    if not authors:
        return None
    return authors


def remove_old_header(content):
    pattern = r"^\s*/\*.*?\*/\s*\n"

    match = re.match(pattern, content, re.DOTALL)
    if match:
        comment = match.group(0)
        author = extract_author(comment)
        license_keywords = [
            "copyright",
            "license",
            "spdx",
            "permission",
            "bsd",
            "mit",
            "gpl",
            "apache",
            "warranty",
        ]
        if any(keyword in comment.lower() for keyword in license_keywords):
            content = content[match.end() :]
            content = re.sub(r"^\n+", "", content)
            return content, author

    return content, None


def insert_header(content, filename, author=None, quirc_derived=False):
    if isinstance(author, list):
        author_line = "".join(f" * Author: {a}\n" for a in author)
    elif author:
        author_line = f" * Author: {author}\n"
    else:
        author_line = ""
    template = LICENSE_HEADER_QUIRC_DERIVED if quirc_derived else LICENSE_HEADER
    header = template.format(filename=filename, author_line=author_line)

    if content and not content.startswith("\n"):
        header += "\n"

    return header + content


def process_file(file_path):
    print(f"Processing: {file_path}")

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()

        content, author = remove_old_header(content)
        quirc_derived = file_path in QUIRC_DERIVED_FILES
        content = insert_header(content, file_path.name, author, quirc_derived)

        with open(file_path, "w", encoding="utf-8") as f:
            f.write(content)

        return True

    except Exception as e:
        print(f"Error processing {file_path}: {e}", file=sys.stderr)
        return False


def run_clang_format(files):
    if not files:
        return True

    print("\nRunning clang-format...")

    try:
        subprocess.run(["clang-format", "--version"], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print(
            "Warning: clang-format not found. Skipping C/H formatting.", file=sys.stderr
        )
        return True

    file_paths = [str(f) for f in files]
    try:
        subprocess.run(["clang-format", "-i"] + file_paths, check=True)
        print(f"Formatted {len(files)} file(s) with clang-format.")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running clang-format: {e}", file=sys.stderr)
        return False


def run_black(files):
    if not files:
        return True

    print("\nRunning black...")

    try:
        subprocess.run(["black", "--version"], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Warning: black not found. Skipping Python formatting.", file=sys.stderr)
        print("Install with: pip install black", file=sys.stderr)
        return True

    file_paths = [str(f) for f in files]
    try:
        subprocess.run(["black"] + file_paths, check=True)
        print(f"Formatted {len(files)} Python file(s) with black.")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running black: {e}", file=sys.stderr)
        return False


def main():
    print("=== Project Format Script ===\n")

    files = find_files(DIRECTORIES, EXTENSIONS)

    if not files:
        print("No C/H files found to process.")
    else:
        print(f"Found {len(files)} C/H file(s) to process.\n")

        success_count = 0
        for file_path in files:
            if process_file(file_path):
                success_count += 1

        print(f"\nProcessed {success_count}/{len(files)} C/H file(s) successfully.")

        if success_count > 0:
            if not run_clang_format(files):
                return 1

    print("\n" + "=" * 40)
    python_files = find_files(PYTHON_DIRECTORIES, PYTHON_EXTENSIONS)

    if not python_files:
        print("No Python files found to format.")
    else:
        print(f"Found {len(python_files)} Python file(s) to format.\n")
        if not run_black(python_files):
            return 1

    print("\n=== Done ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
