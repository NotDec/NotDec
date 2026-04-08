#!/usr/bin/env python3

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path


def normalize_text(text: str) -> str:
    text = text.replace("\r\n", "\n").strip()
    if not text:
        return ""
    return "\n".join(line.rstrip() for line in text.split("\n")).strip()


def format_command(cmd: list[str]) -> str:
    return " ".join(subprocess.list2cmdline([part]) for part in cmd)


def resolve_path(base: Path, path_str: str | None) -> Path | None:
    if path_str is None:
        return None
    path = Path(path_str)
    if path.is_absolute():
        return path
    return (base / path).resolve()


def load_manifest(path: Path) -> dict:
    return json.loads(path.read_text())


def expand_template(value: str, manifest_dir: Path, project_root: Path, workdir: Path) -> str:
    return value.format(
        manifest_dir=str(manifest_dir),
        project_root=str(project_root),
        workdir=str(workdir),
    )


def expand_args(
    raw_args: list[str],
    manifest_dir: Path,
    project_root: Path,
    workdir: Path,
) -> list[str]:
    return [
        expand_template(arg, manifest_dir=manifest_dir, project_root=project_root, workdir=workdir)
        for arg in raw_args
    ]


def command_succeeded(
    process: subprocess.CompletedProcess[str],
    ir_output_path: Path,
    snapshot_path: Path,
) -> bool:
    if process.returncode != 0:
        return False
    if "IR parsing failed:" in process.stdout:
        return False
    for path in (ir_output_path, snapshot_path):
        if not path.exists() or path.stat().st_size == 0:
            return False
    return True


def format_log_section(
    title: str,
    command: list[str],
    process: subprocess.CompletedProcess[str],
) -> str:
    return (
        f"## {title}\n"
        f"$ {format_command(command)}\n"
        f"exit={process.returncode}\n\n"
        f"{process.stdout}"
    )


def write_log(log_path: Path, sections: list[str]) -> None:
    log_path.write_text("\n\n".join(sections) + "\n")


def prepare_case_input(
    *,
    case: dict,
    manifest: dict,
    manifest_dir: Path,
    project_root: Path,
    workdir: Path,
    env: dict[str, str],
) -> tuple[Path, list[str], bool]:
    input_path = resolve_path(manifest_dir, case["input"])
    prep = case.get("source_prep", manifest.get("source_prep"))
    if prep is None:
        return input_path, [], True

    kind = prep.get("kind")
    if kind != "clang-ir":
        raise ValueError(f"unsupported source_prep kind: {kind}")

    compiler = prep["binary"]
    compiler_args = expand_args(
        prep.get("args", []),
        manifest_dir=manifest_dir,
        project_root=project_root,
        workdir=workdir,
    )
    prepared_input = workdir / f"{case['name']}.input.ll"
    command = [compiler, *compiler_args, str(input_path), "-o", str(prepared_input)]
    process = subprocess.run(
        command,
        cwd=project_root,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    sections = [format_log_section("prepare", command, process)]
    if process.returncode != 0:
        return prepared_input, sections, False
    return prepared_input, sections, True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--project-root", required=True)
    parser.add_argument("--workdir", required=True)
    args = parser.parse_args()

    manifest_path = Path(args.manifest).resolve()
    manifest_dir = manifest_path.parent
    project_root = Path(args.project_root).resolve()
    workdir = Path(args.workdir).resolve()
    workdir.mkdir(parents=True, exist_ok=True)

    manifest = load_manifest(manifest_path)
    env = os.environ.copy()
    env.update(manifest.get("env", {}))

    counters = {
        "pass": 0,
        "xfail": 0,
        "skip": 0,
        "fail": 0,
        "xpass": 0,
    }

    suite_name = manifest.get("suite", manifest_path.stem)
    default_args = manifest.get("default_args", [])

    print(f"Running suite: {suite_name}")

    for case in manifest["cases"]:
        name = case["name"]
        status = case.get("status", "pass")
        input_path = resolve_path(manifest_dir, case["input"])
        expected_path = resolve_path(manifest_dir, case.get("expected"))
        output_path = workdir / f"{name}.out.ll"
        snapshot_path = workdir / f"{name}.out.htypes"
        log_path = workdir / f"{name}.log"

        if status == "skip":
            counters["skip"] += 1
            print(f"[SKIP ] {name}")
            continue

        for path in (output_path, snapshot_path, workdir / f"{name}.input.ll"):
            if path.exists():
                path.unlink()

        try:
            input_path, log_sections, prepared_ok = prepare_case_input(
                case=case,
                manifest=manifest,
                manifest_dir=manifest_dir,
                project_root=project_root,
                workdir=workdir,
                env=env,
            )
        except Exception as exc:
            counters["fail"] += 1
            log_path.write_text(f"prepare exception: {exc}\n")
            print(f"[FAIL ] {name}")
            print(f"        log: {log_path}")
            continue

        if not prepared_ok:
            write_log(log_path, log_sections)
            if status == "xfail":
                counters["xfail"] += 1
                reason = case.get("reason")
                suffix = f" ({reason})" if reason else ""
                print(f"[XFAIL] {name}{suffix}")
            else:
                counters["fail"] += 1
                print(f"[FAIL ] {name}")
                print(f"        log: {log_path}")
            continue

        command = [
            args.binary,
            str(input_path),
            "-o",
            str(output_path),
            *default_args,
            *case.get("args", []),
            "--dump-htypes",
            str(snapshot_path),
        ]

        process = subprocess.run(
            command,
            cwd=project_root,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        log_sections.append(format_log_section("notdec", command, process))
        write_log(log_path, log_sections)

        succeeded = command_succeeded(process, output_path, snapshot_path)
        matches_expected = False

        if succeeded and expected_path is not None:
            actual = normalize_text(snapshot_path.read_text())
            expected = normalize_text(expected_path.read_text())
            matches_expected = actual == expected
        elif succeeded and expected_path is None:
            matches_expected = True

        if status == "pass":
            if succeeded and matches_expected:
                counters["pass"] += 1
                print(f"[PASS ] {name}")
                continue

            counters["fail"] += 1
            print(f"[FAIL ] {name}")
            print(f"        log: {log_path}")
            if expected_path is not None and succeeded and not matches_expected:
                print(f"        expected: {expected_path}")
                print(f"        actual:   {snapshot_path}")
            continue

        if status == "xfail":
            if succeeded and matches_expected:
                counters["xpass"] += 1
                print(f"[XPASS] {name}")
                print(f"        log: {log_path}")
            else:
                counters["xfail"] += 1
                reason = case.get("reason")
                suffix = f" ({reason})" if reason else ""
                print(f"[XFAIL] {name}{suffix}")
            continue

        print(f"[FAIL ] {name}")
        print(f"        unknown status: {status}")
        counters["fail"] += 1

    print(
        "Summary: "
        f"{counters['pass']} passed, "
        f"{counters['xfail']} xfailed, "
        f"{counters['skip']} skipped, "
        f"{counters['xpass']} xpassed, "
        f"{counters['fail']} failed"
    )

    return 0 if counters["fail"] == 0 and counters["xpass"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
