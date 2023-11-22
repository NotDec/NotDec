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

from dataclasses import dataclass
import json
from notdec_retypd_provider.type_serialization import (
    CTypeJsonEncoder,
    collect_all_non_primitive_types,
)
from loguru import logger

from typing import Optional, Tuple, Dict, List
from retypd import ConstraintSet, Program, Solver, SolverConfig
from retypd.clattice import CLattice, CLatticeCTypes

from retypd.c_type_generator import CTypeGenerator
from retypd.c_types import CType
from retypd.parser import SchemaParser
from pathlib import Path
from retypd.loggable import LogLevel


class RetypdNotdecError(Exception):
    pass


@dataclass
class FunctionPrototype:
    """
    A class to represent simple function prototypes.
    The types of the return variable and the arguments
    are strings or None.
    """

    ret: Optional[str]
    params: List[Optional[str]]


def get_int_and_pointer_size(data_layout: str):
    """
    Get the size of integers and pointer from LLVM data layout string
    """
    # get pointer part
    layouts = [i for i in data_layout.split("-")]
    pointer_layout = [i for i in layouts if i.startswith("p")][0]
    if "64" in pointer_layout:
        return (4, 8)
    elif "32" in pointer_layout:
        return (4, 4)

    raise RetypdNotdecError(f"Unknown ISA {data_layout}")


def constraints_from_json(path: Path) -> Tuple[str, Program]:
    """
    Load constraints from a JSON encoded file
    :returns: Language of original assembly, loaded type Program, and name map
    """
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    data_layout: str = data["data_layout"]
    constraints: Dict[str, List[str]] = data["constraints"]
    callgraph: Dict[str, List[str]] = data["callgraph"]

    parsed_constraints = {}

    for (func, constrs) in constraints.items():
        parsed = set()

        for constr in constrs:
            try:
                parsed.add(SchemaParser.parse_constraint(constr))
            except ValueError as e:
                logger.error(f"Failed to parse constraint {constr}")
                raise e

        parsed_constraints[func] = ConstraintSet(parsed)

    program = Program(
        CLattice(),
        {},
        parsed_constraints,
        callgraph,
    )

    return data_layout, program


def infer_types(json_in: Path, function: Optional[str] = None) -> List[CType]:
    """
    Infer C types for the P-code functions in the `json_in` json file.
    :param json_in: Path to the JSON-encoded constraints
    :param function: If function is specified,
    the type inference only considers the given function.

    """
    data_layout, program = constraints_from_json(json_in)
    # config = SolverConfig(top_down_propagation=True)
    solver = Solver(program, verbose=LogLevel.DEBUG)
    _, sketches = solver()
    for f, sk in sketches.items():
        logger.debug("Sketches: ", f, sk)

    int_size, pointer_size = get_int_and_pointer_size(data_layout)
    gen = CTypeGenerator(
        sketches,
        CLattice(),
        CLatticeCTypes(),
        int_size,
        pointer_size,
    )
    return list(gen().values())


def serialize_types(types: List[CType], dest: Path) -> None:
    all_types = list(collect_all_non_primitive_types(types))
    logger.debug(f"all types {all_types}")
    with open(dest, "w") as f:
        json.dump(all_types, f, cls=CTypeJsonEncoder, indent=2)
