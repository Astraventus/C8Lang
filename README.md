# C8L

A simple, assembly-like compiler and language for CHIP-8. Write an assembly-style code, get a `*.rom` file, play it.

The compiler features no dependencies aside from pure C and makefile.

C8L is intentionally minimal, it designed to feel authentic to 70s, quick and simple prototyping of small ROMs without complexities of higher-level languages.
Whatever you write maps almost directly to CHIP-8 instructions

## Building 

```bash
make all
```

## Running

```bash
./c8l -o <output-file.rom> <source file>
```

## Design


C8L keeps things simple because CHIP-8 is simple.

**What you see is what you get** — Each line of code turns into one CHIP-8 instruction (or two for if-statements). There's no magic happening behind the scenes. If you write `v0 += 5`, the compiler literally emits `0x7005`.

**No expression parsing** — You can't write `v0 = v1 + v2 + 5`. You have to do one operation at a time: `v0 = v1`, then `v0 += v2`, etc. The parser is dumber this way, but the generated code is predictable.

**Labels are just numbers** — When you write `goto start`, the compiler looks up where `start:` is and replaces it with the actual memory address (like `0x202`). Same for sprites — the name becomes the address of the first byte.

**No stack, no functions** — CHIP-8 has a call stack, but C8L doesn't touch it. No subroutines, no local variables, no recursion. Just jumps and registers, same as writing raw assembly but with nicer syntax.

That's basically it. The compiler does two passes: first to count bytes and note where labels are, second to actually write the bytes. No optimization, no intermediate representation.

## License

MIT (see LICENSE)

You can use it however your heart desires. Just credit the author/repository.