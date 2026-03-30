# Debug Notes

This file records practical notes for debugging this repository with `debugmcp`.

## General Flow

1. Read the `debugmcp` instructions first with `get_debug_instructions`.
2. Add at least one breakpoint before starting a session.
3. Start debugging with a named configuration from `.vscode/launch.json`.
4. Prefer tracing backwards from the symptom until the real root cause is found.

## Breakpoints

- Put breakpoints on executable lines, not on function signatures or comments.
- When chasing a bad value, set a breakpoint where the bad value is observed first.
- If needed, restart the session after adding a deeper breakpoint.
- Clear stale breakpoints before a focused debug session, otherwise unrelated stops can waste time.

## `evaluate_expression`

- Use `evaluate_expression` with an `expr` prefix.
- Prefer the LLDB-style form `expr -- <expression>`.
- Examples:
  - `expr -- MaxEndOff`
  - `expr -- Ent.first.offset`
  - `expr -- (long long)Ent.first.offset`
- If you reference a variable that is not in the current frame, LLDB reports it as an undeclared identifier.
- In the current `debugmcp` integration, some expressions may execute without returning visible text in `Result:` even when the command format is accepted.
- When expression output is empty, fall back to `get_variables_values` and stack navigation tools such as `step_out`, `step_over`, and `continue_execution`.

## Session Notes

- Some launch configurations depend on `${file}` in `.vscode/launch.json`. If you want a reproducible debug target, temporarily replace `${file}` with a concrete test input.
- Keep the debug target stable while investigating a root cause, otherwise the observed type state may change between runs.
- If the stack frame looks inconsistent after stepping, restart the session and reproduce the same stop point.

## Investigation Style

- Distinguish symptom from root cause.
- Record the exact file, line, and variable state where the symptom appears.
- Verify whether a wrong size or type was already wrong at input creation, or only becomes wrong during later transformation.
- For type recovery issues, inspect both:
  - the place where the bad type is consumed
  - the earlier phase where that type or size was first created
