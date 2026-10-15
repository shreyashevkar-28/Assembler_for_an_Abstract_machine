// #define main worklet1_main
// #include "worklet1.cpp"
#undef main

#include "worklet2.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>

// ---------------------------------------------------------------------------
// Adapter: Worklet 1 IR (strings) -> Worklet 2 input (enums + integers)
// ---------------------------------------------------------------------------
namespace {

struct LowerError {
    string message;
    int    line;     // 0 if not tied to a source line
};

bool lookupOp(const string& m, w2::Op& op) {
    if (m == "MOV")  { op = w2::Op::MOV;  return true; }
    if (m == "ADD")  { op = w2::Op::ADD;  return true; }
    if (m == "SUB")  { op = w2::Op::SUB;  return true; }
    if (m == "CMP")  { op = w2::Op::CMP;  return true; }
    if (m == "INC")  { op = w2::Op::INC;  return true; }
    if (m == "DEC")  { op = w2::Op::DEC;  return true; }
    if (m == "PUSH") { op = w2::Op::PUSH; return true; }
    if (m == "POP")  { op = w2::Op::POP;  return true; }
    if (m == "JMP")  { op = w2::Op::JMP;  return true; }
    if (m == "CALL") { op = w2::Op::CALL; return true; }
    if (m == "JE")   { op = w2::Op::JE;   return true; }
    if (m == "JNE")  { op = w2::Op::JNE;  return true; }
    if (m == "NOP")  { op = w2::Op::NOP;  return true; }
    if (m == "RET")  { op = w2::Op::RET;  return true; }
    // ---- added instruction set ----
    if (m == "AND")    { op = w2::Op::AND;    return true; }
    if (m == "OR")     { op = w2::Op::OR;     return true; }
    if (m == "XOR")    { op = w2::Op::XOR;    return true; }
    if (m == "TEST")   { op = w2::Op::TEST;   return true; }
    if (m == "XCHG")   { op = w2::Op::XCHG;   return true; }
    if (m == "IMUL")   { op = w2::Op::IMUL;   return true; }
    if (m == "SHL")    { op = w2::Op::SHL;    return true; }
    if (m == "SHR")    { op = w2::Op::SHR;    return true; }
    if (m == "SAR")    { op = w2::Op::SAR;    return true; }
    if (m == "NOT")    { op = w2::Op::NOT;    return true; }
    if (m == "NEG")    { op = w2::Op::NEG;    return true; }
    if (m == "MUL")    { op = w2::Op::MUL;    return true; }
    if (m == "DIV")    { op = w2::Op::DIV;    return true; }
    if (m == "IDIV")   { op = w2::Op::IDIV;   return true; }
    if (m == "INT")    { op = w2::Op::INT;    return true; }
    if (m == "JL")     { op = w2::Op::JL;     return true; }
    if (m == "JGE")    { op = w2::Op::JGE;    return true; }
    if (m == "JLE")    { op = w2::Op::JLE;    return true; }
    if (m == "JG")     { op = w2::Op::JG;     return true; }
    if (m == "JB")     { op = w2::Op::JB;     return true; }
    if (m == "JAE")    { op = w2::Op::JAE;    return true; }
    if (m == "JBE")    { op = w2::Op::JBE;    return true; }
    if (m == "JA")     { op = w2::Op::JA;     return true; }
    if (m == "JS")     { op = w2::Op::JS;     return true; }
    if (m == "JNS")    { op = w2::Op::JNS;    return true; }
    if (m == "JZ")     { op = w2::Op::JE;     return true; }   // alias of JE
    if (m == "JNZ")    { op = w2::Op::JNE;    return true; }   // alias of JNE
    if (m == "JC")     { op = w2::Op::JB;     return true; }
    if (m == "JNC")    { op = w2::Op::JAE;    return true; }
    if (m == "JNAE")   { op = w2::Op::JB;     return true; }
    if (m == "JNB")    { op = w2::Op::JAE;    return true; }
    if (m == "JNLE")   { op = w2::Op::JG;     return true; }
    if (m == "JNL")    { op = w2::Op::JGE;    return true; }
    if (m == "JNGE")   { op = w2::Op::JL;     return true; }
    if (m == "JNG")    { op = w2::Op::JLE;    return true; }
    if (m == "JNA")    { op = w2::Op::JBE;    return true; }
    if (m == "JNBE")   { op = w2::Op::JA;     return true; }
    if (m == "LEA")    { op = w2::Op::LEA;    return true; }
    if (m == "HLT")    { op = w2::Op::HLT;    return true; }
    if (m == "CLC")    { op = w2::Op::CLC;    return true; }
    if (m == "STC")    { op = w2::Op::STC;    return true; }
    if (m == "CLD")    { op = w2::Op::CLD;    return true; }
    if (m == "STD")    { op = w2::Op::STD;    return true; }
    if (m == "CDQ")    { op = w2::Op::CDQ;    return true; }
    if (m == "LEAVE")  { op = w2::Op::LEAVE;  return true; }
    if (m == "PUSHAD") { op = w2::Op::PUSHAD; return true; }
    if (m == "POPAD")  { op = w2::Op::POPAD;  return true; }
    if (m == "PUSHFD") { op = w2::Op::PUSHFD; return true; }
    if (m == "POPFD")  { op = w2::Op::POPFD;  return true; }
    return false;   // LOAD / STORE are handled separately
}

bool lookupReg(const string& name, w2::Reg& r) {
    if (name == "EAX") { r = w2::Reg::EAX; return true; }
    if (name == "ECX") { r = w2::Reg::ECX; return true; }
    if (name == "EDX") { r = w2::Reg::EDX; return true; }
    if (name == "EBX") { r = w2::Reg::EBX; return true; }
    if (name == "ESP") { r = w2::Reg::ESP; return true; }
    if (name == "EBP") { r = w2::Reg::EBP; return true; }
    if (name == "ESI") { r = w2::Reg::ESI; return true; }
    if (name == "EDI") { r = w2::Reg::EDI; return true; }
    return false;
}


struct Lowering {
    const Worklet1Output&          ir;
    map<unsigned, w2::LabelId>     idByAddr;   // address -> dense label ID
    LowerError                     err;

    explicit Lowering(const Worklet1Output& o) : ir(o) { err.line = 0; }

    bool fail(const string& msg, int line) { err.message = msg; err.line = line; return false; }

    // Worklet 1 label NAME -> Worklet 2 label ID (via the label's address).
    bool labelId(const string& name, int line, w2::LabelId& id) {
        unordered_map<string, Symbol>::const_iterator s = ir.symbolTable.find(name);
        if (s == ir.symbolTable.end()) return fail("undefined symbol '" + name + "'", line);
        id = idByAddr[s->second.address];
        return true;
    }

    // Worklet 1 operand (REG / IMM / SYM / MEM) -> Worklet 2 operand.
    bool operand(const ::Operand& in, int line, bool symIsMemory, w2::Operand& out) {
        if (in.kind == "REG") {
            w2::Reg r;
            if (!lookupReg(in.value, r)) return fail("unknown register '" + in.value + "'", line);
            out = w2::regOp(r);
            return true;
        }
        if (in.kind == "IMM") {
            // Worklet 1 keeps 0..0xFFFFFFFF and -2^31..-1; reinterpret as 32-bit two's complement.
            out = w2::immOp(static_cast<int32_t>(static_cast<uint32_t>(in.number)));
            return true;
        }
        if (in.kind == "SYM") {
            w2::LabelId id;
            if (!labelId(in.value, line, id)) return false;
            out = symIsMemory ? w2::labelMemOp(id) : w2::labelOp(id);
            return true;
        }
        if (in.kind == "MEM") {
            if (!in.symbol.empty() && in.baseReg.empty() && in.indexReg.empty()) {
                w2::LabelId id;
                if (!labelId(in.symbol, line, id)) return false;
                out = w2::labelMemOp(id);
                return true;
            }
            w2::Reg base = w2::Reg::NONE;
            w2::Reg index = w2::Reg::NONE;
            if (!in.baseReg.empty()) {
                if (!lookupReg(in.baseReg, base)) return fail("unknown base register '" + in.baseReg + "'", line);
            }
            if (!in.indexReg.empty()) {
                if (!lookupReg(in.indexReg, index)) return fail("unknown index register '" + in.indexReg + "'", line);
            }
            uint8_t sc = static_cast<uint8_t>(in.scale ? in.scale : 1);
            int32_t disp = static_cast<int32_t>(in.displacement);
            out = w2::memOp(base, index, sc, disp);
            return true;
        }
        return fail("unknown operand kind '" + in.kind + "'", line);
    }

    // LOAD/STORE use a register as an address ([reg]) or a label as an absolute address ([label]).
    bool addressOperand(const ::Operand& in, int line, w2::Operand& out) {
        if (in.kind == "REG") {
            w2::Reg r;
            if (!lookupReg(in.value, r)) return fail("unknown register '" + in.value + "'", line);
            out = w2::memOp(r, w2::Reg::NONE, 1, 0);
            return true;
        }
        return operand(in, line, true, out);
    }

    bool lower(vector<w2::Instruction>& code, w2::LabelId& endLabel) {
        const vector< ::Instruction >& src = ir.instructions;
        endLabel = w2::NO_LABEL;

        // 1. Dense label IDs. All names at one address share one ID, and IDs are
        //    assigned in address order so the result doesn't depend on hash order.
        for (unordered_map<string, Symbol>::const_iterator it = ir.symbolTable.begin();
             it != ir.symbolTable.end(); ++it)
            idByAddr[it->second.address] = 0;
        w2::LabelId next = 0;
        for (map<unsigned, w2::LabelId>::iterator it = idByAddr.begin(); it != idByAddr.end(); ++it)
            it->second = next++;

        // 2. Instructions.
        code.assign(src.size(), w2::Instruction());
        map<unsigned, size_t> instrAt;                      // address -> instruction index
        for (size_t i = 0; i < src.size(); ++i) {
            const ::Instruction& in = src[i];
            instrAt[in.address] = i;

            w2::Instruction& out = code[i];
            out = w2::Instruction();
            out.defLabel = w2::NO_LABEL;
            out.operandCount = static_cast<uint8_t>(in.operands.size());
            if (in.operands.size() > 2) return fail("more than 2 operands", in.line);

            if (in.mnemonic == "LOAD" || in.mnemonic == "STORE") {
                if (in.operands.size() != 2) return fail(in.mnemonic + " needs 2 operands", in.line);
                out.op = w2::Op::MOV;
                const bool isLoad = (in.mnemonic == "LOAD");
                if (isLoad) {      // LOAD reg, [reg|label]  ->  MOV reg, [..]
                    if (!operand(in.operands[0], in.line, false, out.operands[0])) return false;
                    if (!addressOperand(in.operands[1], in.line, out.operands[1])) return false;
                } else {           // STORE [reg|label], reg ->  MOV [..], reg
                    if (!addressOperand(in.operands[0], in.line, out.operands[0])) return false;
                    if (!operand(in.operands[1], in.line, false, out.operands[1])) return false;
                }
                continue;
            }

            if (!lookupOp(in.mnemonic, out.op))
                return fail("unsupported mnemonic '" + in.mnemonic + "'", in.line);
            for (size_t k = 0; k < in.operands.size(); ++k)
                if (!operand(in.operands[k], in.line, false, out.operands[k])) return false;
        }

        // 3. Attach label definitions to the instruction that starts at their address.
        for (map<unsigned, w2::LabelId>::const_iterator it = idByAddr.begin(); it != idByAddr.end(); ++it) {
            if (it->first == ir.finalLC) { endLabel = it->second; continue; }   // end-of-code label
            map<unsigned, size_t>::const_iterator at = instrAt.find(it->first);
            if (at == instrAt.end())
                return fail("label address does not coincide with an instruction start", 0);
            code[at->second].defLabel = it->second;
        }
        return true;
    }
};

// Worklet 1's Pass 1 fixed every address and length up front. If Worklet 2 emits
// something different, every later label/jump would be wrong, so check it.
bool crossCheck(const Worklet1Output& ir, const vector<size_t>& offs, size_t totalBytes) {
    bool ok = true;
    for (size_t i = 0; i < ir.instructions.size(); ++i) {
        const ::Instruction& in = ir.instructions[i];
        const size_t got = offs[i + 1] - offs[i];
        if (offs[i] != in.address || got != static_cast<size_t>(in.length)) {
            std::fprintf(stderr,
                "size mismatch at line %d (%s): Worklet 1 expected %d byte(s) at 0x%X, "
                "Worklet 2 emitted %lu byte(s) at 0x%lX\n",
                in.line, in.mnemonic.c_str(), in.length, in.address,
                static_cast<unsigned long>(got), static_cast<unsigned long>(offs[i]));
            ok = false;
        }
    }
    if (totalBytes != ir.finalLC) {
        std::fprintf(stderr, "total size mismatch: Worklet 1 says %u, Worklet 2 produced %lu\n",
                     ir.finalLC, static_cast<unsigned long>(totalBytes));
        ok = false;
    }
    return ok;
}

void printListing(const Worklet1Output& ir, const vector<size_t>& offs,
                  const vector<uint8_t>& bytes) {
    std::printf("LISTING\n-------------------------\n");
    for (size_t i = 0; i < ir.instructions.size(); ++i) {
        const ::Instruction& in = ir.instructions[i];
        std::string hex;
        char buf[4];
        for (size_t k = offs[i]; k < offs[i + 1]; ++k) {
            std::snprintf(buf, sizeof buf, "%02X ", bytes[k]);
            hex += buf;
        }
        std::string text = in.mnemonic;
        for (size_t k = 0; k < in.operands.size(); ++k)
            text += (k ? ", " : " ") + in.operands[k].value;
        std::printf("%08lX  %-21s %s\n", static_cast<unsigned long>(offs[i]), hex.c_str(), text.c_str());
    }
}

} // namespace

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <file.asm> [out.bin] [baseAddress]\n", argv[0]);
        return 1;
    }
    const char* outPath = (argc > 2) ? argv[2] : "out.bin";
    // Load address added to [label] operands (LOAD/STORE). Default 0 = same as Worklet 1's addresses.
    const uint32_t base = (argc > 3) ? static_cast<uint32_t>(std::strtoul(argv[3], 0, 0)) : 0u;

    // ---- Worklet 1 (unchanged) ----
    vector<string> sourceLines;
    if (!readSourceFile(argv[1], sourceLines)) {
        std::fprintf(stderr, "could not open '%s'\n", argv[1]);
        return 1;
    }
    Worklet1Output ir = runWorklet1(sourceLines);
    if (!ir.ok) {
        printErrors(ir.errors);
        return 1;
    }

    // ---- Adapter: strings -> enums / integer label IDs ----
    Lowering lowering(ir);
    vector<w2::Instruction> code;
    w2::LabelId endLabel;
    if (!lowering.lower(code, endLabel)) {
        std::fprintf(stderr, "adapter error%s%.0d: %s\n",
                     lowering.err.line ? " at line " : "", lowering.err.line,
                     lowering.err.message.c_str());
        return 1;
    }
    const w2::Program prog = w2::makeProgram(code.empty() ? 0 : &code[0], code.size(),
                                             static_cast<uint32_t>(lowering.idByAddr.size()),
                                             base, endLabel);

    // ---- Worklet 2 ----
    vector<uint8_t> bytes;
    vector<size_t>  offs;
    w2::Result r = w2::encode(prog, bytes, &offs);
    if (r.status != w2::Status::OK) {
        const bool atEnd = r.instrIndex >= ir.instructions.size();
        std::fprintf(stderr, "encode failed: %s (%s",
                     w2::statusText(r.status), atEnd ? "end of code" : "line ");
        if (!atEnd) std::fprintf(stderr, "%d", ir.instructions[r.instrIndex].line);
        std::fprintf(stderr, ")\n");
        return 1;
    }
    if (!crossCheck(ir, offs, bytes.size())) return 1;

    // ---- Hand the .bin to Worklet 3 ----
    if (!w2::writeBinFile(outPath, bytes)) {
        std::fprintf(stderr, "could not write %s\n", outPath);
        return 1;
    }
    printListing(ir, offs, bytes);
    std::printf("\nwrote %lu bytes to %s\n", static_cast<unsigned long>(bytes.size()), outPath);
    return 0;
}
