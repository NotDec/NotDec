# Changelog & TODO

## TODO

- Move all header to a dedicated `include` folder. Make other change according to the cmake project structure.
- Add more comments.
- Write docs in English.

**Backend**
- make null pointer constant really NULL instead of 0.
- add typedef for boolean constant
- handle unordered float pointer operation

## Changelog

**2024-04-26**

1. migrate structural analysis from reko.
2. add reduceSelf: A -> B -> C, B is an empty block, and B has only one succ and only one pred.
3. consider reducing return stmt before SSA destruction. Because: `tmp1 = 10; goto ret; ret: tmp0 = tmp1; return tmp0`
