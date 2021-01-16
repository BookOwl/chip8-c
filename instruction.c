#include <stdint.h>

typedef enum {
    CLEAR,
    RET,
    JMP,
    CALL,
    SKP_EQ,
    SKP_NEQ,
    SKP_EQ_REG,
    LOAD,
    ADD_NUM,
    MOV,
    OR,
    AND,
    XOR,
    ADD_REG,
    SUB_REG,
    RSHIFT,
    SUB_FROM,
    LSHIFT,
    SKP_NEQ_REG,
    LOAD_I,
    JMP_REL,
    RAND,
    DRAW,
    SKP_IF_KEY,
    SKP_IF_NOT_KEY,
    STORE_DELAY,
    WAIT_FOR_KEY,
    SET_DELAY,
    SET_SOUND,
    ADD_I,
    LOAD_DIGIT_SPRITE,
    STORE_BCD,
    SAVE_REG,
    RESTORE_REG,
    INVALID,
} instruction_tag;

/*
 * Struct that represents a decoded instruction
 */
typedef struct {
    instruction_tag tag;
    uint8_t reg1;
    uint8_t reg2;
    uint16_t data;
} instruction;


instruction decode_instruction(uint16_t i) {
    instruction inst;
    inst.tag = INVALID;
    switch (i >> 12) {
        case 0x0:
            inst.tag = (i == 0x00E0 ? CLEAR : RET);
            break;
        case 0x1:
            inst.tag = JMP;
            inst.data = i & 0x0FFF;
            break;
        case 0x2:
            inst.tag = CALL;
            inst.data = i & 0x0FFF;
            break;
        case 0x3:
            inst.tag = SKP_EQ;
            inst.reg1 = (i & 0x0F00) >> 8;
            inst.data = i & 0x00FF;
            break;
        case 0x4:
            inst.tag = SKP_NEQ;
            inst.reg1 = (i & 0x0F00) >> 8;
            inst.data = i & 0x00FF;
            break;
        case 0x5:
            inst.tag = SKP_EQ_REG;
            inst.reg1 = (i & 0x0F00) >> 8;
            inst.reg2 = (i & 0x00F0) >> 4;
            break;
        case 0x6:
            inst.tag = LOAD;
            inst.reg1 = (i & 0x0F00) >> 8;
            inst.data = i & 0x00FF;
            break;
        case 0x7:
            inst.tag = ADD_NUM;
            inst.reg1 = (i & 0x0F00) >> 8;
            inst.data = i & 0x00FF;
            break;
        case 0x8:
            switch (i & 0x000F) {
                case 0x0:
                    inst.tag = MOV;
                    inst.reg1 = (i & 0x0F00) >> 8;
                    inst.reg2 = (i & 0x00F0) >> 4;
                    break;
                case 0x1:
                    inst.tag = OR;
                    inst.reg1 = (i & 0x0F00) >> 8;
                    inst.reg2 = (i & 0x00F0) >> 4;
                    break;
                case 0x2:
                    inst.tag = AND;
                    inst.reg1 = (i & 0x0F00) >> 8;
                    inst.reg2 = (i & 0x00F0) >> 4;
                    break;
                case 0x3:
                    inst.tag = XOR;
                    inst.reg1 = (i & 0x0F00) >> 8;
                    inst.reg2 = (i & 0x00F0) >> 4;
                    break;
                case 0x4:
                    inst.tag = ADD_REG;
                    inst.reg1 = (i & 0x0F00) >> 8;
                    inst.reg2 = (i & 0x00F0) >> 4;
                    break;
                case 0x5:
                    inst.tag = SUB_REG;
                    inst.reg1 = (i & 0x0F00) >> 8;
                    inst.reg2 = (i & 0x00F0) >> 4;
                    break;
                case 0x6:
                    inst.tag = RSHIFT;
                    inst.reg1 = (i & 0x0F00) >> 8;
                    inst.reg2 = (i & 0x00F0) >> 4;
                    break;
                case 0x7:
                    inst.tag = SUB_FROM;
                    inst.reg1 = (i & 0x0F00) >> 8;
                    inst.reg2 = (i & 0x00F0) >> 4;
                    break;
                case 0xE:
                    inst.tag = LSHIFT;
                    inst.reg1 = (i & 0x0F00) >> 8;
                    inst.reg2 = (i & 0x00F0) >> 4;
                    break;
            };
            break;
        case 0x9:
            inst.tag = SKP_NEQ_REG;
            inst.reg1 = (i & 0x0F00) >> 8;
            inst.reg2 = (i & 0x00F0) >> 4;
            break;
        case 0xA:
            inst.tag = LOAD_I;
            inst.data = i & 0x0FFF;
            break;
        case 0xB:
            inst.tag = JMP_REL;
            inst.data = i & 0x0FFF;
            break;
        case 0xC:
            inst.tag = RAND;
            inst.reg1 = (i & 0x0F00) >> 8;
            inst.data = i & 0x00FF;
            break;
        case 0xD:
            inst.tag = DRAW;
            inst.reg1 = (i & 0x0F00) >> 8;
            inst.reg2 = (i & 0x00F0) >> 4;
            inst.data = i & 0x000F;
            break;
        case 0xE:
            inst.tag = ((i & 0xF0FF) == 0xE09E ? SKP_IF_KEY : SKP_IF_NOT_KEY);
            inst.reg1 = (i & 0x0F00) >> 8;
            break;
        case 0xF:
            inst.reg1 = (i & 0x0F00) >> 8;
            switch (i & 0x00FF) {
                case 0x07:
                    inst.tag = STORE_DELAY;
                    break;
                case 0x0A:
                    inst.tag = WAIT_FOR_KEY;
                    break;
                case 0x15:
                    inst.tag = SET_DELAY;
                    break;
                case 0x18:
                    inst.tag = SET_SOUND;
                    break;
                case 0x1E:
                    inst.tag = ADD_I;
                    break;
                case 0x29:
                    inst.tag = LOAD_DIGIT_SPRITE;
                    break;
                case 0x33:
                    inst.tag = STORE_BCD;
                    break;
                case 0x55:
                    inst.tag = SAVE_REG;
                    break;
                case 0x65:
                    inst.tag = RESTORE_REG;
            }
    };
    return inst;
}