from pathlib import Path

INPUT_FILE = "../etu/invader.pgm"
OUTPUT_FILE = "comment.pgm"

COMMENT = b"# This is a comment\n"

data = Path(INPUT_FILE).read_bytes()

newline_pos = data.find(b"\n")

if newline_pos == -1:
    raise Exception("Invalid PGM file")

header_start = data[:newline_pos + 1]
rest = data[newline_pos + 1:]

modified_data = header_start + COMMENT + rest

Path(OUTPUT_FILE).write_bytes(modified_data)