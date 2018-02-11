/*
 Name 1: Brent Atchison
 UTEID 1: bma862
*/

/***************************************************************/
/*                                                             */
/*   LC-3b Instruction Level Simulator                         */
/*                                                             */
/*   EE 460N                                                   */
/*   The University of Texas at Austin                         */
/*                                                             */
/***************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************/
/*                                                             */
/*   Files: isaprogram   LC-3b machine language program file   */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void process_instruction(void);
void br(short, short, short, short);
void imm_add(short, short, short);
void reg_add(short, short, short);
void imm_and(short, short, short);
void reg_and(short, short, short);
void jmp(short);
void jsr(short);
void jsrr(short);
void ldb(short, short, short);
void ldw(short, short, short);
void lea(short, short);
void lshf(short, short, short);
void rshfl(short, short, short);
void rshfa(short, short, short);
void stb(short, short, short);
void stw(short, short, short);
void trap(short);
void imm_xor(short, short, short);
void reg_xor(short, short, short);
void flags(short);
short sign(short, short);

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
 * MEMORY[A][1] stores the most significant byte of word at word address A */

#define WORDS_IN_MEM    0x08000
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;    /* run bit */

typedef struct System_Latches_Struct{
    int PC, /* program counter */
    N,      /* n condition bit */
    Z,      /* z condition bit */
    P;      /* p condition bit */
    int REGS[LC_3b_REGS];   /* register file. */
} System_Latches;

/* Data Structure for Latch */
System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int INSTRUCTION_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands                    */
/*                                                             */
/***************************************************************/
void help() {
    printf("----------------LC-3b ISIM Help-----------------------\n");
    printf("go               -  run program to completion         \n");
    printf("run n            -  execute program for n instructions\n");
    printf("mdump low high   -  dump memory from low to high      \n");
    printf("rdump            -  dump the register & bus values    \n");
    printf("?                -  display this help menu            \n");
    printf("quit             -  exit the program                  \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {
    process_instruction();
    CURRENT_LATCHES = NEXT_LATCHES;
    INSTRUCTION_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {
    int i;
    
    if (RUN_BIT == FALSE) {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }
    
    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
        if (CURRENT_LATCHES.PC == 0x0000) {
            RUN_BIT = FALSE;
            printf("Simulator halted\n\n");
            break;
        }
        cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed                 */
/*                                                             */
/***************************************************************/
void go() {
    if (RUN_BIT == FALSE) {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }
    
    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000) { cycle(); }
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {
    int address; /* this is a byte address */
    
    printf("\nMemory content [0x%.4x..0x%.4x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
        printf("  0x%.4x (%d) : 0x%.2x%.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");
    
    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%.4x..0x%.4x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
        fprintf(dumpsim_file, " 0x%.4x (%d) : 0x%.2x%.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {
    int k;
    
    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Instruction Count : %d\n", INSTRUCTION_COUNT);
    printf("PC                : 0x%.4x\n", CURRENT_LATCHES.PC);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++) { printf("%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]); }
    printf("\n");
    
    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Instruction Count : %d\n", INSTRUCTION_COUNT);
    fprintf(dumpsim_file, "PC                : 0x%.4x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++) { fprintf(dumpsim_file, "%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]); }
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {
    char buffer[20];
    int start, stop, cycles;
    
    printf("LC-3b-SIM> ");
    
    scanf("%s", buffer);
    printf("\n");
    
    switch(buffer[0]) {
        case 'G':
        case 'g':
            go();
            break;
            
        case 'M':
        case 'm':
            scanf("%i %i", &start, &stop);
            mdump(dumpsim_file, start, stop);
            break;
            
        case '?':
            help();
            break;
            
        case 'Q':
        case 'q':
            printf("Bye.\n");
            exit(0);
            
        case 'R':
        case 'r':
            if (buffer[1] == 'd' || buffer[1] == 'D') { rdump(dumpsim_file); }
            else {
                scanf("%d", &cycles);
                run(cycles);
            }
            break;
            
        default:
            printf("Invalid Command\n");
            break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Zero out the memory array                       */
/*                                                             */
/***************************************************************/
void init_memory() {
    int i;
    
    for (i = 0; i < WORDS_IN_MEM; i++) {
        MEMORY[i][0] = 0;
        MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {
    FILE * prog;
    int ii, word, program_base;
    
    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
        printf("Error: Can't open program file %s\n", program_filename);
        exit(-1);
    }
    
    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF) { program_base = word >> 1; }
    else {
        printf("Error: Program file is empty\n");
        exit(-1);
    }
    
    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
        /* Make sure it fits. */
        if (program_base + ii >= WORDS_IN_MEM) {
            printf("Error: Program file %s is too long to fit in memory. %x\n", program_filename, ii);
            exit(-1);
        }
        
        /* Write the word to memory array. */
        MEMORY[program_base + ii][0] = word & 0x00FF;
        MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
        ii++;
    }
    
    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = (program_base << 1);
    
    printf("Read %d words from program into memory.\n\n", ii);
}

/************************************************************/
/*                                                          */
/* Procedure : initialize                                   */
/*                                                          */
/* Purpose   : Load machine language program                */
/*             and set up initial state of the machine.     */
/*                                                          */
/************************************************************/
void initialize(char *program_filename, int num_prog_files) {
    int i;
    
    init_memory();
    for (i = 0; i < num_prog_files; i++) {
        load_program(program_filename);
        while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;
    NEXT_LATCHES = CURRENT_LATCHES;
    
    RUN_BIT = TRUE;
}

/************************************************************/
/*                                                          */
/* Procedure : main                                         */
/*                                                          */
/************************************************************/
int main(int argc, char *argv[]) {
    FILE * dumpsim_file;
    
    /* Error Checking */
    if (argc < 2) {
        printf("Error: usage: %s <program_file_1> <program_file_2> ...\n", argv[0]);
        exit(1);
    }
    
    printf("LC-3b Simulator\n\n");
    initialize(argv[1], argc - 1);
    
    if ((dumpsim_file = fopen("dumpsim", "w")) == NULL) {
        printf("Error: Can't open dumpsim file\n");
        exit(-1);
    }
    
    while (1) { get_command(dumpsim_file); }
    
}

/***************************************************************/
/* Do not modify the above code.
 You are allowed to use the following global variables in your
 code. These are defined above.
 
 MEMORY
 
 CURRENT_LATCHES
 NEXT_LATCHES
 
 You may define your own local/global variables and functions.
 You may use the functions to get at the control bits defined
 above.
 
 Begin your code here */
/***************************************************************/


/* function: process_instruction
 *
 * Process one instruction at a time
 *  -Fetch one instruction
 *  -Decode
 *  -Execute
 *  -Update NEXT_LATCHES
 */
void process_instruction(void){
    
    /* Increment PC */
    NEXT_LATCHES.PC = CURRENT_LATCHES.PC + 2;
    
    /* Update Flags */
    NEXT_LATCHES.N = CURRENT_LATCHES.N;
    NEXT_LATCHES.Z = CURRENT_LATCHES.Z;
    NEXT_LATCHES.P = CURRENT_LATCHES.P;
    
    /* Update Opcode */
    int imm = MEMORY[CURRENT_LATCHES.PC / 2][0] + (MEMORY[CURRENT_LATCHES.PC / 2][1] << 8);
    int op = (imm & 0xF000) >> 12;
    
    /* Process Opcode */
    switch (op) {
        case 0: { /* BR */
            short n = (imm & 0x0800) >> 11;
            short z = (imm & 0x0400) >> 10;
            short p = (imm & 0x0200) >> 9;
            short offset = imm & 0x1FF;
            br(n, z, p, offset);
            break;
        }
        case 1: { /* ADD */
            short mask = 0x0020;
            short dr = (imm & 0x0E00) >> 9;
            short sr1 = (imm & 0x01C0) >> 6;
            short sr2 = imm & 0x0007;
            short off = imm & 0x001F;
            if (imm & mask) { imm_add(dr, sr1, off); }  /* imm[5] is set */
            else { reg_add(dr, sr1, sr2); }
            break;
        }
        case 2: { /* LDB */
            short dr  = (imm & 0x0E00) >> 9;
            short base = (imm & 0x01C0) >> 6;
            short off = imm & 0x003F;
            ldb(dr, base, off);
            break;
        }
        case 3: { /* STB */
            short sr = imm & 0x0E00 >> 9;
            short base = imm & 0x01C0 >> 6;
            short off = imm & 0x003F;
            stb(sr, base, off);
            break;
        }
        case 4: { /* JSR */
            if (imm & 0x0800) { jsr(imm & 0x07FF); }
            else { jsrr((imm & 0x1C0) << 6); }
            break;
        }
        case 5: { /* AND */
            short mask = 0x0020;
            short dr  = (imm & 0x0E00) >> 9;
            short sr1 = (imm & 0x01C0) >> 6;
            short sr2 = imm & 0x0007;
            short off = imm & 0x001F;
            if (imm & mask) { imm_and(dr, sr1, off); }  /* imm[5] is set */
            else { reg_and(dr, sr1, sr2); }
            break;
        }
        case 6: { /* LDW */
            short dr = (imm & 0x0E00) >> 9;
            short base = (imm & 0x01C0) >> 6;
            short off = imm & 0x003F;
            ldw(dr, base, off);
            break;
        }
        case 7: { /* STW */
            short sr = (imm & 0x0E00) >> 9;
            short base = (imm & 0x01C0) >> 6;
            short off = imm & 0x003F;
            stw(sr, base, off);
            break;
        }
        case 9: { /* XOR */
            unsigned short mask = 0x0020;
            unsigned short dr  = (imm & 0x0E00) >> 9;
            unsigned short sr1 = (imm & 0x01C0) >> 6;
            unsigned short sr2 = imm & 0x0007;
            unsigned short off = imm & 0x001F;
            if (imm & mask) { imm_xor(dr, sr1, off); }  /* imm[5] is set */
            else { reg_xor(dr, sr1, sr2); }
            break;
        }
        case 12: { /* JMP */
            jmp((imm & 0x01C0) >> 6);
            break;
        }
        case 13: { /* SHF */
            short dr  = (imm & 0x0E00) >> 9;
            short sr = (imm & 0x01C0) >> 6;
            short shift = imm & 0x000F;
            if(imm & 0x0010) {
                if(imm & 0x0020) { rshfa(dr, sr, shift); }
                else { rshfl(dr, sr, shift); }
            } else { lshf(dr, sr, shift); }
            break;
        }
        case 14: { /* LEA */
            short dr = (imm & 0x0E00) >> 9;
            short off = imm & 0x01FF;
            lea(dr, off);
            break;
        }
        case 15: { /* TRAP */
            trap(imm & 0x00FF);
            break;
        }
        default: { printf("You done messed up."); break; }
    }
}

/* Branch */
void br(short n, short z, short p, short offset) {
    if((n && CURRENT_LATCHES.N) || (z && CURRENT_LATCHES.Z) || (p && CURRENT_LATCHES.P)) {
        short imm = sign(9, offset);
        NEXT_LATCHES.PC = NEXT_LATCHES.PC + imm * 2;
    }
}

/* Add Immediate */
void imm_add(short dr, short sr1, short off) {
    short imm = sign(5, off);
    NEXT_LATCHES.REGS[dr] = (short)(CURRENT_LATCHES.REGS[sr1] + imm);
    flags((short)NEXT_LATCHES.REGS[dr]);
}

/* Add Register */
void reg_add(short dr, short sr1, short sr2) {
    NEXT_LATCHES.REGS[dr] = (short)(CURRENT_LATCHES.REGS[sr1] + CURRENT_LATCHES.REGS[sr2]);
    flags((short)NEXT_LATCHES.REGS[dr]);
}

/* And Immediate */
void imm_and(short dr, short sr1, short off) {
    signed short imm = sign(5, off);
    NEXT_LATCHES.REGS[dr] = (short) (CURRENT_LATCHES.REGS[sr1] & imm);
    flags((short) NEXT_LATCHES.REGS[dr]);
}

/* And Register */
void reg_and(short dr, short sr1, short sr2) {
    NEXT_LATCHES.REGS[dr] = (short) (CURRENT_LATCHES.REGS[sr1] & CURRENT_LATCHES.REGS[sr2]);
    flags((short) NEXT_LATCHES.REGS[dr]);
}

/* Jump */
void jmp(short base) { NEXT_LATCHES.PC = (short) (CURRENT_LATCHES.REGS[base]); }

/* Jump to Subroutine with Offset */
void jsr(short off) {
    NEXT_LATCHES.REGS[7] = (short) NEXT_LATCHES.PC;
    short imm = sign(11, off);
    NEXT_LATCHES.PC = (short) (NEXT_LATCHES.PC + imm * 2);
}

/* Jump to Subroutine with Register */
void jsrr(short base) {
    NEXT_LATCHES.REGS[7] = (short) NEXT_LATCHES.PC;
    NEXT_LATCHES.PC = (short) CURRENT_LATCHES.REGS[base];
}

/* Load Byte */
void ldb(short dr, short base, short off) {
    short imm = sign(6, off) / 2;
    int p = sign(6, off) % 2;
    short mem = CURRENT_LATCHES.REGS[base] / 2 + imm;
    if(mem < 0){ return; }
    NEXT_LATCHES.REGS[dr] = (short) MEMORY[mem][p];
    flags((short) NEXT_LATCHES.REGS[dr]);
}

/* Load Word */
void ldw(short dr, short base, short off) {
    short imm = sign(6, off);
    short mem = CURRENT_LATCHES.REGS[base] + imm;
    if(mem < 0){ return; }
    NEXT_LATCHES.REGS[dr] = (short) (MEMORY[mem / 2][0] + MEMORY[mem / 2][1] * 256);
    flags((short) NEXT_LATCHES.REGS[dr]);
}

/* Load Effective Address */
void lea(short dr, short off) {
    short imm = sign(9, off);
    NEXT_LATCHES.REGS[dr] = NEXT_LATCHES.PC + (imm * 2);
}

/* Left Shift */
void lshf(short dr, short sr, short shift) {
    NEXT_LATCHES.REGS[dr] = (short) (CURRENT_LATCHES.REGS[sr] << shift);
    flags((short) NEXT_LATCHES.REGS[dr]);
}

/* Right Shift Logical */
void rshfl(short dr, short sr, short shift) {
    NEXT_LATCHES.REGS[dr] = (short) (CURRENT_LATCHES.REGS[sr] >> shift);
    flags((short) NEXT_LATCHES.REGS[dr]);
}

/* Right Shift Arithmetic */
void rshfa(short dr, short sr, short shift) {
    if (CURRENT_LATCHES.REGS[sr] >= 0) {
        rshfl(dr, sr, shift);
        return;
    }
    else {
        NEXT_LATCHES.REGS[dr] = CURRENT_LATCHES.REGS[sr];
        for (; shift > 0; shift--) { NEXT_LATCHES.REGS[dr] = (short) ((NEXT_LATCHES.REGS[dr] >> 1) | 0x8000); }
    }
    flags((short) NEXT_LATCHES.REGS[dr]);
}

/* Store Byte */
void stb(short sr, short dr, short off) {
    short imm = sign(6, off) / 2;
    int p = sign(6, off) % 2;
    short mem = CURRENT_LATCHES.REGS[dr] / 2 + imm;
    if(mem < 0) { return; }
    MEMORY[mem][p] = CURRENT_LATCHES.REGS[sr] & 0x00FF;
}

/* Store Word */
void stw(short sr, short dr, short off) {
    signed short imm = sign(6, off);
    short mem = CURRENT_LATCHES.REGS[dr] + imm;
    if(mem < 0) { return; }
    MEMORY[mem/2][0] = CURRENT_LATCHES.REGS[sr] & 0x00FF;
    MEMORY[mem/2][1] = (CURRENT_LATCHES.REGS[sr] & 0x0FF0) >> 8;
}

/* Trap */
void trap(short tv) {
    NEXT_LATCHES.REGS[7] = NEXT_LATCHES.PC;
    short mem = tv & 0x00FF;
    NEXT_LATCHES.PC = MEMORY[mem][0] + MEMORY[mem][1] * 256;
}

/* XOR with Immediate */
void imm_xor(short dr, short sr, short off) {
    short imm = sign(5, off);
    NEXT_LATCHES.REGS[dr] = (short)(CURRENT_LATCHES.REGS[sr] ^ imm);
    flags((short) NEXT_LATCHES.REGS[dr]);
}

/* XOR with Register */
void reg_xor(short dr, short sr1, short sr2) {
    NEXT_LATCHES.REGS[dr] = (short) (CURRENT_LATCHES.REGS[sr1] ^ CURRENT_LATCHES.REGS[sr2]);
    flags((short) NEXT_LATCHES.REGS[dr]);
}

/* Update NZP Flags */
void flags(short dv){
    if(dv < 0) {
        NEXT_LATCHES.N = 1;
        NEXT_LATCHES.Z = 0;
        NEXT_LATCHES.P = 0;
    }
    else if(dv == 0) {
        NEXT_LATCHES.N = 0;
        NEXT_LATCHES.Z = 1;
        NEXT_LATCHES.P = 0;
    }
    else {
        NEXT_LATCHES.N = 0;
        NEXT_LATCHES.Z = 0;
        NEXT_LATCHES.P = 1;
    }
}

/* Apply Masks */
short sign(short mask, short data) {
    switch (mask) {
        case 5: {
            if(data & 0x0010) { return -(((data & 0x001F) ^ 0x001F) + 1); }
            else { return data & 0x001F; }
        }
        case 6: {
            if(data & 0x0020) { return -(((data & 0x003F) ^ 0x003F) + 1); }
            else { return data & 0x001F; }
        }
        case 9: {
            if(data & 0x0100) { return -(((data & 0x01FF) ^ 0x01FF) + 1); }
            else { return data & 0x001F; }
        }
        case 11: {
            if(data & 0x0800) { return -(((data & 0x0FFF) ^ 0x0FFF) + 1); }
            else { return data & 0x0FFF; }
        }
        default: { return data; }
    }
}
