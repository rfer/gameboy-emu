/*
 * memory.c
 *
 *  Created on: 2010/06/18
 *      Author: brNX
 */
#include "memory.h"
#include <stdio.h>

//0000-3FFF   16KB ROM Bank 00     (in cartridge, fixed at bank 00)
//4000-7FFF   16KB ROM Bank 01..NN (in cartridge, switchable bank number)
//8000-9FFF   8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
//A000-BFFF   8KB External RAM     (in cartridge, switchable bank, if any)
//C000-CFFF   4KB Work RAM Bank 0 (WRAM)
//D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
//E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)
//FE00-FE9F   Sprite Attribute Table (OAM)
//FEA0-FEFF   Not Usable
//FF00-FF7F   I/O Ports
//FF80-FFFE   High RAM (HRAM)
//FFFF        Interrupt Enable Register

//TODO: for now
extern inline uint8 readMem(uint16 address)
{
	return (gb_memory[address]);
}

//TODO: for now
extern inline uint8 readOpcode(uint16 address)
{
        return (gb_memory[address]);
}

extern inline void writeMem(uint16 address, uint8 value)
{
	gb_memory[address] = value;

        //TODO: use optionaly SIMD instructions for ranges

        //ECHO ram
        //a <= x <= b -> x-a <= b - a (hackers delight)
        //C000 <= adress <= DDFF
        uint16 x = address - 0xC000;
        if (x <= 0x1DFF){
            gb_memory[0XE000+x] = value;
            return;
        }

        //E000 <= adress <= FDFF
        x = address - 0xE000;
        if (x <= 0x1DFF){
            gb_memory[0XC000+x] = value;
            return;
        }

        //rom bank switching
        //2000<= adress <= 3FFF
        x = address - 0x2000;
        if (x <= 0x3FFF){

            //MBC1
            if (gb_cart->type.index > 0 && gb_cart->type.index < 4){

                //00 -> 01, 20 -> 21 , 40 -> 41 , 60 -> 61
                uint8 addr = value&0x1F;
                addr+=(addr==0)?1:0;

                gb_cart->rombank=(gb_cart->rombank&0xE0) | addr;
                return;
            }

            //MBC2
            if (gb_cart->type.index > 4 && gb_cart->type.index < 7){
                gb_cart->rombank=value&0xF;
                return;
            }

            //MBC3
            if (gb_cart->type.index > 0xE && gb_cart->type.index < 0x14){
                uint8 addr = value&0x7F;
                addr+=(addr==0)?1:0;

                gb_cart->rombank=value;
                return;
            }


            //TODO: write value?
            //gb_memory[address] = value;
        }


    }
