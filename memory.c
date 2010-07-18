/*
 * memory.c
 *
 *  Created on: 2010/06/18
 *      Author: brNX
 */
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


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

extern INLINE uint8 readMem(uint16 address,Memory * mem)
{
    uint8 index;
    uint16 newaddress;
    uint16 addr;

    switch (address >> 12){

    /************ROM**************/
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        index=address >> 14;
        newaddress = address + index*((mem->cart->rombank-1)*0x4000);
        return mem->rombanks[newaddress];
        break;

    /*********VRAM**************/
        //TODO: verificar
    case 0x8:
    case 0x9:
        return mem->wram[address-0x8000];
        break;

    /*********external ram*********/
    case 0xA:

        //MBC2
        if (mem->cart->type.index == 0x05 || mem->cart->type.index == 0x06){
            /*0xA000 <= adress <= 0xA1FF*/
            addr = address - 0xA000;
            if (addr <= 0xA1FF) return (mem->rambanks[addr]&0xF);
            break;
        }

    case 0xB:
        return mem->rambanks[address-0xA000 + (mem->cart->rambank*8192)];
        break;

    /***********Work RAM************/
    case 0xC:
    case 0xD:
        return mem->wram[address-0xC000];
        break;

    /********part of echo ram*******/
    case 0xE:
         return mem->wram[address-0xE000];
         break;

    //E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)
    //FE00-FE9F   Sprite Attribute Table (OAM)
    //FEA0-FEFF   Not Usable
    //FF00-FF7F   I/O Ports
    //FF80-FFFE   High RAM (HRAM)
    //FFFF        Interrupt Enable Register
    case 0xF:

         //FE00-FE9F   Sprite Attribute Table (OAM)
         /*FE00 <= addr <= 0xFE9F*/
         addr= address - 0xFE00;
         if (addr <= 0xFE9F){
             return mem->OAM[addr];
             break;
         }

         //FF80-FFFE   High RAM (HRAM)
         /*FF80 <= addr <= 0xFFFE*/
         addr= address - 0xFF80;
         if (addr <= 0xFFFE){
             return mem->hram[addr];
             break;
         }

         //FF00-FF7F   I/O Ports
         //FFFF  Interrupt Enable Register
         if (address == 0xFFFF){
             return mem->ie;
             break;
         }

         //echo ram
         /*F000 <= addr <= 0xFDFF*/
         addr= address - 0xF000;
         if (addr <= 0xFDFF){
             return mem->wram[0X1000|addr];
             break;
         }

        break;
    default:
        break;

    }

    return 0;
}


extern INLINE void writeMem(uint16 address, uint8 value,Memory * mem)
{
    uint16 x;
    uint16 addr;
    uint16 change;

    switch (address >> 12){

    case 0x0:
        break;
    case 0x1:
        break;

    //rom bank switching
    //2000<= adress <= 3FFF
    case 0x2:
    case 0x3:

        x = address - 0x2000;
        switch (mem->cart->type.index){

            //MBC1
        case 0x1:
        case 0x2:
        case 0x3:
            //00 -> 01, 20 -> 21 , 40 -> 41 , 60 -> 61
            addr = value&0x1F;
            addr+=(addr==0)?1:0;
            mem->cart->rombank=(mem->cart->rombank&0xE0) | addr;
            break;

            //MBC2
        case 0x5:
        case 0x6:
            mem->cart->rombank=value&0xF;
            break;

            //MBC3
        case 0xF:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            addr = value&0x7F;
            addr+=(addr==0)?1:0;
            mem->cart->rombank=value;
            break;

        default :
                break;

        }
        break;

    /*4000-5FFF - RAM Bank Number (MBC 1/3) - or - Upper Bits of ROM Bank Number (Write Only) (MBC 1)
    -or - RTC Register Select (Write Only) (MBC 3)*/
    case 0x4:
    case 0x5:

        //MBC1
        /*0x1 <= index <= 0x3*/
        x= mem->cart->type.index - 0x1;
        if (x <= 0x3){

            change = value&0x3; //2 bit register

            /*if rom mode*/
            if (mem->cart->mbc1mode==0)
                mem->cart->rombank=(mem->cart->rombank&0x1F) | (change<<5);
            else
                mem->cart->rambank= change;
            return;
        }

        //MBC3
        /*0xF <= index <= 0x13*/
        x= mem->cart->type.index - 0xF;
        if (x <= 0x13){
            change = value & 0xF;
            switch(change){

                /*ram bank change*/
            case 0x0:
            case 0x1:
            case 0x2:
            case 0x3:
                mem->cart->rambank= change;
                break;

                /*rtc*/
            case 0x8:
            case 0x9:
            case 0xA:
            case 0xB:
            case 0xC:
                mem->cart->mbc3rtc=change;
                break;

            default:
                break;
            }
            return;
        }

        break;

    case 0x6:
        break;
    case 0x7:
        break;

    /*******Video Ram*********/
    case 0x8:
    case 0x9:
        mem->vram[address -0x8000]=value;
        break;

    /*******external ram********/
    case 0xA:

        //MBC2
        if (mem->cart->type.index == 0x05 || mem->cart->type.index == 0x06){
            /*0xA000 <= adress <= 0xA1FF*/
            addr = address - 0xA000;
            if (addr <= 0xA1FF) mem->rambanks[addr]=(value&0xF);
            break;
        }

    case 0xB:
        mem->rambanks[address-0xA000 + (mem->cart->rambank*8192)]=value;
        break;

    /*C000-CFFF   4KB Work RAM Bank 0 (WRAM)*/
    /*D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)*/
    case 0xC:
        addr=address & 0xFFF;
        mem->wram[addr] = value;
        break;
    case 0xD:
        addr= address - 0xC000;
        mem->wram[addr] = value;
        break;

    /*E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)*/
    case 0xE:
        addr=address & 0xFFF;
        mem->wram[addr] = value;
        break;


    //E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)
    //FE00-FE9F   Sprite Attribute Table (OAM)
    //FEA0-FEFF   Not Usable
    //FF00-FF7F   I/O Ports
    //FF80-FFFE   High RAM (HRAM)
    //FFFF        Interrupt Enable Register
    case 0xF:

        //FE00-FE9F   Sprite Attribute Table (OAM)
        /*FE00 <= addr <= 0xFE9F*/
        addr= address - 0xFE00;
        if (addr <= 0xFE9F){
            mem->OAM[addr]=value;
            break;
        }

        //FF80-FFFE   High RAM (HRAM)
        /*FF80 <= addr <= 0xFFFE*/
        addr= address - 0xFF80;
        if (addr <= 0xFFFE){
            mem->hram[addr]=value;
            break;
        }

        //FF00-FF7F   I/O Ports
        //FFFF  Interrupt Enable Register
        if (address == 0xFFFF){
            mem->ie=value;
            break;
        }

        //echo ram
        /*F000 <= addr <= 0xFDFF*/
        addr= address - 0xF000;
        if (addr <= 0xFDFF){
            mem->wram[0X1000|addr] = value;
            break;
        }


        break;
    }


}

void initMemory(Memory * mem,Cartridge * cart){
    int romsize,ramsize;
    
    cart->rambank=0;
    cart->rombank=1;
    cart->mbc1mode=0;

    mem->cart=cart;

    
    /*16384 bytes *  n rombanks*/
    if (cart->size.nbanks > 0)
        romsize=cart->size.nbanks*16384*sizeof(uint8);
    else
        romsize=2*16384*sizeof(uint8);
    mem->rombanks=(uint8 *) malloc (romsize);


    /*rambanks*/
    switch (cart->ramsize)
    {
    case 0:
        ramsize=0;
        mem->rambanks=NULL;
        break;
    case 1:
        ramsize=2*1024*sizeof(uint8);
        break;
    case 2:
        ramsize=8*1024*sizeof(uint8);
        break;
    case 3:
        ramsize=32*1024*sizeof(uint8);
        break;
    default:
        break;
    }

    //TODO : read and write ram from/to file  (savegame)
    if (ramsize > 0)
        mem->rambanks=(uint8 *) malloc (ramsize);

    //MBC2
    if (cart->type.index == 0x06 || cart->type.index == 0x05)
        mem->rambanks=(uint8 *) malloc (512*sizeof(uint8));


    /*copy rom from file*/
    memcpy(mem->rombanks,cart->gbcart,romsize);

    /*free cart file*/
    destroy_cart_file(cart);

}


void destroyMemory(Memory * mem){

    if (mem->rambanks)
        free(mem->rambanks);
    mem->rambanks=NULL;

    if (mem->rombanks)
        free(mem->rombanks);
    mem->rombanks=NULL;


}
