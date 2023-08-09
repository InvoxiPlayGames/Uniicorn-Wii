Uniicorn - WIP Research Wii emulator using Unicorn engine

**THIS DOES NOT, AND PROBABLY WILL NEVER, PLAY RETAIL GAMES OR HOMEBREW!**
Check https://dolphin-emu.org out for that.

**THIS DOES NOT COMPLETELY BOOT THE STARLET!**
Check out https://github.com/eigenform/ironic for that.

---------
- ABOUT -
---------

This is a Wii emulator that can emulate the Wii's Starlet
security coprocessor using the Unicorn CPU emulator engine.
It eventually aims to completely emulate the Wii up to and
including the System Menu and certain homebrew applications.
For now, this is not what it does. It's *very* incomplete.

I made this to research how the Wii's boot process works,
along with its low-level system components. I don't have any
particular goal in mind, although booting up to CEIL1NG_CAT or
the official Wii System Menu would be a fairly good endgame.

I've only tested compiling/running this on an ARM Mac, but it
should be able to build on any ARM or x86_64 Mac or Linux PC.

Contributions are welcome! (as long as no information is
sourced from any illegally obtained material, i.e. leaks)

----------
- STATUS -
----------

Current state: 
 - boot0 completes, boot1 completes(?), boot2loader decrypts
   and loads boot2, boot2 disables boot0, outputs a few debug lines,
   then crashes due to MMU inconsistencies with Unicorn Engine.
   Currently attempting to work around this - possibly virtualising
   the whole MMU is in order as Unicorn Engine seems to do this
   incorrectly and raises a prefetch abort after boot2 enables MMU.
   "mini" is able to boot up, but also crashes due to MMU errors.
- Requires the latest dev commit of Unicorn engine.

Implementations: (y = complete, p = partial)
[p] Starlet CPU bootup [boot0-boot2] (p: 16-07-2022)
[y] AES Engine (p: 17-07-2022, y: 17-07-2022)
[y] SHA1 Engine (p: 17-07-2022, y: 17-07-2022)
[p] NAND Interface (p: 17-07-2022)
[y] OTP Registers (y: 17-07-2022)
[p] GPIOs (p: 17-07-2022)
[ ] Memory Interface
[ ] Memory Controller
[p] Memory Mapping (p: 19-07-2022)
[ ] AHB Interface
[p] Hollywood Registers (p: 16-07-2022)
[ ] SEEPROM GPIO Emulation
[ ] Interrupt Management
[ ] SD Interface
[ ] EHCI Interface
[p] EXI Interface (p: 09-08-2023)
[ ] MMU
    ... more is needed for boot2...

Milestones: (y = complete, p = partial)
[y] boot0 Launch (p: 16-07-2022, y: 17-07-2022)
[p] boot1 Launch (p: 17-07-2022)
[p] boot2/BootMii Launch (p: 19-07-2022)
[p] "mini" Launch (p: 09-08-2023)
[p] boot2/Nintendo Launch (p: 19-07-2022)
[ ] IOS Launch
[ ] PowerPC bringup
    ... more after this...

-----------
- LICENSE -
-----------

This project is licensed under the GPLv2, or later at your
discrescion.

This project uses:
    * Unicorn Engine, under the LGPLv2 license.
      https://www.unicorn-engine.org/
    * 'SHA-1 in C' by Steve Reid, under public domain.
    * tiny-AES-c by kokke, under public domain.
      https://github.com/kokke/tiny-AES-c
  