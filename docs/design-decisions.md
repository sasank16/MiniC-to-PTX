# Design Decisions & GPU Execution Model

## 1. NVIDIA PTX vs Machine SASS

A fundamental concept when working with NVIDIA GPUs is the distinction between **PTX (Parallel Thread Execution)** and **SASS (Streaming Assembler / Machine Assembly)**:

1. **PTX is an Intermediate Virtual ISA**:
   - PTX is an architecture-independent virtual assembly language.
   - It assumes an infinite set of typed virtual registers (`%r0`, `%r1`, ..., `%rd0`, `%p0`).
   - It abstracts away hardware-specific instruction scheduling, warp allocation, register file limits, and machine instruction encodings.
   - It is distributed inside CUDA binaries (`fatbin`) to allow forward compatibility across future GPU architectures.

2. **SASS is the Actual GPU Machine Code**:
   - The NVIDIA optimizing assembler (`ptxas`) compiles PTX into SASS for a specific hardware Streaming Multiprocessor (SM) target (e.g., `sm_75`, `sm_80`, `sm_90`).
   - Physical register allocation takes place during PTX -> SASS compilation. If a kernel uses too many physical 32-bit registers, `ptxas` either spills registers to local memory (DRAM backed, cached in L1/L2) or limits thread occupancy.

MiniC-to-PTX targets the **PTX ISA**, generating valid virtual PTX instructions.

---

## 2. Kernel Interface vs Device Function ABI

In standard CPU C compilers, `int main()` or any integer-returning function places its return value in an ABI register (e.g. `%eax` on x86-64).

On a GPU:
- **PTX Device Functions (`.func`)**:
  - Callable from other GPU functions.
  - Can accept `.param` input arguments and return a `.param` value:
    ```ptx
    .visible .func (.param .s32 %retval) add_two(.param .s32 %a, .param .s32 %b) {
        .reg .s32 %r<3>;
        ld.param.s32 %r0, [%a];
        ld.param.s32 %r1, [%b];
        add.s32 %r2, %r0, %r1;
        st.param.s32 [%retval], %r2;
        ret;
    }
    ```
- **PTX Entry Kernels (`.entry`)**:
  - Invoked from host CPU code via the CUDA driver or runtime API (`cuLaunchKernel`).
  - **Kernels cannot return values directly via registers**. GPU kernels execute asynchronously across thousands of parallel threads in thread blocks.
  - To communicate results back to the host, a kernel must receive a global memory pointer (e.g. `int* out` represented as `.param .u64 %out_ptr`) and write its calculated value into that memory address (`st.global.s32 [%rd], %r`).

### Compiler Support in MiniC-to-PTX:
MiniC-to-PTX provides first-class support for both modes via CLI flags:
- `--target=func` (default for functions): Emits a `.func` returning `.param .s32`.
- `--target=kernel`: Emits a `.entry` kernel where the return statement stores the output into an explicit output buffer passed as a 64-bit parameter pointer.

---

## 3. Register Naming and Typing in PTX

PTX mandates typed virtual registers. In MiniC-to-PTX, we use the standard PTX register convention:
- `%r<n>`: 32-bit integer registers (`.reg .s32 %r0, %r1...`).
- `%p<n>`: 1-bit predicate registers (`.reg .pred %p0, %p1...`) used for conditional branching and predicated execution (`@%p0 bra label;`).
- `%rd<n>`: 64-bit address registers (`.reg .u64 %rd0...`) used for global pointers and memory addressing.

---

## 4. Intermediate Representation Design

We chose a linear Three-Address Code (TAC) organized into a Control-Flow Graph (CFG) of Basic Blocks.
- Why not AST-to-PTX directly?
  - Direct AST to PTX makes optimization (constant propagation, dead-code elimination) complicated and tightly couples language syntax to PTX ISA quirks.
  - Separating AST lowering into TAC IR allows clean, target-independent optimization passes before feeding into any backend.
- Why Basic Blocks?
  - A Basic Block is a maximal sequence of instructions with single entry and single exit. This makes control flow explicit via edges and branch targets, allowing robust CFG analysis and easy translation to PTX branch instructions (`bra`).
