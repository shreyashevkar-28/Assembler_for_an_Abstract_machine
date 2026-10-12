# Assembler_for_an_Abstract_machine
x86 32-bit compatible

compile:
g++ -std=c++17 -O2 -Wall -Wextra -o assembler main.cpp worklet2.cpp

run:
./assembler t2.asm out.bin

validate:
objdump -D -b binary -mi386 -M intel out.bin


FLOW:
How test.asm file is converted into machine code inside out.bin file
**[ 1. Input Initialization ]**
* **Source File Reading:** The pipeline begins in `main_2.cpp`, where `readSourceFile()` loads the raw text of the assembly file (`t2.asm`) line-by-line into a string vector.
* **Execution Handoff:** The main function calls `runWorklet1(sourceLines)` to trigger the front-end string processing.

⬇️

**[ 2. Worklet 1: Lexical Analysis ]**
* **Tokenization:** Inside `worklet1.cpp`, a loop passes each source line to `tokenizeLine()`.
* **Categorization:** The lexer strips whitespace and comments, converting raw text into a `vector<Token>` by classifying elements as `MNEMONIC`, `REGISTER`, `IMMEDIATE`, `IDENTIFIER`, or `LABEL`.

⬇️

**[ 3. Worklet 1: Parsing & Validation ]**
* **Instruction Grouping:** Tokens are fed into `parseLine()`, which separates label definitions into a `LabelDef` vector and sends instructions to `parseInstruction()`.
* **IR Construction:** `parseInstruction()` maps the strings into an `Instruction` struct, assigning generic kinds like "REG" or "IMM" to operands.
* **Hardware Validation:** The syntax is strictly checked against standard x86 rules using `expectedOperandCount()` and `checkOperandKinds()` to prevent illegal operand combinations.

⬇️

**[ 4. Worklet 1: Pass 1 (Addresses & Symbols) ]**
* **Location Counter Calculation:** The `pass1()` function iterates over the generated Intermediate Representation (IR) instructions.
* **Sizing:** It calls `getInstructionLength()` to assign the exact required byte count for each mnemonic.
* **Symbol Table Generation:** As it tracks the running byte total (the Location Counter), it populates a `symbolTable` mapping every detected label name to its precise memory address.

⬇️

**[ 5. Worklet 1: Pass 2 (Reference Resolution) ]**
* **Displacement Calculation:** `pass2Resolve()` scans the instruction list specifically for `SYM` operands (jump targets).
* **Lookup:** It searches the `symbolTable` for the target's address and calculates the jump displacement by subtracting the end address of the current instruction from the target address.
* **Return:** The fully resolved `Worklet1Output` (containing the IR and symbol table) is returned in memory to `main_2.cpp`.

⬇️

**[ 6. Adapter: Lowering to Enums ]**
* **Translation Bridge:** Back in `main_2.cpp`, the `Lowering::lower()` method translates the string-heavy IR into a strictly typed format.
* **Enum Mapping:** It uses `lookupOp()` and `lookupReg()` to convert textual mnemonics and registers into C++ enums defined in `worklet2_2.h` (e.g., `w2::Op::MOV`, `w2::Reg::EAX`).
* **ID Assignment:** Label text strings are converted into dense integer identifiers (`w2::LabelId`), and the result is packaged into a `w2::Program` struct before being sent to `w2::encode()`.

⬇️

**[ 7. Worklet 2: Machine Code Encoding ]**
* **Initialization:** Inside `worklet2_3.cpp`, `encodeImpl()` reserves memory for the final `out` byte vector and initializes a `fixups` array to track unresolved labels.
* **Instruction Routing:** It loops over the `w2::Program` and passes each instruction to `encodeOne()`, which routes execution via a `switch` statement based on the `Op` enum.
* **Byte Generation:** Opcodes are pulled using `singleByteOpcode()` or `aluForm()`, and memory addressing bytes are calculated and pushed to the vector via `emitModRM()`.
* **Placeholders:** If an instruction jumps to a label, a `Fixup` is recorded and placeholder zeros are temporarily pushed into the byte stream.

⬇️

**[ 8. Worklet 2: Back-Patching ]**
* **Offset Resolution:** After all bytes are emitted, `encodeImpl()` iterates through the list of recorded `fixups`.
* **Math Application:** It computes the exact absolute address or `rel32` relative jump distance using the final label offsets.
* **Overwriting:** It calls `patch32()` to overwrite the temporary placeholder zeros in the byte array with the correct mathematically calculated offsets.

⬇️

**[ 9. Verification & Final Output ]**
* **Cross-Check:** The completed byte array is returned to `main_2.cpp`, which immediately runs `crossCheck()` to ensure the generated array size matches the byte lengths predicted during Worklet 1's Pass 1.
* **Disk Write:** If validation passes, `writeBinFile()` opens an output stream in `std::ios::binary` mode and writes the raw byte array directly to `out.bin`.
