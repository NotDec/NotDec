# Retypd - machine code type inference
# Copyright (C) 2022 GrammaTech, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

from pathlib import Path
from notdec_retypd_provider.type_inference import infer_types, serialize_types
from loguru import logger
import argparse
import sys


def main():
    logger.enable("notdec_retypd_provider")
    logger.add(
        sys.stdout,
        format="{time} {level} {message}",
        filter="notdec_retypd_provider",
        level="INFO",
    )

    parser = argparse.ArgumentParser(
        description="Infer types from JSON constraints file using Retypd",
    )

    parser.add_argument(
        "--json-in", type=Path, help="The json file to analyze", required=True
    )
    parser.add_argument(
        "--function",
        type=str,
        help="The function to analyze, "
        "if not specified the whole program is analyzed",
    )
    args = parser.parse_args()
    types = infer_types(args.json_in, args.function)
    output_path = args.json_in.with_name(args.json_in.name + ".types.json")
    serialize_types(types, output_path)


if __name__ == "__main__":
    main()
