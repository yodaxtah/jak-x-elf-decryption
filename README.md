# Jak X ELF decryption

Jak X's ELF is encrypted, which makes it challenging to work with the ISO's ELF. For now, the memory must be dumped by running PCSX2, but it also dumps goal code with that. That's not the biggest problem, but it'd be nice to work in a smaller search space to match functions!

## LZO1x compression

The encryption is actually simply a compression and decompression algorithm, called LZO compression. The compression is probably integrated in the build/release/publish process as a post-operation, and the decompression is run immediately when the ELF is executed. For decompression, `lzo1x_decompress` is called, which supposedly has the following signature:

```c++
int lzo1x_decompress (byte * src, uint src_len, byte * dst, uint * dst_len, void * wrkmem);
```

As of now, two occurrences of `lzo1x_decompress` were found. One is right at the top of the ELF, called in a function at `001009dc`. This function is called from `entry` (`001001b4`) two calls nested. Looking at the code, it appears the decompresssion is preceded by some initialization, and followed by a function call that cannot be resolved. Notice all occurences of `0x800000`.

```c++
void FUN_001009dc(undefined8 param_1,undefined8 param_2,undefined8 param_3)
{
    undefined4 uVar1;
    int iVar2;
    long lVar3;
    uint destinationLength;
    
    FUN_0010068c(1);
    lVar3 = FUN_00100850_precedes_decompressing(0xc00000,0x800000);
    FUN_00102d30();
    scePrintf("decompressing old ELF (size is %d)\n",0x8900e);
    destinationLength = 0;
    iVar2 = lzo1x_decompress(&DAT_0016bbc4,0x89002,(byte *)0x800000,&destinationLength,(void *)0x0);
    if ((uint *)destinationLength == &EXPECTED_OUTPUT_LENGTH_WS) {
        if (iVar2 == 0) {
            goto LAB_00100a98;
        }
        scePrintf("decompress error %d; aborting\n",iVar2);
    }
    else {
        scePrintf("decompress error: output length mismatch (%u != %u)\n",destinationLength,
                &EXPECTED_OUTPUT_LENGTH_WS);
    }
    // ...
    if (lVar3 == 0) {
        FUN_00137b08();
        FUN_00100518();
    }
    else {
        (*(code *)&SUB_00c00000)(0xc00000,0x800000,0x1000000,0x1400000,0,0x1800000,param_3);
    }
    return;
}
```

The other call is made in `dnas_load`, which can only be found by reversing the PCSX2's memory dump. This call also seems to be preceded by some initialization, but it's more generic, as it's used on multiple other places. The function seems to be related to Sony's [Dynamic Network Authentication System](https://manuals.playstation.net/document/en/psp/current/settings/dnas.html). This explains why it was also not present in the codebase thus far. It is however not found among the debug symbols of MyStreet or Ratchet and Clank: Deadlocked, which also use SCE-RT libraries.

```c++
uVar3 = FUN_0025c6f8_also_precedes_decompressing(0x120000c,DAT_01200004);
uVar7 = DAT_01200000;
if (uVar3 == DAT_01200008) {
local_30 = 0;
lVar4 = lzo1x_decompress(0x120000c,DAT_01200004,0x1080000,&local_30,0);
if (local_30 == uVar7) {
    if (lVar4 == 0) {
        CacheFlush(&DAT_01080000,local_30 + 0x3f & 0xffffffc0);
        DAT_00284b04 = 2;
        goto switchD_002795bc_caseD_2;
    }
    pcVar5 = "%s: decompress error %d; aborting\n";
    iVar1 = (int)lVar4;
    iVar8 = -3;
    break;
}
```

## What's in this repo?

For now, I'm working on creating a runnable `decrypt-stage1-jakx-1.exe` that decompresses the necessary parts. On Windows, I can run `make Makefile g++`, and it produces some result! For now, only the first decompression is executed, but it's not clear if that's succesful, besides some things being analyzed differently. I'm working on the `SCES_532.86` file, but I'm guessing the american version (`SCUS...`) is encrypted in the same way. I copied the file here and renamed it `game-elf`. The output will be `decrypted-elf`.

Another bad smell is that some functions contain `halt_baddata`. This function below exists in the dumped memory from PCSX2 (at `00266260`, but unimportant). I `W`ildly guessed the name to be `SceInitTrue_W`.

```c++
void SceInitTrue_W(void)
{
  code *pcVar1;
  
  if (_gp_2 == '\0') {
    _gp_2 = 0;
    pcVar1 = *(code **)PTR_SceInitFalseInitData.function_pointer1_00282d84;
    while (pcVar1 != _gp_1) {
      PTR_SceInitFalseInitData.function_pointer1_00282d84 =
           PTR_SceInitFalseInitData.function_pointer1_00282d84 + 4;
      (*pcVar1)();
      pcVar1 = *(code **)PTR_SceInitFalseInitData.function_pointer1_00282d84;
    }
    if (true) {
      FUN_001034b0(&SceInitTrueInitData);
    }
    _gp_2 = '\x01';
  }
  return;
}
```

In Jak X's ELF and the current output of the decryption tool, it currently is (remains) the code below, while it should match structurally. This means we are missing some kind of fix that is executed by PCSX2. Notice that this function resides at address `00100120`, and it's the third function. This is far before the decompression that starts only at `0x89002` (+ `00100000`).

```c++
void FUN_00100120(void)
{
  if (true) {
    FUN_0012cb88(0x14e684,0x1f5688);
  }
  if ((_DAT_0014e684 != 0) && (false)) {
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  }
  return;
}
```

## Credits

This project is built upon the small Minilzo, hence I left in the README from the original author.