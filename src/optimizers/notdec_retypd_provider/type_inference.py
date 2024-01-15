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
from retypd.schema import maybe_to_var, DerivedTypeVariable, SubtypeConstraint

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


def constraints_from_json(path: Path) -> Tuple[str, Program, dict, dict]:
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
    func_vars = {}

    for (func, constrs) in constraints.items():
        parsed = set()
        vars = set()

        for constr in constrs:
            try:
                cons = SchemaParser.parse_constraint(constr)
                parsed.add(cons)
                vars.add(cons.left); vars.add(cons.right)
            except ValueError as e:
                logger.error(f"Failed to parse constraint {constr}")
                raise e

        parsed_constraints[func] = ConstraintSet(parsed)
        func_vars[maybe_to_var(func)] = vars

    program = Program(
        CLattice(),
        {}, # TODO add global vars
        parsed_constraints,
        callgraph,
    )

    return data_layout, program, parsed_constraints, func_vars


def infer_types(json_in: Path, function: Optional[str] = None, debug_dir: Optional[str] = None) -> List[CType]:
    """
    Infer C types for the P-code functions in the `json_in` json file.
    :param json_in: Path to the JSON-encoded constraints
    :param function: If function is specified,
    the type inference only considers the given function.

    """
    data_layout, program, constraint_map, func_vars = constraints_from_json(json_in)
    int_size, pointer_size = get_int_and_pointer_size(data_layout)

    # config = SolverConfig(top_down_propagation=True)
    solver = Solver(program, verbose=LogLevel.DEBUG)
    # https://github.com/GrammaTech/gtirb-ddisasm-retypd/blob/c084da35d2765aca2cf351389dee6482e1033888/src/ddisasm_retypd/ddisasm_retypd.py#L204
    type_schemes, sketches_map = solver()
    for f, sk in sketches_map.items():
        logger.debug(f"Sketches: {f}:\n{sk}")
    # sketches map is a map from func name to func type sketch
    result = {}
    for f in sketches_map:
        logger.debug(f"inferring shape for all vars in {f}")
        sks = {f: sketches_map[f]}
        var_sketches = solver.get_type_of_variables(sketches_map, type_schemes, f, func_vars[f])
        sks.update(var_sketches)
        gen = CTypeGenerator(
            sks,
            CLattice(),
            CLatticeCTypes(),
            int_size,
            pointer_size,
        )
        result[f.base] = gen()

    if debug_dir is not None:
        with open(Path(debug_dir) / "derived_constraints.txt", "w") as f:
            for dtv, derived_constraint in type_schemes.items():
                f.write("-------" + '\n')
                f.write(str(dtv) + '\n')
                f.write("ORIGINAL" + '\n')
                f.write(str(constraint_map[str(dtv)]) + '\n')
                f.write("DERIVED" + '\n')
                f.write(str(derived_constraint) + '\n')

            for (dtv, sketch) in sketches_map.items():
                out = Path(debug_dir) / f"{dtv.base}.dot"
                if len(sketch.sketches) > 0:
                    try:
                        out.write_text(sketch.to_dot(dtv))
                    except KeyError:
                        logger.warning(f"Cannot draw {dtv} sketch")

    # for f, sk in sketches_map.items():
    #     solver.get_type_of_variables(sketches_map, type_schemes, f, set())

    return result

from retypd.schema import DerivedTypeVariable
def serialize_types(types: Dict[str, Dict[DerivedTypeVariable, CType]], dest: Path) -> None:
    all_types = {}
    for func, type_dict in types.items():
        var_types = {}
        for dtv, ctype in type_dict.items():
            var_types[str(dtv)] = list(collect_all_non_primitive_types([ctype]))
        all_types[func] = var_types
    logger.debug(f"all types {all_types}")
    with open(dest, "w") as f:
        json.dump(all_types, f, cls=CTypeJsonEncoder, indent=2, ensure_ascii=False)
