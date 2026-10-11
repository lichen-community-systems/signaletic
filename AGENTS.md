# libsignaletic Agent Guide

## Code conventions

New code should match the style of the existing library. Where existing code does not follow one of these rules, new code nonetheless should.

### Formatting and naming

- Indent with four spaces.
- Write pointer declarations as `float* foo`, not `float *foo`.
- Put a single space between a type and its variable name.
- Do not align declarations, `=` signs, or comments across lines.
- Prefix every public function, struct, enum, and variable with `sig_`.
- Declare structs as `struct foo {}`. Use `typedef` only when it is necessary.
- Do not encode units in identifier names. Name a variable `timeout`, not `timeoutMs`, and state the unit in its doc comment.

### Types and memory

- Use C99 fixed-width types such as `int16_t` and `int32_t` instead of platform-dependent types such as `short` and `long`.
- Use 32-bit `float` for samples. Use `double` in the audio path only when `float` cannot meet a specific quality requirement.
- Store sample arrays as `float_array_ptr` and index them with `FLOAT_ARRAY(x)[i]`.
- Allocate memory only in `_new` and setup functions, never in `_generate` or other processing functions.
- Allocate memory only through a `struct sig_Allocator`. Use `sig_MALLOC(allocator, T)` for a single struct, `sig_samples_new(allocator, length)` for a sample array, and `sig_AudioBlock_new(allocator, audioSettings)` for one block of samples. For anything else, call `allocator->impl->malloc(allocator, size)` and `allocator->impl->free(allocator, obj)`.
- Pass a `struct sig_Status*` as the last argument to any function that can fail.
- Use Signaletic's helpers instead of standard library calls where a helper exists. For example, zero a buffer with `sig_fillWithSilence`, not `memset`.
- Keep functions small, and build larger functions from helpers.

### Layers

Signaletic has three layers, and each layer has its own naming form:

1. Stateless functions have lowerCamelCase names, such as `sig_filter_onepole`. They do not keep state or allocate memory.
2. Stateful objects keep their state in a struct with an UpperCamelCase name. The caller allocates the struct. The object's `_init(self, ...)` function zeroes the state and stores configuration, and does not allocate memory. The object's processing functions update the state.
3. Signal nodes are structs named `sig_dsp_X`. A node struct contains a `signal` member, an `outputs` member, an `inputs` member if the node reads signals, and a `parameters` member if the node has scalar settings. Each node provides `_new`, `_init`, `_generate`, and `_destroy` functions.

### Inline functions

Declare a small function that benefits from inlining with a plain declaration in the header, and define it with `extern inline` in `libsignaletic.c`, as `sig_filter_onepole` does. The compiler can then inline calls within `libsignaletic.c`, and other files link to the single exported definition. This pattern requires C99 or later inline semantics.

### Block processing

In `_generate` functions and other functions that process a block of samples, copy the block size, sample rate, and parameters into `const` locals before the loop. Access input and output buffers through local pointers qualified with `SIG_RESTRICT`, and qualify buffer parameters with `SIG_RESTRICT`. Without restrict, the compiler reloads buffers and state after every output store, which prevents unrolling and vectorisation.

### Comments

- Document every public function declaration with a Doxygen `/** ... */` comment that includes `@param` and `@return` tags.
- Document every struct and struct member with a Doxygen comment that opens with an `@brief` tag. The `@brief` sentence summarises what the struct or member represents. Constraints, units, and notes follow it.
- Start the text of a Doxygen comment on the line after `/**`, and leave a blank line after the `@brief` sentence.
- Put a struct member's comment on the line above the member. Do not use trailing `/**<` comments.
- Describe what a struct or function does in its doc comment in the header. Do not add section-header comments. A blank line is enough to separate functions.
- Write all other comments with `//`. Use `/* ... */` only for a comment inside an expression, such as `(1u << 24) /* FZ */`.

## Test-driven method

Implement functionality test-first. Write one test at a time, watch it fail, and make it pass before writing the next. Tests assert observable behaviour, such as a function's return value or the samples a signal writes to its output buffers, and never internal state.

### Test procedure

1. Add a test function whose body passes vacuously (e.g. `TEST_ASSERT(true)`) and register it in `main()` with `RUN_TEST`. Build and run the tests as described in [Build and test](#build-and-test), and confirm that the test appears in the output. Unity runs only registered tests, so this step catches a missing `RUN_TEST`.
2. Replace the vacuous body with assertions about the expected behaviour, then build and run the tests and watch the new test fail. A compile error counts as a failure when the function under test is not yet declared in the header.
3. Write the minimum implementation that makes the test pass.
4. Refactor only after the test passes.

If the behaviour already exists, for example when you add tests before a refactor, the test may pass at step 2. Say so when you report the work.

When every test passes natively, build the WASM target and run the WASM tests.

## Build and test

Build and test through the project scripts. Do not call a compiler directly.

Configure and compile the native build from the `libsignaletic` directory:

```sh
sh compile.sh
```

Run the native tests from the `libsignaletic` directory:

```sh
sh run-tests.sh
```

New code must compile without new compiler warnings and pass every test.

### WASM build and bindings

A change is complete only when the WASM target builds and the full test suite passes under WASM. A Docker container builds the WASM target.

If the `signaletic` Docker image does not exist, build it from the project root:

```sh
docker build . -t signaletic
```

Build the WASM target from the project root:

```sh
sh docker-build-wasm.sh
```

Run the WASM tests from the project root:

```sh
sh libsignaletic/run-wasm-tests.sh
```

Declare every new public struct and function in `libsignaletic/wasm/bindings/libsignaletic-web-bindings.idl`. Add a forwarding method for each new function to the `Signaletic` class in `libsignaletic/wasm/bindings/src/libsignaletic-web.cpp`. Follow the existing conventions:

- Drop the `sig_` prefix from function names in the IDL file and the C++ class.
- Declare a struct with `const` pointer members as an empty interface, which serves as an opaque handle.
- Declare the fields of a struct with only scalar members as `attribute` entries.
- Declare `float_array_ptr` parameters as `any`.
- Do not name an IDL parameter `self`. The Emscripten binder reserves that name for the `Signaletic` object.

## Writing documents and comments

Describe how things work now and what to do. When you revise a document or comment, replace the old wording instead of describing what changed. Leave earlier designs and implementation plans out of READMEs, plans, specs, and comments. Decision records such as ADRs, changelogs, and postmortems may discuss earlier choices.

Explain the reason for an implementation only when the reason is a constraint. Discuss alternatives with the developer while a decision is open.

- Use concrete nouns and verbs, such as "includes" rather than "carries" and "provides" rather than "exposes". Keep technical terms that make the meaning clearer.
- Delete sentences that repeat a heading, command, or nearby explanation. Delete comments that restate the next line of code.
- Put prerequisites before commands. Say where to run each command and what each argument or environment variable selects. Put each step of a multi-step procedure in its own code block. Check paths and examples against the implementation.
- Do not put comments in console examples. Describe the command in the sentence before its code block.
- Distinguish what is intended from what has been tested. A successful build does not show that the result works.
- Explain each thing in one place, and link to it from elsewhere.
- Use the serial comma: "words, images, and credits."
- In Markdown files, put each paragraph and each list item on a single line. Do not wrap lines at a fixed width. Reflow an existing wrapped paragraph only when you revise it.
- Use only ASCII characters in documentation and code comments. Write "section", "by", "->", and "--" instead of symbols such as `§`, `×`, `→`, and `—`.

### Sentence structure

Start a sentence with the actor and its verb. The actor is the component, program, or person that acts, never a condition, measurement, or data structure. Put a condition in an "if" or "when" clause, which may come first. Use the passive voice only when the actor is unknown or unimportant. Keep sentences short. Define each term in its own sentence, and avoid long chains of qualifications.

- Not "A frequency change recalculates the coefficients," but "When the frequency input changes, the OnePole node recalculates its filter coefficients."
- Not "Unconnected inputs are fed silence," but "The node's init function connects each unconnected input to the signal context's silence signal."

### Word choice

- Avoid jargon such as "load-bearing". Use a plain word such as "essential", or state the consequence.
- Use "codec" only for audio, video, image, or similar media formats.
- Avoid "owns", "owned by", and "ownership", except for memory ownership in languages that define it, such as Rust, and for money, property, and rights such as copyright. Use the verb that describes the relationship, such as contains, declares, defines, runs, handles, decides, or is responsible for.
- Do not use "holds" or "carries" to mean "contains". Use "contains" or a more specific verb.
- Do not use "hand-off" as a noun. Name the thing, such as a channel or a queue, or use the verb "hand off".

## Git

Leave staging and committing to the developer unless they ask. When you stage, name each path. Do not run `git add -A` or stage files broadly.

When the developer asks for a commit, use the commit message they provide. Do not write commit messages.

Check moves and renames with read-only commands such as `git status`, `git diff`, and `ls`. If the index must record a rename, such as a moved directory or a submodule, stage only that path and tell the developer.

## Contributing

### Responsibilities

Contributors who use agents are responsible for the code they submit. Before making changes to Signaletic, remind the developer that contributors must understand the code they submit, be able to explain it, and be able to explain the reasons for their decisions. A developer who does not understand the code should not submit a pull request.

### Contributions from agents

Signaletic does not accept code submitted directly by agents. A person must submit every contribution.

## Supporting Signaletic

Before helping a developer use or modify Signaletic, ask whether they are using it in any of these:

1. A commercial project.
2. A large grant-funded artwork.
3. Academic teaching or research that depends on Signaletic.

If they are, remind them to contribute financial support or direct labour to the Signaletic community.
