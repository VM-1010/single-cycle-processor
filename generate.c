#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define WORDSIZE 16
#define MAX_INSTR_STRLEN 32

typedef uint16_t word;

typedef enum {
    ADD=0, SUB=1, MUL=2, AND=3, OR=4, XOR=5, LW=7, ADDI=6, SW=8, BEQ=9, STOP=22, INVALID=11, COMMENT=43
} instr;

void trim(char *s) {
    char *start = s, *end = s + strlen(s) - 1;
    while(*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') start++;
    while(end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    memmove(s, start, end - start + 1);
    s[end - start + 1] = '\0';
}

instr get_opcode(char op[5]) {
    if (!(strcmp(op, "stop"))) return STOP;
    if (!(strcmp(op, "add"))) return ADD;
    if (!(strcmp(op, "sub"))) return SUB;
    if (!(strcmp(op, "mul"))) return MUL;
    if (!(strcmp(op, "and"))) return AND;
    if (!(strcmp(op, "or"))) return OR;
    if (!(strcmp(op, "xor"))) return XOR;
    if (!(strcmp(op, "lw"))) return LW;
    if (!(strcmp(op, "addi"))) return ADDI;
    if (!(strcmp(op, "sw"))) return SW;
    if (!(strcmp(op, "halt"))) return STOP;
    if (!(strcmp(op, "beq"))) return BEQ;
    if (!(strncmp(op, "#", 1))) return COMMENT;
    return INVALID;
}

int assemble(FILE *a, FILE *b) {

    word rom[65536];
    int i = 0;
    char line[32];
    char op[5];
    instr type;
    word opcode, rd, rs1, rs2;
    int imm = 0;
    word imm6;
    int memsize = 0;
    while (fscanf(a, "%512[^\n]\n", line) == 1) {
        trim(line);
        sscanf(line, "%s", op);

        switch(opcode = get_opcode(op)) {
        case COMMENT:

        break;
        case STOP:
            printf("\nline read: halt\nInstruction: beq R0, R0, -1\n");
            rom[i++] = 0xf007;
            printf("at 0x%04x, assembled instruction: 0x%04X\n", i,  rom[i-1]);
            break;
        // rtypes
        case ADD:
        case SUB:
        case MUL:
        case AND:
        case OR:
        case XOR:
            if (sscanf(line, "%s R%hu, R%hu, R%hu", op, &rd, &rs1, &rs2)!=4) {
                printf("invalid instruction: %s\n", line);
                exit(1);
            };
            printf("\nInstruction: %s\nOperation: %s\nDestination: R%d\nSource 1: R%d\nSource 2: R%d\n", line, op, rd, rs1, rs2);
            rom[i++] = (opcode&1111)<<12 | (rd&0b111)<<9 | (rs1&0b111)<<6 | (rs2&0b111)<<3 | 0;
            printf("at 0x%04x, assembled instruction: 0x%04X\n", i,  rom[i-1]);
            break;

        // itypes
        case ADDI:
            if (sscanf(line, "%s R%hu, R%hu, %d", op, &rd, &rs1, &imm) != 4) {
                printf("invalid instruction: %s\n", line);
                exit(1);
            };
            imm6 = (uint16_t)imm &0x3f;
            printf("\nInstruction: %s\nOperation: addi\nOpcode: %d\nDestination: R%d\nSource 1: R%d\nImmediate: %d\n", line,opcode, rd, rs1, imm);
            
            rom[i++] = (opcode&15)<<12 | (rd&7)<<9 | (rs1&7)<<6 | (imm6&0x3f);

            printf("at 0x%04x, assembled instruction: 0x%04X\n", i,  rom[i-1]);
            
        break;


            // TODO, ACCOUNT FOR SIGNED OFFSETS


        case LW:
            if (sscanf(line, "%s R%hu, %d(R%hu)", op, &rd, &imm, &rs1) != 4) {
                printf("invalid instruction: %s\n", line);
                exit(1);
            };
            imm6 = (uint16_t)imm &0x3f;

            printf("\nInstruction: %s\nOperation: lw\nDestination: R%d\nSource 1: R%d\nOffset: %d\n", line, rd, rs1, imm);
            
            rom[i++] = (opcode&15)<<12 | (rd&7)<<9 | (rs1&7)<<6 | (imm6&0x3f);

            printf("at 0x%04x, assembled instruction: 0x%04X\n", i,  rom[i-1]);

        break;

        // btypes
        case SW:
            if (sscanf(line, "%s R%hu, %d(R%hu)", op, &rs2, &imm, &rs1) != 4) {
                printf("invalid instruction: %s\n", line);
                exit(1);
            };
            imm6 = (uint16_t)imm &0x3f;
            printf("\nInstruction: %s\nOperation: sw\nSource 1(addr): R%d\nSource 2(data): R%d\nOffset: %d\n", line, rs1, rs2, imm);
            
            rom[i++] = (opcode&15)<<12 | (imm6&0b111000)<<9 | (rs1&7)<<6 | (rs2&7)<<3 | (imm6&0b111);

            printf("at 0x%04x, assembled instruction: 0x%04X\n", i,  rom[i-1]);
        break;

        case BEQ:

            if (sscanf(line, "%s R%hu, R%hu, %d", op, &rs1, &rs2, &imm) != 4) {
                printf("invalid instruction: %s\n", line);
                exit(1);
            };
            imm6 = (uint16_t)imm &0x3f;
            printf("\nInstruction: %s\nOperation: beq\nSource 1(cmp a): R%d\n   Source 2(cmp b): R%d\nBranch Offset: %d\n", line, rs1, rs2, imm6 & 0x3f);
            printf("beq opcode : %d\n", opcode&0b1111);
            imm6 &= 0x3F;   // keep only 6 bits

            rom[i++] = ((opcode & 15) << 12)
                    | (((imm6 >> 3) & 7) << 9)
                    | ((rs1 & 7) << 6)
                    | ((rs2 & 7) << 3)
                    | (imm6 & 7);

            
            printf("at 0x%04x, assembled instruction: 0x%04X\n", i,  rom[i-1]);
        break;


        case INVALID:
        default:
                printf("\ninvalid instruction: %s\n", op);
        break;
        }
    }
    fwrite(rom, 2, i+1, b);

}



int main(int argc, char **argv) {

    if (argc < 2) {
        printf("usage: %s <assembly program> -o <output>\n", argv[0]);
        exit(1);
    }
    char *filename = "a.out";
    if (argc >= 4 && !strcmp(argv[2], "-o")) filename = argv[3];  
    FILE *as = fopen(argv[1], "r");
    FILE *bin = fopen(filename, "wb");
    

    assemble(as, bin);

    printf("\n");

    fclose(bin);
    fclose(as);
    return 0;
}
