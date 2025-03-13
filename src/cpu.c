#include "../include/cpu.h"
#include <assert.h>

CPU cpu;

static byte cycles[0x100];
static byte pc_inc[0x100];
static const byte extended_cycles[0x100];
static const byte extended_pc_inc[0x100];

#define DO_NOTHING -1
#define RESET 0
#define SET 1

static void execute(byte opcode);
static void extended_execute(byte opcode);

/* Maybe make macro? - probably doesnt matter */
static inline void SET_FLAGS(int z, int n, int h, int c)
{
    if (z == RESET) F &= ~(FLAG_Z);
    else if (z > 0) F |= FLAG_Z;

    if (n == RESET) F &= ~(FLAG_N);
    else if (n > 0) F |= FLAG_N;

    if (h == RESET) F &= ~(FLAG_H); 
    else if (h > 0) F |= FLAG_H;

    if (c == RESET) F &= ~(FLAG_C);
    else if (c > 0) F |= FLAG_C;
}

void cpu_init()
{
    AF = 0x01B0;
    BC = 0x0013;
    DE = 0x00D8;
    HL = 0x014D;
    PC = 0x0100;
    SP = 0xFFFE; 
    IME = 0;
    cpu.cycles = 0;
    cpu.is_halted = 0;
    cpu.halt_bug = 0;
}

byte cpu_cycle() 
{
    //Perhaps we cycle every T-cycle rather than by instruction?

    byte cur_instr = mem_read(PC);
    execute(cur_instr);

    return 4;
}

void set_cpu_registers(byte a, byte b, byte c, byte d, byte e, byte f, byte h, byte l, word pc, word sp)
{
    A = a;
    B = b;
    C = c;
    D = d;
    E = e;
    F = f;
    H = h;
    L = l;
    PC = pc;
    SP = sp;
}

int check_cpu_registers(byte a, byte b, byte c, byte d, byte e, byte f, byte h, byte l, word pc, word sp)
{
    if((A == a) &&
    (B == b) &&
    (C == c) &&
    (D == d) &&
    (E == e) &&
    (F == f) &&
    (H == h) &&
    (L == l) &&
    (PC == pc) &&
    (SP == sp)
    ) {
        return true;
    }
    return false;
}

void print_registers() 
{
    printf("A:%2X F:%2X B:%2X C:%2X D:%2X E:%2X H:%2X L:%2X SP:%4X PC:%4X PCMEM:%2X,%2X,%2X,%2X\n", 
        A, F, B, C, D, E, H, L, SP, PC, mem_read(PC), mem_read(PC+1), mem_read(PC+2), mem_read(PC+3));
}

static inline void LD(byte *dst, byte val){
    *dst = val;
}

static inline void LD16(word *dst, word val){
    *dst = val;
}

static inline void LDA8(){
    word addr = mem_read16(PC+1);
    byte low = SP & 0x00FF;
    byte high = SP >> 8;
    mem_write(addr, low); addr++;
    mem_write(addr, high);
}

static inline void INC(byte *dst){
    const byte dst_val = *dst;
    *dst += 1;
    SET_FLAGS(*dst == 0, 
            RESET, 
            ((dst_val & 0x0F) + (1 & 0x0F)) > 0x0F, 
            DO_NOTHING 
    ); 
}

static inline void LDHL(){
    const int8_t val = (int8_t)mem_read(PC + 1);
    HL = SP + val;
    SET_FLAGS(RESET, 
            RESET, 
            ((SP & 0x0F) + (val & 0x0F)) > 0x0F, 
            ((SP & 0xFF) + (val & 0xFF)) > 0xFF 
    ); 
}

static inline void ADDSP(){
    //imediate data needed to be signed, hence int8_t instead of byte
    const int8_t val = (int8_t)mem_read(PC + 1);
    SET_FLAGS(RESET, 
            RESET, 
            ((SP & 0x0F) + (val & 0x0F)) > 0x0F, 
            ((SP & 0xFF) + (val & 0xFF)) > 0xFF 
    );
    SP += val;
}

static inline void INC_MEM(word address){
    const byte old_mem_val = mem_read(address);
    mem_write(address, old_mem_val + 1);
    SET_FLAGS(mem_read(address) == 0, 
            RESET, 
            ((old_mem_val & 0x0F) + (1 & 0x0F)) > 0x0F, 
            DO_NOTHING 
    ); 
}

static inline void DEC(byte *dst){
    const byte dst_val = *dst;
    *dst -= 1;
    SET_FLAGS(*dst == 0, 
            SET, 
            !(dst_val & 0x0F), 
            DO_NOTHING 
    ); 
}

static inline void DEC_MEM(word address){
    const byte old_mem_val = mem_read(address);
    mem_write(address, old_mem_val - 1);
    SET_FLAGS(mem_read(address) == 0, 
            SET, 
            !(old_mem_val & 0x0F), 
            DO_NOTHING 
    ); 
}

static inline void RLA(){ 
    const byte carry_val = F & FLAG_C;
    const byte will_carry = A & ( 1 << 7 );
    A <<= 1;
    if(carry_val) A += 1;
    SET_FLAGS(RESET, 
            RESET, 
            RESET, 
            will_carry 
    ); 
}

static inline void RRA(){
    const byte carry_val = F & FLAG_C;
    const byte will_carry = A & 0x01;
    A >>= 1;
    if(carry_val) A |= (1<<7);
    SET_FLAGS(RESET, 
            RESET, 
            RESET, 
            will_carry 
    ); 
}

static inline void RLCA(){
    const byte will_carry = A & ( 1 << 7 );
    A <<= 1;
    if(will_carry) A += 1;
    SET_FLAGS(RESET, 
            RESET, 
            RESET, 
            will_carry 
    ); 
}

static inline void RRCA(){
    const byte will_carry = A & 0x01;
    A >>= 1;
    if(will_carry) A |= (1<<7);
    SET_FLAGS(RESET, 
            RESET, 
            RESET, 
            will_carry 
    ); 
}

static inline void JRN(byte flag, byte opcode){ //jump if not set
    const byte flag_set = F & flag;
    if(!flag_set){
        PC += (int8_t)mem_read(PC+1); //need signed data here     
        cycles[opcode] = 12; 
    }else{
        cycles[opcode] = 8;
    }
}

static inline void JPN(byte flag, byte opcode){ //jump if not set
    const byte flag_set = F & flag;
    if(!flag_set){
        byte low = mem_read(PC + 1);
        byte high = mem_read(PC + 2);
        PC = low | (high << 8); 
        pc_inc[opcode] = 0;
        cycles[opcode] = 16; 
    }else{ 
        pc_inc[opcode] = 3;
        cycles[opcode] = 12;
    }
}

static inline void JRS(byte flag, byte opcode){ //jump if set
    const byte flag_set = F & flag;
    if(flag_set){
        PC+= (int8_t)mem_read(PC+1); //need signed data here      
        cycles[opcode] = 12; 
    }else{
        cycles[opcode] = 8;
    }
}

static inline void JPS(byte flag, byte opcode){ //jump if set
    const byte flag_set = F & flag;
    if(flag_set){
        byte low = mem_read(PC + 1);
        byte high = mem_read(PC + 2);
        PC = low | (high << 8); 
        pc_inc[opcode] = 0;
        cycles[opcode] = 16; 
    }else{
        pc_inc[opcode] = 3;
        cycles[opcode] = 12;
    }
}

static inline void JP(){ 
    byte low = mem_read(PC + 1);
    byte high = mem_read(PC + 2);
    PC = low | (high << 8);
}

static inline void JR(){
    PC+=(int8_t)mem_read(PC+1);
}

//https://blog.ollien.com/posts/gb-daa/ 
static inline void DAA(){
    byte should_carry = 0;
    const byte half_carry = F & FLAG_H ? 1 : 0;
    const byte carry = F & FLAG_C ? 1 : 0;
    const byte subtract = F & FLAG_N ? 1 : 0;
    
    const byte dst_val = A;
    byte offset = 0;
    if((subtract == 0 && ((dst_val & 0x0F) > 0x09)) || half_carry == 1){
        offset |= 0x06;
    }

    if((subtract == 0 &&  dst_val > 0x99) || carry == 1){
        offset |= 0x60;
        should_carry = 1;
    }

    A = subtract ? dst_val - offset : dst_val + offset;
    SET_FLAGS(A == 0, 
            DO_NOTHING, 
            RESET, 
            (dst_val > 0x99 && subtract == 0) || should_carry  
    ); 
}

static inline void SCF(){
    SET_FLAGS(DO_NOTHING, 
            RESET, 
            RESET, 
            SET  
    );    
}

static inline void CPL(){
    A = ~A;
    SET_FLAGS(DO_NOTHING, 
            SET, 
            SET, 
            DO_NOTHING  
    );  
}

static inline void CCF(){
    const byte carry_set = F & FLAG_C;
    SET_FLAGS(DO_NOTHING, 
            RESET, 
            RESET, 
            !carry_set  
    );  
}

static inline void CALLN(byte flag, byte opcode){
    if(!(F & flag)){
        word address = mem_read(PC + 1) | (mem_read(PC + 2) << 8);
        SP--; mem_write(SP, (PC+3) >> 8);
        SP--; mem_write(SP, (PC+3) & 0x00FF);
        PC = address;
        pc_inc[opcode] = 0;
        cycles[opcode] = 24;
    }else{
        pc_inc[opcode] = 3;
        cycles[opcode] = 12;
    }
}

static inline void CALLS(byte flag, byte opcode){
    if(F & flag){
        word address = mem_read(PC + 1) | (mem_read(PC + 2) << 8);
        SP--; mem_write(SP, (PC+3) >> 8);
        SP--; mem_write(SP, (PC+3) & 0x00FF);
        PC = address;
        pc_inc[opcode] = 0;
        cycles[opcode] = 24;
    }else{ 
        pc_inc[opcode] = 3;
        cycles[opcode] = 12;
    }
}

static inline void CALL(){
    word address = mem_read(PC + 1) | (mem_read(PC + 2) << 8);
    SP--; mem_write(SP, (PC+3) >> 8);
    SP--; mem_write(SP, (PC+3) & 0x00FF);
    PC = address;
}

static inline void RET(){
    const byte low = mem_read(SP); SP++;
    const byte high = mem_read(SP); SP++;
    PC = low | (high << 8); 
}

static inline void RETN(byte flag, byte opcode){
    if(!(F & flag)){
        RET();
        pc_inc[opcode] = 0;
    }else{
        pc_inc[opcode] = 1;
    }
}

static inline void RETS(byte flag, byte opcode){
    if(F & flag){
        RET();
        pc_inc[opcode] = 0;
        cycles[opcode] = 20;
    }else{
        pc_inc[opcode] = 1;
        cycles[opcode] = 8;
    }
}

static inline void POP(word *dst){
    const byte low = mem_read(SP); SP++;
    const byte high = mem_read(SP); SP++;
    *dst = low | (high << 8);
    F &= ~0x0F;
}

extern void PUSH(word *dst){
    SP--; mem_write(SP, (*dst >> 8));
    SP--; mem_write(SP, *dst & 0x00FF);
}

static inline void RST(byte val){
    PC++; SP--;
    mem_write(SP, PC >> 8); SP--;
    mem_write(SP, PC & 0x00FF); 
    PC = val | (0x00 << 8); 
}

static inline void HALT(){
    cpu.is_halted = 1;
}

static inline void ADD(byte* dst, byte val) {
    const byte dst_val = *dst;
    const word result = dst_val + val;
    *dst = (byte)result;
    SET_FLAGS(*dst == 0, 
            RESET, 
            ((dst_val & 0x0F) + (val & 0x0F)) > 0x0F, 
            result > 0xFF
    ); 
}

static inline void ADD16(word* dst, word val) {
    const word dst_val = *dst;
    const int result = dst_val + val;
    *dst = (word)result;
    SET_FLAGS(DO_NOTHING, 
            RESET, 
            ((dst_val & 0x0FFF) + (val & 0x0FFF)) > 0x0FFF, 
            result > 0xFFFF
    ); 
}

static inline void ADC(byte *dst, byte val){
    const byte carry_set = (F & FLAG_C) ? 1 : 0;
    const byte dst_val = *dst;
    word result = dst_val + val + carry_set;
    *dst = (byte)result;
    SET_FLAGS(*dst == 0, 
            RESET, 
            ((dst_val & 0x0F) + (val & 0x0F) + carry_set) > 0x0F, 
            result > 0xFF
    ); 
}

static inline void SUB(byte *dst, byte val){
    const byte dst_val = *dst;
    const word result = dst_val - val;
    *dst = (byte)result;
    SET_FLAGS(*dst == 0, 
            SET, 
            (dst_val & 0x0F) < (val & 0x0F), 
            result > 0xFF
    ); 
}

static inline void SBC(byte *dst, byte val){ 
    const byte carry_set = (F & FLAG_C) ? 1 : 0;
    const byte dst_val = *dst;
    const word result = dst_val - val - carry_set;
    *dst = (byte)result;
    SET_FLAGS(*dst == 0, 
            SET, 
            (dst_val & 0x0F) < (val & 0x0F) + carry_set, 
            result > 0xFF
    ); 
}

static inline void AND(byte *dst, byte val){
    *dst &= val;
    SET_FLAGS(*dst == 0, RESET, SET, RESET);
}

static inline void XOR(byte *dst, byte val){
    *dst ^= val;
    SET_FLAGS(*dst == 0, RESET, RESET, RESET);
}

static inline void OR(byte *dst, byte val){
    *dst |= val;
    SET_FLAGS(*dst == 0, RESET, RESET, RESET);
}

static inline void CP(byte *dst, byte val){
    const word result = *dst - val;
    SET_FLAGS(result == 0, 
            SET, 
            (*dst & 0x0F) < (val & 0x0F), 
            result > 0xFF
    ); 
}

//the following instructions are only used in the extended instruction set.
static inline void RLC(byte *reg){
    const byte will_carry = *reg & (1<<7);
    *reg <<= 1;
    if(will_carry) *reg += 1;
    SET_FLAGS(*reg == 0, 
            RESET, 
            RESET, 
            will_carry 
    ); 
}

static inline void RLCMEM(){
    byte val = mem_read(HL);
    RLC(&val);
    mem_write(HL, val);
}

static inline void RRC(byte *reg){
    const byte will_carry = *reg & 0x01;
    *reg >>= 1;
    if(will_carry) *reg |= (1<<7);
    SET_FLAGS(*reg == 0, 
            RESET, 
            RESET, 
            will_carry
    ); 
}

static inline void RRCMEM(){
    byte val = mem_read(HL);
    RRC(&val);
    mem_write(HL, val);
}

static inline void RL(byte *reg){
    const int will_carry = *reg & (1<<7);
    const int is_carry = F & FLAG_C;
    *reg <<= 1;
    if(is_carry) *reg += 1;
    SET_FLAGS(*reg == 0, 
            RESET, 
            RESET, 
            will_carry
    ); 
}

static inline void RLMEM(){
    byte val = mem_read(HL);
    RL(&val);
    mem_write(HL, val);
}

static inline void RR(byte *reg){
    const int will_carry = *reg & (0x01);
    const int is_carry = F & FLAG_C;
    *reg >>= 1;
    if(is_carry) *reg |= (1<<7);
    SET_FLAGS(*reg == 0, 
            RESET, 
            RESET, 
            will_carry
    ); 
}

static inline void RRMEM(){
    byte val = mem_read(HL);
    RR(&val);
    mem_write(HL, val);
}

static inline void SLA(byte *reg){
    const int will_carry = *reg & (1<<7);
    *reg <<= 1;
    SET_FLAGS(*reg == 0, 
            RESET, 
            RESET, 
            will_carry
    ); 
}

static inline void SLAMEM(){
    byte val = mem_read(HL);
    SLA(&val);
    mem_write(HL, val);
}

static inline void SRA(byte *reg){
    const int will_carry = *reg & 0x01;
    const int high_order = *reg & (1<<7);
    *reg >>= 1;
    if(high_order) *reg |= (1<<7);
    SET_FLAGS(*reg == 0, 
            RESET, 
            RESET, 
            will_carry
    ); 
}

static inline void SRAMEM(){
    byte val = mem_read(HL);
    SRA(&val);
    mem_write(HL, val);
}

static inline void SWAP(byte *reg){
    *reg = *reg << 4 | *reg >> 4;
    SET_FLAGS(*reg == 0, 
            RESET, 
            RESET, 
            RESET
    );  
}

static inline void SWAPMEM(){
    byte val = mem_read(HL);
    SWAP(&val);
    mem_write(HL, val);
}

static inline void SRL(byte *reg){
    const int will_carry = *reg & (0x01);
    *reg >>= 1;
    SET_FLAGS(*reg == 0, 
            RESET, 
            RESET, 
            will_carry 
    ); 
}

static inline void SRLMEM(){
    byte val = mem_read(HL);
    SRL(&val);
    mem_write(HL, val);
}

static inline void BIT(byte *reg, byte bit){
    const int is_set = *reg & (1<<bit);
    SET_FLAGS(!is_set, 
            RESET, 
            SET, 
            DO_NOTHING 
    ); 
}

static inline void BITMEM(byte bit){
    byte val = mem_read(HL);
    BIT(&val, bit);
}

static inline void _SET(byte *reg, byte bit){
    *reg |= (1<<bit);
}

static inline void _SETMEM(byte bit){
    byte val = mem_read(HL);
    _SET(&val, bit);
    mem_write(HL, val);
}

static inline void RES(byte *reg, byte bit){
    *reg &= ~(1<<bit);
}

static inline void RESMEM(byte bit){
    byte val = mem_read(HL);
    RES(&val, bit);
    mem_write(HL, val);
}

static void execute(byte opcode){
    switch(opcode){
        case 0x00: break; //NOP
        case 0x01: LD16(&BC, mem_read16(PC+1)); break;
        case 0x02: mem_write(BC, A); break;
        case 0x03: BC++; break;
        case 0x04: INC(&B); break;
        case 0x05: DEC(&B); break;
        case 0x06: LD(&B, mem_read(PC+1)); break;
        case 0x07: RLCA(); break;
        case 0x08: LDA8(); break;
        case 0x09: ADD16(&HL, BC); break;
        case 0x0A: LD(&A, mem_read(BC)); break;
        case 0x0B: BC--; break;
        case 0x0C: INC(&C); break;
        case 0x0D: DEC(&C); break;
        case 0x0E: LD(&C, mem_read(PC+1)); break;
        case 0x0F: RRCA(); break;

        case 0x10: break; //STOP
        case 0x11: LD16(&DE, mem_read16(PC+1)); break;
        case 0x12: mem_write(DE, A); break;
        case 0x13: DE++; break;
        case 0x14: INC(&D); break;
        case 0x15: DEC(&D); break;
        case 0x16: LD(&D, mem_read(PC+1)); break;
        case 0x17: RLA(); break;
        case 0x18: JR(); break;
        case 0x19: ADD16(&HL, DE); break;
        case 0x1A: LD(&A, mem_read(DE));break;
        case 0x1B: DE--; break;
        case 0x1C: INC(&E); break;
        case 0x1D: DEC(&E); break;
        case 0x1E: LD(&E, mem_read(PC+1)); break;
        case 0x1F: RRA(); break;

        case 0x20: JRN(FLAG_Z, 0x20); break;
        case 0x21: LD16(&HL, mem_read16(PC+1)); break;
        case 0x22: mem_write(HL, A); HL++; break;
        case 0x23: HL++; break;
        case 0x24: INC(&H); break;
        case 0x25: DEC(&H); break;
        case 0x26: LD(&H, mem_read(PC+1)); break;
        case 0x27: DAA(); break;
        case 0x28: JRS(FLAG_Z, 0x28); break;
        case 0x29: ADD16(&HL, HL); break;
        case 0x2A: LD(&A, mem_read(HL)); HL++; break;
        case 0x2B: HL--; break;
        case 0x2C: INC(&L); break;
        case 0x2D: DEC(&L); break;
        case 0x2E: LD(&L, mem_read(PC+1)); break;
        case 0x2F: CPL(); break;

        case 0x30: JRN(FLAG_C, 0x30); break;
        case 0x31: LD16(&SP, mem_read16(PC+1)); break;
        case 0x32: mem_write(HL, A); HL--; break;
        case 0x33: SP++; break;
        case 0x34: INC_MEM(HL); break;
        case 0x35: DEC_MEM(HL); break;
        case 0x36: mem_write(HL, mem_read(PC+1)); break;
        case 0x37: SCF(); break;
        case 0x38: JRS(FLAG_C, 0x38); break;
        case 0x39: ADD16(&HL, SP); break;
        case 0x3A: LD(&A, mem_read(HL)); HL--; break;
        case 0x3B: SP--; break;
        case 0x3C: INC(&A); break;
        case 0x3D: DEC(&A); break;
        case 0x3E: LD(&A, mem_read(PC+1)); break;
        case 0x3F: CCF(); break;

        case 0x40: LD(&B, B); break;
        case 0x41: LD(&B, C); break;
        case 0x42: LD(&B, D); break;
        case 0x43: LD(&B, E); break;
        case 0x44: LD(&B, H); break;
        case 0x45: LD(&B, L); break;
        case 0x46: LD(&B, mem_read(HL)); break;
        case 0x47: LD(&B, A); break;
        case 0x48: LD(&C, B); break;
        case 0x49: LD(&C, C); break;
        case 0x4A: LD(&C, D); break;
        case 0x4B: LD(&C, E); break;
        case 0x4C: LD(&C, H); break;
        case 0x4D: LD(&C, L); break;
        case 0x4E: LD(&C, mem_read(HL)); break;
        case 0x4F: LD(&C, A); break;

        case 0x50: LD(&D, B); break;
        case 0x51: LD(&D, C); break;
        case 0x52: LD(&D, D); break;
        case 0x53: LD(&D, E); break;
        case 0x54: LD(&D, H); break;
        case 0x55: LD(&D, L); break;
        case 0x56: LD(&D, mem_read(HL)); break;
        case 0x57: LD(&D, A); break;
        case 0x58: LD(&E, B); break;
        case 0x59: LD(&E, C); break;
        case 0x5A: LD(&E, D); break;
        case 0x5B: LD(&E, E); break;
        case 0x5C: LD(&E, H); break;
        case 0x5D: LD(&E, L); break;
        case 0x5E: LD(&E, mem_read(HL)); break;
        case 0x5F: LD(&E, A); break;

        case 0x60: LD(&H, B); break;
        case 0x61: LD(&H, C); break;
        case 0x62: LD(&H, D); break;
        case 0x63: LD(&H, E); break;
        case 0x64: LD(&H, H); break;
        case 0x65: LD(&H, L); break;
        case 0x66: LD(&H, mem_read(HL)); break;
        case 0x67: LD(&H, A); break;
        case 0x68: LD(&L, B); break;
        case 0x69: LD(&L, C); break;
        case 0x6A: LD(&L, D); break;
        case 0x6B: LD(&L, E); break;
        case 0x6C: LD(&L, H); break;
        case 0x6D: LD(&L, L); break;
        case 0x6E: LD(&L, mem_read(HL)); break;
        case 0x6F: LD(&L, A); break;

        case 0x70: mem_write(HL, B); break;
        case 0x71: mem_write(HL, C); break;
        case 0x72: mem_write(HL, D); break;
        case 0x73: mem_write(HL, E); break;
        case 0x74: mem_write(HL, H); break;
        case 0x75: mem_write(HL, L); break;
        case 0x76: HALT(); break;
        case 0x77: mem_write(HL, A); break;
        case 0x78: LD(&A, B); break;
        case 0x79: LD(&A, C); break;
        case 0x7A: LD(&A, D); break;
        case 0x7B: LD(&A, E); break;
        case 0x7C: LD(&A, H); break;
        case 0x7D: LD(&A, L); break;
        case 0x7E: LD(&A, mem_read(HL)); break;
        case 0x7F: LD(&A, A); break;
        
        case 0x80: ADD(&A, B); break;
        case 0x81: ADD(&A, C); break;
        case 0x82: ADD(&A, D); break;
        case 0x83: ADD(&A, E); break;
        case 0x84: ADD(&A, H); break;
        case 0x85: ADD(&A, L); break;
        case 0x86: ADD(&A, mem_read(HL)); break;
        case 0x87: ADD(&A, A); break;
        case 0x88: ADC(&A, B); break;
        case 0x89: ADC(&A, C); break;
        case 0x8A: ADC(&A, D); break;
        case 0x8B: ADC(&A, E); break;
        case 0x8C: ADC(&A, H); break;
        case 0x8D: ADC(&A, L); break;
        case 0x8E: ADC(&A, mem_read(HL)); break;
        case 0x8F: ADC(&A, A); break;

        case 0x90: SUB(&A, B); break;
        case 0x91: SUB(&A, C); break;
        case 0x92: SUB(&A, D); break;
        case 0x93: SUB(&A, E); break;
        case 0x94: SUB(&A, H); break;
        case 0x95: SUB(&A, L); break;
        case 0x96: SUB(&A, mem_read(HL)); break;
        case 0x97: SUB(&A, A); break;
        case 0x98: SBC(&A, B); break;
        case 0x99: SBC(&A, C); break;
        case 0x9A: SBC(&A, D); break;
        case 0x9B: SBC(&A, E); break;
        case 0x9C: SBC(&A, H); break;
        case 0x9D: SBC(&A, L); break;
        case 0x9E: SBC(&A, mem_read(HL)); break;
        case 0x9F: SBC(&A, A); break;

        case 0xA0: AND(&A, B); break;
        case 0xA1: AND(&A, C); break;
        case 0xA2: AND(&A, D); break;
        case 0xA3: AND(&A, E); break;
        case 0xA4: AND(&A, H); break;
        case 0xA5: AND(&A, L); break;
        case 0xA6: AND(&A, mem_read(HL)); break;
        case 0xA7: AND(&A, A); break;
        case 0xA8: XOR(&A, B); break;
        case 0xA9: XOR(&A, C); break;
        case 0xAA: XOR(&A, D); break;
        case 0xAB: XOR(&A, E); break;
        case 0xAC: XOR(&A, H); break;
        case 0xAD: XOR(&A, L); break;
        case 0xAE: XOR(&A, mem_read(HL)); break;
        case 0xAF: XOR(&A, A); break;

        case 0xB0: OR(&A, B); break;
        case 0xB1: OR(&A, C); break;
        case 0xB2: OR(&A, D); break;
        case 0xB3: OR(&A, E); break;
        case 0xB4: OR(&A, H); break;
        case 0xB5: OR(&A, L); break;
        case 0xB6: OR(&A, mem_read(HL)); break;
        case 0xB7: OR(&A, A); break;
        case 0xB8: CP(&A, B); break;
        case 0xB9: CP(&A, C); break;
        case 0xBA: CP(&A, D); break;
        case 0xBB: CP(&A, E); break;
        case 0xBC: CP(&A, H); break;
        case 0xBD: CP(&A, L); break;
        case 0xBE: CP(&A, mem_read(HL)); break;
        case 0xBF: CP(&A, A); break;

        case 0xC0: RETN(FLAG_Z, 0xC0); break;
        case 0xC1: POP(&BC); break;
        case 0xC2: JPN(FLAG_Z, 0xC2); break;
        case 0xC3: JP(); break;
        case 0xC4: CALLN(FLAG_Z, 0xC4); break;
        case 0xC5: PUSH(&BC); break;
        case 0xC6: ADD(&A, mem_read(PC + 1)); break;
        case 0xC7: RST(0x00); break;
        case 0xC8: RETS(FLAG_Z, 0xC8); break;
        case 0xC9: RET(); break;
        case 0xCA: JPS(FLAG_Z, 0xCA); break;
        case 0xCB: extended_execute(mem_read(PC+1)); break; //extended instrs
        case 0xCC: CALLS(FLAG_Z, 0xCC); break;
        case 0xCD: CALL(); break;
        case 0xCE: ADC(&A, mem_read(PC + 1)); break;
        case 0xCF: RST(0x08); break;

        case 0xD0: RETN(FLAG_C, 0xD0); break;
        case 0xD1: POP(&DE); break;
        case 0xD2: JPN(FLAG_C, 0xD2); break;
        case 0xD3: break;
        case 0xD4: CALLN(FLAG_C, 0xD4); break;
        case 0xD5: PUSH(&DE); break;
        case 0xD6: SUB(&A, mem_read(PC + 1)); break;
        case 0xD7: RST(0x10); break;
        case 0xD8: RETS(FLAG_C, 0xD8); break;
        case 0xD9: RET(); IME = 1; break;
        case 0xDA: JPS(FLAG_C, 0xDA); break;
        case 0xDB: break;
        case 0xDC: CALLS(FLAG_C, 0xDC); break;
        case 0xDD: break;
        case 0xDE: SBC(&A, mem_read(PC + 1)); break;
        case 0xDF: RST(0x18); break;

        case 0xE0: mem_write(0xFF00 + mem_read(PC + 1), A); break;
        case 0xE1: POP(&HL); break;
        case 0xE2: mem_write(0xFF00 + C, A); break;
        case 0xE3: break;
        case 0xE4: break;
        case 0xE5: PUSH(&HL); break;
        case 0xE6: AND(&A, mem_read(PC + 1)); break;
        case 0xE7: RST(0x20); break;
        case 0xE8: ADDSP(); break;
        case 0xE9: PC = HL; break;
        case 0xEA: mem_write(mem_read(PC + 1) | (mem_read(PC + 2) << 8), A); break;
        case 0xEB: break;
        case 0xEC: break;
        case 0xED: break;
        case 0xEE: XOR(&A, mem_read(PC + 1)); break;
        case 0xEF: RST(0x28); break;

        case 0xF0: LD(&A, mem_read(0xFF00 + mem_read(PC + 1))); break;
        case 0xF1: POP(&AF); break;
        case 0xF2: LD(&A, mem_read(0xFF00 + C));break;
        case 0xF3: IME = 0; break;
        case 0xF4: break;
        case 0xF5: PUSH(&AF); break;
        case 0xF6: OR(&A, mem_read(PC + 1)); break;
        case 0xF7: RST(0x30); break;
        case 0xF8: LDHL(); break;
        case 0xF9: SP = HL; break;
        case 0xFA: LD(&A, mem_read(mem_read(PC+1) | (mem_read(PC+2)<<8))); break;
        case 0xFB: IME = 1; break;
        case 0xFC: break;
        case 0xFD: break;
        case 0xFE: CP(&A, mem_read(PC + 1)); break;
        case 0xFF: RST(0x38); break;

        default: break;
    }

    if(opcode == 0xCB){
        PC += extended_pc_inc[opcode];
        cpu.cycles = extended_cycles[opcode];   
    }else{
        PC += pc_inc[opcode];
        cpu.cycles = cycles[opcode];
    }
}


static void extended_execute(byte opcode) {
    switch (opcode) {
        case 0x00: RLC(&B); break;
        case 0x01: RLC(&C); break;
        case 0x02: RLC(&D); break;
        case 0x03: RLC(&E); break;
        case 0x04: RLC(&H); break;
        case 0x05: RLC(&L); break;
        case 0x06: RLCMEM(); break;
        case 0x07: RLC(&A); break;
        case 0x08: RRC(&B);break;
        case 0x09: RRC(&C);break;
        case 0x0A: RRC(&D);break;
        case 0x0B: RRC(&E);break;
        case 0x0C: RRC(&H);break;
        case 0x0D: RRC(&L);break;
        case 0x0E: RRCMEM();break;
        case 0x0F: RRC(&A);break;

        case 0x10: RL(&B); break;
        case 0x11: RL(&C); break;
        case 0x12: RL(&D); break;
        case 0x13: RL(&E); break;
        case 0x14: RL(&H); break;
        case 0x15: RL(&L); break;
        case 0x16: RLMEM(); break;
        case 0x17: RL(&A); break;
        case 0x18: RR(&B); break;
        case 0x19: RR(&C); break;
        case 0x1A: RR(&D); break;
        case 0x1B: RR(&E); break;
        case 0x1C: RR(&H); break;
        case 0x1D: RR(&L); break;
        case 0x1E: RRMEM(); break;
        case 0x1F: RR(&A); break;

        case 0x20: SLA(&B); break;
        case 0x21: SLA(&C); break;
        case 0x22: SLA(&D); break;
        case 0x23: SLA(&E); break;
        case 0x24: SLA(&H); break;
        case 0x25: SLA(&L); break;
        case 0x26: SLAMEM(); break;
        case 0x27: SLA(&A); break;
        case 0x28: SRA(&B); break;
        case 0x29: SRA(&C); break;
        case 0x2A: SRA(&D); break;
        case 0x2B: SRA(&E); break;
        case 0x2C: SRA(&H); break;
        case 0x2D: SRA(&L); break;
        case 0x2E: SRAMEM(); break;
        case 0x2F: SRA(&A); break;

        case 0x30: SWAP(&B); break;
        case 0x31: SWAP(&C); break;
        case 0x32: SWAP(&D); break;
        case 0x33: SWAP(&E); break;
        case 0x34: SWAP(&H); break;
        case 0x35: SWAP(&L); break;
        case 0x36: SWAPMEM(); break;
        case 0x37: SWAP(&A); break;
        case 0x38: SRL(&B); break;
        case 0x39: SRL(&C); break;
        case 0x3A: SRL(&D); break;
        case 0x3B: SRL(&E); break;
        case 0x3C: SRL(&H); break;
        case 0x3D: SRL(&L); break;
        case 0x3E: SRLMEM(); break;
        case 0x3F: SRL(&A); break;

        case 0x40: BIT(&B, 0); break;
        case 0x41: BIT(&C, 0); break;
        case 0x42: BIT(&D, 0); break;
        case 0x43: BIT(&E, 0); break;
        case 0x44: BIT(&H, 0); break;
        case 0x45: BIT(&L, 0); break;
        case 0x46: BITMEM(0); break;
        case 0x47: BIT(&A, 0); break;

        case 0x48: BIT(&B, 1); break;
        case 0x49: BIT(&C, 1); break;
        case 0x4A: BIT(&D, 1); break;
        case 0x4B: BIT(&E, 1); break;
        case 0x4C: BIT(&H, 1); break;
        case 0x4D: BIT(&L, 1); break;
        case 0x4E: BITMEM(1); break;
        case 0x4F: BIT(&A, 1); break;

        case 0x50: BIT(&B, 2); break;
        case 0x51: BIT(&C, 2); break;
        case 0x52: BIT(&D, 2); break;
        case 0x53: BIT(&E, 2); break;
        case 0x54: BIT(&H, 2); break;
        case 0x55: BIT(&L, 2); break;
        case 0x56: BITMEM(2); break;
        case 0x57: BIT(&A, 2); break;

        case 0x58: BIT(&B, 3); break;
        case 0x59: BIT(&C, 3); break;
        case 0x5A: BIT(&D, 3); break;
        case 0x5B: BIT(&E, 3); break;
        case 0x5C: BIT(&H, 3); break;
        case 0x5D: BIT(&L, 3); break;
        case 0x5E: BITMEM(3); break;
        case 0x5F: BIT(&A, 3); break;

        case 0x60: BIT(&B, 4); break;
        case 0x61: BIT(&C, 4); break;
        case 0x62: BIT(&D, 4); break;
        case 0x63: BIT(&E, 4); break;
        case 0x64: BIT(&H, 4); break;
        case 0x65: BIT(&L, 4); break;
        case 0x66: BITMEM(4); break;
        case 0x67: BIT(&A, 4); break;

        case 0x68: BIT(&B, 5); break;
        case 0x69: BIT(&C, 5); break;
        case 0x6A: BIT(&D, 5); break;
        case 0x6B: BIT(&E, 5); break;
        case 0x6C: BIT(&H, 5); break;
        case 0x6D: BIT(&L, 5); break;
        case 0x6E: BITMEM(5); break;
        case 0x6F: BIT(&A, 5); break;

        case 0x70: BIT(&B, 6); break;
        case 0x71: BIT(&C, 6); break;
        case 0x72: BIT(&D, 6); break;
        case 0x73: BIT(&E, 6); break;
        case 0x74: BIT(&H, 6); break;
        case 0x75: BIT(&L, 6); break;
        case 0x76: BITMEM(6); break;
        case 0x77: BIT(&A, 6); break;

        case 0x78: BIT(&B, 7); break;
        case 0x79: BIT(&C, 7); break;
        case 0x7A: BIT(&D, 7); break;
        case 0x7B: BIT(&E, 7); break;
        case 0x7C: BIT(&H, 7); break;
        case 0x7D: BIT(&L, 7); break;
        case 0x7E: BITMEM(7); break;
        case 0x7F: BIT(&A, 7); break;

        case 0x80: RES(&B, 0); break;
        case 0x81: RES(&C, 0); break;
        case 0x82: RES(&D, 0); break;
        case 0x83: RES(&E, 0); break;
        case 0x84: RES(&H, 0); break;
        case 0x85: RES(&L, 0); break;
        case 0x86: RESMEM(0); break;
        case 0x87: RES(&A, 0); break;

        case 0x88: RES(&B, 1); break;
        case 0x89: RES(&C, 1); break;
        case 0x8A: RES(&D, 1); break;
        case 0x8B: RES(&E, 1); break;
        case 0x8C: RES(&H, 1); break;
        case 0x8D: RES(&L, 1); break;
        case 0x8E: RESMEM(1); break;
        case 0x8F: RES(&A, 1); break;

        case 0x90: RES(&B, 2); break;
        case 0x91: RES(&C, 2); break;
        case 0x92: RES(&D, 2); break;
        case 0x93: RES(&E, 2); break;
        case 0x94: RES(&H, 2); break;
        case 0x95: RES(&L, 2); break;
        case 0x96: RESMEM(2); break;
        case 0x97: RES(&A, 2); break;

        case 0x98: RES(&B, 3); break;
        case 0x99: RES(&C, 3); break;
        case 0x9A: RES(&D, 3); break;
        case 0x9B: RES(&E, 3); break;
        case 0x9C: RES(&H, 3); break;
        case 0x9D: RES(&L, 3); break;
        case 0x9E: RESMEM(3); break;
        case 0x9F: RES(&A, 3); break;

        case 0xA0: RES(&B, 4); break;
        case 0xA1: RES(&C, 4); break;
        case 0xA2: RES(&D, 4); break;
        case 0xA3: RES(&E, 4); break;
        case 0xA4: RES(&H, 4); break;
        case 0xA5: RES(&L, 4); break;
        case 0xA6: RESMEM(4); break;
        case 0xA7: RES(&A, 4); break;

        case 0xA8: RES(&B, 5); break;
        case 0xA9: RES(&C, 5); break;
        case 0xAA: RES(&D, 5); break;
        case 0xAB: RES(&E, 5); break;
        case 0xAC: RES(&H, 5); break;
        case 0xAD: RES(&L, 5); break;
        case 0xAE: RESMEM(5); break;
        case 0xAF: RES(&A, 5); break;

        case 0xB0: RES(&B, 6); break;
        case 0xB1: RES(&C, 6); break;
        case 0xB2: RES(&D, 6); break;
        case 0xB3: RES(&E, 6); break;
        case 0xB4: RES(&H, 6); break;
        case 0xB5: RES(&L, 6); break;
        case 0xB6: RESMEM(6); break;
        case 0xB7: RES(&A, 6); break;

        case 0xB8: RES(&B, 7); break;
        case 0xB9: RES(&C, 7); break;
        case 0xBA: RES(&D, 7); break;
        case 0xBB: RES(&E, 7); break;
        case 0xBC: RES(&H, 7); break;
        case 0xBD: RES(&L, 7); break;
        case 0xBE: RESMEM(7); break;
        case 0xBF: RES(&A, 7); break;

        case 0xC0: _SET(&B, 0); break;  
        case 0xC1: _SET(&C, 0); break;  
        case 0xC2: _SET(&D, 0); break;  
        case 0xC3: _SET(&E, 0); break;  
        case 0xC4: _SET(&H, 0); break;  
        case 0xC5: _SET(&L, 0); break;  
        case 0xC6: _SETMEM(0); break; 
        case 0xC7: _SET(&A, 0); break;  
        case 0xC8: _SET(&B, 1); break;  
        case 0xC9: _SET(&C, 1); break;  
        case 0xCA: _SET(&D, 1); break;  
        case 0xCB: _SET(&E, 1); break;  
        case 0xCC: _SET(&H, 1); break;  
        case 0xCD: _SET(&L, 1); break;          
        case 0xCE: _SETMEM(1); break; 
        case 0xCF: _SET(&A, 1); break;  
        
        case 0xD0: _SET(&B, 2); break;
        case 0xD1: _SET(&C, 2); break;
        case 0xD2: _SET(&D, 2); break;
        case 0xD3: _SET(&E, 2); break;
        case 0xD4: _SET(&H, 2); break;
        case 0xD5: _SET(&L, 2); break;
        case 0xD6: _SETMEM(2); break;
        case 0xD7: _SET(&A, 2); break;
        case 0xD8: _SET(&B, 3); break;
        case 0xD9: _SET(&C, 3); break;
        case 0xDA: _SET(&D, 3); break;
        case 0xDB: _SET(&E, 3); break;
        case 0xDC: _SET(&H, 3); break;
        case 0xDD: _SET(&L, 3); break;
        case 0xDE: _SETMEM(3); break;
        case 0xDF: _SET(&A, 3); break;

        case 0xE0: _SET(&B, 4); break;
        case 0xE1: _SET(&C, 4); break;
        case 0xE2: _SET(&D, 4); break;
        case 0xE3: _SET(&E, 4); break;
        case 0xE4: _SET(&H, 4); break;
        case 0xE5: _SET(&L, 4); break;
        case 0xE6: _SETMEM(4); break;
        case 0xE7: _SET(&A, 4); break;
        case 0xE8: _SET(&B, 5); break;
        case 0xE9: _SET(&C, 5); break;
        case 0xEA: _SET(&D, 5); break;
        case 0xEB: _SET(&E, 5); break;
        case 0xEC: _SET(&H, 5); break;
        case 0xED: _SET(&L, 5); break;
        case 0xEE: _SETMEM(5); break;
        case 0xEF: _SET(&A, 5); break;

        case 0xF0: _SET(&B, 6); break;
        case 0xF1: _SET(&C, 6); break;
        case 0xF2: _SET(&D, 6); break;
        case 0xF3: _SET(&E, 6); break;
        case 0xF4: _SET(&H, 6); break;
        case 0xF5: _SET(&L, 6); break;
        case 0xF6: _SETMEM(6); break;
        case 0xF7: _SET(&A, 6); break;
        case 0xF8: _SET(&B, 7); break;
        case 0xF9: _SET(&C, 7); break;
        case 0xFA: _SET(&D, 7); break;
        case 0xFB: _SET(&E, 7); break;
        case 0xFC: _SET(&H, 7); break;
        case 0xFD: _SET(&L, 7); break;
        case 0xFE: _SETMEM(7); break;
        case 0xFF: _SET(&A, 7); break;

        default: break;
    }
}

//sometimes cycles/pc increments can vary but this will cover most cases
static byte cycles[0x100] = {
  /*0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F	*/
    4, 12,  8,  8,  4,  4,  8,  4, 20,  8,  8,  8,  4,  4,  8,  4,	/* 0x00 */
    4, 12,  8,  8,  4,  4,  8,  4, 12,  8,  8,  8,  4,  4,  8,  4,	/* 0x10 */
    8, 12,  8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4,	/* 0x20 */
    8, 12,  8,  8, 12, 12, 12,  4,  8,  8,  8,  8,  4,  4,  8,  4,	/* 0x30 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x40 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x50 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x60 */
    8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x70 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x80 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x90 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0xA0 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0xB0 */
    8, 12, 12, 16, 12, 16,  8, 16,  8, 16, 12,  8, 12, 24,  8, 16,	/* 0xC0 */
    8, 12, 12,  0, 12, 16,  8, 16,  8, 16, 12,  0, 12,  0,  8, 16,	/* 0xD0 */
   12, 12,  8,  0,  0, 16,  8, 16, 16,  4, 16,  0,  0,  0,  8, 16,	/* 0xE0 */
   12, 12,  8,  4,  0, 16,  8, 16, 12,  8, 16,  4,  0,  0,  8, 16	/* 0xF0 */
};

//some are 0 because pc get incd during the instruction
//the -1's never get called, they are undefined
static byte pc_inc[0x100] = {
  /*0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F	*/
    1,  3,  1,  1,  1,  1,  2,  1,  3,  1,  1,  1,  1,  1,  2,  1,	/* 0x00 */
    1,  3,  1,  1,  1,  1,  2,  1,  2,  1,  1,  1,  1,  1,  2,  1,	/* 0x10 */
    2,  3,  1,  1,  1,  1,  2,  1,  2,  1,  1,  1,  1,  1,  2,  1,	/* 0x20 */
    2,  3,  1,  1,  1,  1,  2,  1,  2,  1,  1,  1,  1,  1,  2,  1,	/* 0x30 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x40 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x50 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x60 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x70 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x80 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x90 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0xA0 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0xB0 */
    1,  1,  0,  0,  0,  1,  2,  0,  0,  0,  0,  1,  0,  0,  2,  0,	/* 0xC0 */
    0,  1,  0, -1,  0,  1,  2,  0,  0,  0,  0, -1,  0, -1,  2,  0,	/* 0xD0 */
    2,  1,  1, -1, -1,  1,  2,  0,  2,  0,  3, -1, -1, -1,  2,  0,	/* 0xE0 */
    2,  1,  1,  1, -1,  1,  2,  0,  2,  1,  3,  1, -1, -1,  2,  0	/* 0xF0 */
};

static const byte extended_cycles[0x100] = {
  /* 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
     8,  8,  8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, /* 0x00 */
     8,  8,  8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, /* 0x10 */
     8,  8,  8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, /* 0x20 */
     8,  8,  8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, /* 0x30 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x40 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x50 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x60 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x70 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x80 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x90 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0xA0 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0xB0 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0xC0 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0xD0 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0xE0 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8  /* 0xF0 */
};

static const byte extended_pc_inc[0x100] = {
  /* 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x00 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x10 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x20 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x30 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x40 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x50 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x60 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x70 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x80 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x90 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xA0 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xB0 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xC0 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xD0 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xE0 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xF0 */
};