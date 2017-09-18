minos v06r03
Author: hex_usr
Dependencies:
  nall (by byuu)
  hiro (by byuu)
  kaijuu (by byuu)
  icarus (by byuu)
License: ISC

minos is an emulator launcher that temporarily converts a cartridge folder into
formats that emulators other than higan support. It is a companion program to
kaijuu.

On Windows, it is recommended that kaijuu be installed to take full advantage
of minos. Strictly speaking, however, kaijuu is optional.

minos needs to be configured before using. See the Configuration section for
how to configure settings such as emulator paths in settings.bml.

===============================================================================
Usage

minos is meant for simulating cartridge folder support for emulators that do
not native support cartridge folders, such as Project64, DeSmuME, and
Kega Fusion. As such, it is at its most effective when paired with kaijuu.
The command format is:

  "C:/<path-to>/minos.exe" {file}

Or:

  "C:/<path-to>/minos.exe" {file} "<emulator-name>"

Where {file} is the path to the cartridge folder, and <emulator-name> is the
name of a specific emulator to run if the default is not desired.

If using kaijuu, {file} can be directly put into the Command field, including
the curly braces, but without quotation marks. Make sure the Match Folders
setting is checked.

After this is configured, simply double-click on a cartridge folder to run
the emulator.

For consoles not supported by higan (such as the Nintendo 64, Nintendo DS, or
Neo Geo Pocket), you will need to build cartridge folders yourself, as icarus
will not help you with those.

  Nintendo 64 Format:
  board region={ntsc|pal}
    rom name=program.rom size={rom.size}
    ram type={sram|eeprom|flash} name=save.ram size={ram.size}

  Nintendo DS Format (not compatible with dasShiny):
  board
    rom name=program.rom size={rom.size}
    ram type={sram|eeprom|flash} name=save.ram size={ram.size}

  Mega Drive Format, Lock-On:
  board region={ntsc-j|ntsc-u|pal}
    rom name=program.rom size={rom.size}
    lock-on
      rom name=upmem.rom size=0x40000
  //minos will show a browser dialog prompting for a cartridge folder
  //to Lock-On when opening this one.

  Neo Geo Pocket:
  board
    rom name=program.rom size={rom.size}
    ram type=flash name=save.ram size={ram.size}

===============================================================================
Configuration

First, specify the location of icarus in the "icarus/path" node at the top.
This is required to support cartridge folders of consoles supported by higan
without manifest.bml files. For other consoles, manifest.bml files are required,
and icarus cannot generate them.

You should not alter the list of supported consoles in "icarus/supports",
unless you want to use nSide's icarus equivalent, cart-pal (in which case you
should add "vs" and "pc10" to the list).

settings.bml has 3 major sections: Systems, Extensions, and Emulators.

-------------------------------------------------------------------------------
Systems

Each system node defines a cartridge folder extension, a console name, and
the locations of the nodes in the manifest to look up ROM and RAM filenames
and sizes.

"system/ext" defines the extension of the cartridge folder. For consoles
supported by higan, this should match the extension expected by higan and
icarus. For example, the Famicom uses "fc" instead of "nes".

"system/name" defines the name of the console. For consoles supported by higan,
this should match the name that higan and icarus use. For example, the SNES
uses "Super Famicom", not "Super Family Computer" or "Super NES" or "SNES".

"system/rom/ext" defines the ROM extension most typical of emulator support. For
example, Famicom emulators usually expect "nes" instead of "fc". For the
Super Famicom, you can use either "sfc" or "smc" depending on what extension
the majority of your ROMs use, though "sfc" is recommended.

"system/rom"'s children (excluding "ext") define how to put together the ROM for
use with the emulator. Order is important. The possible component types are
below:

  "system/rom/query" defines the BML query used to locate the ROM node in the
  manifest. This is required to look up the ROM's name and size.
  A "system/rom" node can have multiple query children to represent, for
  example, BIOS files or SPC7110 split ROMs.

  "system/rom/special" defines a console-specific special feature, such as the
  iNES header (nes20) or a trick needed to play Galaxian (undersize-prg).

    nes20: NES 2.0 header
      A 16-byte header that nearly every Famicom emulator expects to see at the
      beginning of a file. Defines PRG/CHR ROM/RAM size, mapper number,
      mirroring, presence of a battery, and arcade hardware.

    undersize-prg: Famicom ROM mirroring for undersize PRG
      Galaxian's PRG ROM size is too small for iNES's specifications, so it is
      usually stored twice in an iNES file. This feature will not activate for
      larger ROMs such as Donkey Kong and Balloon Fight.

    key16: 7-byte mirror of PlayChoice-10 key ROM
      no$nes expects a 32-byte string after the INST-ROM, but strictly speaking,
      only 9 of those bytes actually exist in the actual cartridge.
      This feature mirrors the last byte and adds 0x00 in a specific way to
      match the PlayChoice-10's Data port.

    counter-out: 00 00 00 00 FF FF FF FF 00 00 00 00 FF FF FF FF
      no$nes expects a 32-byte string after the INST-ROM, but strictly speaking,
      only 9 of those bytes actually exist in the actual cartridge.
      This feature adds the byte sequence above to match the PlayChoice-10's
      Counter Out port.

"system/ram/ext" defines the RAM extension most typical of emulator support.
For example, Famicom emulators usually expect "sav", while Super Famicom
emulators usually expect "srm".

"system/ram"'s children (excluding "ext") define how to put together the RAM
for use with the emulator. Order is important. The possible component types are
below:

  "system/ram/query" defines the BML query used to locate the RAM node in the
  manifest. This is required to look up the RAM's name and size.
  A "system/ram" node can only have 1 query child, though its order relative to
  special nodes is not locked.

  "system/ram/special" defines a console-specific special feature, such as
  DeSmuME's DSV footer (dsv).

    dsv: DeSmuME's DSV footer
      A 114-byte footer designed by DeSmuME's developers to reduce headaches
      from determining save type for each ROM.

-------------------------------------------------------------------------------
Extensions and Emulators

Each extension node defines the default emulator for a ROM extension, as well
as global settings that apply to all emulators, except when overrided.

Each emulator node defines its executable path, the path of its save files, and
certain miscellaneous settings specific to each emulator. Settings specified
here override extension settings.

"extension|emulator/ram-path" defines the path where the emulator(s) place RAM
files. If omitted, minos assumes that they are left in the same directories as
their respective ROMs.
It is possible to use Windows environment variables such as %AppData% and
%LocalAppData% in the path.

"emulator/ram-name-format" specifies how the emulator expects RAM files to be
named. If omitted, the default is "<rom-name>.<ext>". Tokens surrounded by
less-than and greater-than symbols are substituted with dynamic info.

  <name>: Uses the name of the cartridge folder without its extension.

  <ext>: Uses the specified RAM extension. It is recommended to use this
    token instead of writing the RAM extension directly, so that multi-system
    emulators that use different extensions for each console can take advantage
    of the dynamic substitution, as well as emulators that use multiple RAMs
    such as RTC (real-time clock) memory.

  <internal>: Specific to the Nintendo 64. Takes the name from the ROM header,
    and substitutes slashes (/), colons (:), and backslashes (\) with
    hyphen-minuses (-), semicolons (;), and hyphen-minuses (-) respectively.

  <md5>: Uses the MD5 digest of the ROM.

  <fullstop-bug>: Uses a substring of the cartridge folder's name from the
    beginning up to the character preceding the first full stop.
    For example, using this with "Legend of Zelda, The (USA) (1.1).fc" will
    give "Legend of Zelda, The (USA) (1".

===============================================================================
Planned Future Additions

These features are not supported in the current version of minos. They may be
added at a later date.

* manifest generation for non-icarus consoles
    Obviously, icarus does not support the Nintendo 64, Nintendo DS, and other
    consoles, making it impossible to use icarus to generate manifests for
    their games.
    Default paths for ROMs and RAMs could be specified, and default sizes for
    special components such as the DSV footer could be specified to aid in
    predictive re-purification of RAMs.

* "emulator/ascii"
    Many emulators have no support for Unicode characters in paths and/or
    filenames. This setting will alter minos's behavior to use generic
    names for the ROM and RAM in a temporary directory when launching, but only
    if the path includes at least 1 character not in ASCII, such as the Pokémon
    games or most Japanese-region games.

* "emulator/ram-destructive"
    Some emulators, such as VisualBoyAdvance-M, are potentially harmful to save
    files depending on various conditions such as RAM type, saving the file
    with the wrong size, or assuming the wrong RAM type. This setting will
    enable loading the RAM in the emulator, but upon closing, the RAM is not
    written back.
    This will result in lost progress if save states are not used.
