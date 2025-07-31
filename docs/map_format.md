Map format
==========

Thirty saves levels in the **[ClassicWorld](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/ClassicWorld_file_format)** format.
Thirty-specific data is stored in a `Thirty` compound under the `Metadata` compound, e.g.

```
TAG_Compound('ClassicWorld'): 8 entries
{
    TAG_Byte('FormatVersion'): 1
    TAG_String('Name'): 'world2'
    TAG_Byte_Array('UUID'): [16 bytes]
    TAG_Short('X'): 256
    TAG_Short('Y'): 256
    TAG_Short('Z'): 256
    TAG_Byte_Array('BlockArray'): [16777216 bytes]
    TAG_Compound('Metadata'): 1 entry
    {
        TAG_Compound('Thirty'): 1 entry
        {
            TAG_Compound('ScheduledTicks'): 2 entries
            {
                TAG_Int_Array('Indices'): [0 ints]
                TAG_Int_Array('Times'): [0 ints]
            }
        }
    }
}
```

## Scheduled ticks

Scheduled ticks are stored in the `ScheduledTicks` compound:

```
TAG_Compound('ScheduledTicks'): 2 entries
{
    TAG_Int_Array('Indices'): [0 ints]
    TAG_Int_Array('Times'): [0 ints]
}
```

- `Indices` contains the block indices that have a tick scheduled.
- `Times` contains the number of ticks until the tick will run.

The two arrays must be of the same size, otherwise the data is ignored.
