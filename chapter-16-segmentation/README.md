# OSTEP Chapter 16: Segmentation

Homework for [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/), chapter 16 ("Segmentation").
All runs use `segmentation.py` from [ostep-homework](https://github.com/remzi-arpacidusseau/ostep-homework) (`vm-segmentation/`).
The simulator and its flags are described in the [homework README](https://github.com/remzi-arpacidusseau/ostep-homework/blob/master/vm-segmentation/README.md).

The simulator uses two segments. The top bit of the virtual address selects the segment:

| | Segment 0 | Segment 1 |
|---|---|---|
| Top bit | 0 | 1 |
| Holds | code, heap | stack |
| Grows | positive (up) | negative (down) |
| Base points to | physical start of the segment | physical end of the segment |

The rules behind every answer, for an address space of size `asize`:

```
Segment 0 (address < asize / 2):
    valid if address < limit0
    physical address = base0 + address

Segment 1 (address >= asize / 2):
    distance = asize - address          # how far below the top edge
    valid if distance <= limit1
    physical address = base1 - distance
```

The top edge pairs up in both worlds: virtual `asize` corresponds to physical `base1`. Neither belongs to the segment itself, both are the first address past its end.

---

## 1. First let's use a tiny address space to translate some addresses. Here's a simple set of parameters with a few different random seeds; can you translate the addresses?

Address space 128 bytes, so addresses 0 to 63 are segment 0 and 64 to 127 are segment 1. Both segments are 20 bytes.

```sh
./segmentation.py -a 128 -p 512 -b 0 -l 20 -B 512 -L 20 -s 0 -c
```

```
Segment register information:

  Segment 0 base  (grows positive) : 0x00000000 (decimal 0)
  Segment 0 limit                  : 20

  Segment 1 base  (grows negative) : 0x00000200 (decimal 512)
  Segment 1 limit                  : 20

Virtual Address Trace
  VA  0: 0x0000006c (decimal:  108) --> VALID in SEG1: 0x000001ec (decimal:  492)
  VA  1: 0x00000061 (decimal:   97) --> SEGMENTATION VIOLATION (SEG1)
  VA  2: 0x00000035 (decimal:   53) --> SEGMENTATION VIOLATION (SEG0)
  VA  3: 0x00000021 (decimal:   33) --> SEGMENTATION VIOLATION (SEG0)
  VA  4: 0x00000041 (decimal:   65) --> SEGMENTATION VIOLATION (SEG1)
```

```sh
./segmentation.py -a 128 -p 512 -b 0 -l 20 -B 512 -L 20 -s 1 -c
```

```
Virtual Address Trace
  VA  0: 0x00000011 (decimal:   17) --> VALID in SEG0: 0x00000011 (decimal:   17)
  VA  1: 0x0000006c (decimal:  108) --> VALID in SEG1: 0x000001ec (decimal:  492)
  VA  2: 0x00000061 (decimal:   97) --> SEGMENTATION VIOLATION (SEG1)
  VA  3: 0x00000020 (decimal:   32) --> SEGMENTATION VIOLATION (SEG0)
  VA  4: 0x0000003f (decimal:   63) --> SEGMENTATION VIOLATION (SEG0)
```

```sh
./segmentation.py -a 128 -p 512 -b 0 -l 20 -B 512 -L 20 -s 2 -c
```

```
Virtual Address Trace
  VA  0: 0x0000007a (decimal:  122) --> VALID in SEG1: 0x000001fa (decimal:  506)
  VA  1: 0x00000079 (decimal:  121) --> VALID in SEG1: 0x000001f9 (decimal:  505)
  VA  2: 0x00000007 (decimal:    7) --> VALID in SEG0: 0x00000007 (decimal:    7)
  VA  3: 0x0000000a (decimal:   10) --> VALID in SEG0: 0x0000000a (decimal:   10)
  VA  4: 0x0000006a (decimal:  106) --> SEGMENTATION VIOLATION (SEG1)
```

**Answer:**

| Seed | VA | Segment | Check | Result |
|---|---|---|---|---|
| 0 | 108 | 1 | distance 128 - 108 = 20, 20 <= 20 | 512 - 20 = 492 |
| 0 | 97 | 1 | distance 31 > 20 | violation |
| 0 | 53 | 0 | 53 >= 20 | violation |
| 0 | 33 | 0 | 33 >= 20 | violation |
| 0 | 65 | 1 | distance 63 > 20 | violation |
| 1 | 17 | 0 | 17 < 20 | 0 + 17 = 17 |
| 1 | 108 | 1 | distance 20 <= 20 | 492 |
| 1 | 97 | 1 | distance 31 > 20 | violation |
| 1 | 32 | 0 | 32 >= 20 | violation |
| 1 | 63 | 0 | 63 >= 20 | violation |
| 2 | 122 | 1 | distance 6 <= 20 | 512 - 6 = 506 |
| 2 | 121 | 1 | distance 7 <= 20 | 512 - 7 = 505 |
| 2 | 7 | 0 | 7 < 20 | 7 |
| 2 | 10 | 0 | 10 < 20 | 10 |
| 2 | 106 | 1 | distance 22 > 20 | violation |

The decimal value in the trace is the full virtual address including the top bit. For segment 1 the useful number is the distance to the top edge (128), not the distance to the bottom of the segment range (64). For 97 the low bits give 97 - 64 = 33, which measures from the wrong edge. The book gets the same result with 33 - 64 = -31.

## 2. Now, let's see if we understand this tiny address space we've constructed (using the parameters from the question above). What is the highest legal virtual address in segment 0? What about the lowest legal virtual address in segment 1? What are the lowest and highest illegal addresses in this entire address space? Finally, how would you run segmentation.py with the -A flag to test if you are right?

```
0 ........ 19 | 20 ........ 107 | 108 ........ 127
    valid     |     illegal     |      valid
```

Testing the four boundary addresses:

```sh
./segmentation.py -a 128 -p 512 -b 0 -l 20 -B 512 -L 20 -A 19,20,107,108 -c
```

```
Virtual Address Trace
  VA  0: 0x00000013 (decimal:   19) --> VALID in SEG0: 0x00000013 (decimal:   19)
  VA  1: 0x00000014 (decimal:   20) --> SEGMENTATION VIOLATION (SEG0)
  VA  2: 0x0000006b (decimal:  107) --> SEGMENTATION VIOLATION (SEG1)
  VA  3: 0x0000006c (decimal:  108) --> VALID in SEG1: 0x000001ec (decimal:  492)
```

**Answer:**

- Highest legal address in segment 0: **19** (valid offsets are 0 to 19, the check is `< 20`)
- Lowest legal address in segment 1: **108** (distance 20, the check is `<= 20`)
- Lowest illegal address: **20**
- Highest illegal address: **107**

Note the asymmetry. Both segments hold exactly 20 bytes, but the offsets of segment 0 run from 0 to 19, while the distances of segment 1 run from 1 to 20. The highest address 127 already sits one byte below the top edge.

63 and 64 are only the boundaries of the two segment ranges (where the top bit flips), not of the legal addresses.

## 3. Let's say we have a tiny 16-byte address space in a 128-byte physical memory. What base and bounds would you set up so as to get the simulator to generate the following translation results for the specified address stream: valid, valid, violation, ..., violation, valid, valid?

Wanted pattern:

```
Address: 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
Result:  V  V  X  X  X  X  X  X  X  X  X  X  X  X  V  V
```

Address space 16, so the top bit is worth 8: addresses 0 to 7 are segment 0, 8 to 15 are segment 1.

```sh
./segmentation.py -a 16 -p 128 -A 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 --b0 0 --l0 2 --b1 14 --l1 2 -c
```

```
Segment register information:

  Segment 0 base  (grows positive) : 0x00000000 (decimal 0)
  Segment 0 limit                  : 2

  Segment 1 base  (grows negative) : 0x0000000e (decimal 14)
  Segment 1 limit                  : 2

Virtual Address Trace
  VA  0: 0x00000000 (decimal:    0) --> VALID in SEG0: 0x00000000 (decimal:    0)
  VA  1: 0x00000001 (decimal:    1) --> VALID in SEG0: 0x00000001 (decimal:    1)
  VA  2: 0x00000002 (decimal:    2) --> SEGMENTATION VIOLATION (SEG0)
  VA  3: 0x00000003 (decimal:    3) --> SEGMENTATION VIOLATION (SEG0)
  VA  4: 0x00000004 (decimal:    4) --> SEGMENTATION VIOLATION (SEG0)
  VA  5: 0x00000005 (decimal:    5) --> SEGMENTATION VIOLATION (SEG0)
  VA  6: 0x00000006 (decimal:    6) --> SEGMENTATION VIOLATION (SEG0)
  VA  7: 0x00000007 (decimal:    7) --> SEGMENTATION VIOLATION (SEG0)
  VA  8: 0x00000008 (decimal:    8) --> SEGMENTATION VIOLATION (SEG1)
  VA  9: 0x00000009 (decimal:    9) --> SEGMENTATION VIOLATION (SEG1)
  VA 10: 0x0000000a (decimal:   10) --> SEGMENTATION VIOLATION (SEG1)
  VA 11: 0x0000000b (decimal:   11) --> SEGMENTATION VIOLATION (SEG1)
  VA 12: 0x0000000c (decimal:   12) --> SEGMENTATION VIOLATION (SEG1)
  VA 13: 0x0000000d (decimal:   13) --> SEGMENTATION VIOLATION (SEG1)
  VA 14: 0x0000000e (decimal:   14) --> VALID in SEG1: 0x0000000c (decimal:   12)
  VA 15: 0x0000000f (decimal:   15) --> VALID in SEG1: 0x0000000d (decimal:   13)
```

**Answer:** `--l0 2 --l1 2`. The limit is a size, not an index: segment 0 needs 2 bytes to cover offsets 0 and 1, segment 1 needs 2 bytes to cover distances 1 (address 15) and 2 (address 14).

The bases are almost free. They only have to keep both segments inside the 128 bytes of physical memory without overlapping. Here segment 0 sits at physical 0 to 1 and segment 1 at physical 12 to 13, right below its base of 14. Virtual 14 maps to physical 12, not 14: the virtual and the physical address space are independent.

## 4. Assume we want to generate a problem where roughly 90% of the randomly-generated virtual addresses are valid (not segmentation violations). How should you configure the simulator to do so? Which parameters are important?

```sh
./segmentation.py -a 128 -p 512 --b0 0 --l0 58 --b1 512 --l1 58 -n 100 -s 0 -c | grep -c VALID
```

Number of valid addresses out of 100, for seeds 0 to 4:

```
seed 0: 89
seed 1: 87
seed 2: 86
seed 3: 91
seed 4: 93
```

**Answer:** what matters is the ratio of the limits to the size of the address space. Addresses are drawn uniformly over the whole address space, so

```
valid fraction = (limit0 + limit1) / asize
```

For 90% of 128 the limits have to add up to about 115. Split evenly that is 58 per segment, 116 / 128 = 90.6%. The runs above average 89.2%.

Each segment only covers half of the address space, so a limit larger than `asize / 2` adds nothing. The physical memory has to be large enough to hold both segments without overlap (at least 116 bytes here), otherwise the simulator rejects the configuration.

## 5. Can you run the simulator such that no virtual addresses are valid? How?

```sh
./segmentation.py -a 128 -p 512 --b0 0 --l0 0 --b1 512 --l1 0 -n 5 -c
```

```
Segment register information:

  Segment 0 base  (grows positive) : 0x00000000 (decimal 0)
  Segment 0 limit                  : 0

  Segment 1 base  (grows negative) : 0x00000200 (decimal 512)
  Segment 1 limit                  : 0

Virtual Address Trace
  VA  0: 0x0000006c (decimal:  108) --> SEGMENTATION VIOLATION (SEG1)
  VA  1: 0x00000061 (decimal:   97) --> SEGMENTATION VIOLATION (SEG1)
  VA  2: 0x00000035 (decimal:   53) --> SEGMENTATION VIOLATION (SEG0)
  VA  3: 0x00000021 (decimal:   33) --> SEGMENTATION VIOLATION (SEG0)
  VA  4: 0x00000041 (decimal:   65) --> SEGMENTATION VIOLATION (SEG1)
```

**Answer:** set both limits to 0. In segment 0 the offset would have to be smaller than 0, in segment 1 the distance to the top edge is always at least 1 and therefore larger than 0. The bases do not matter, because no address ever gets translated.

Generating addresses outside the address space does not work: the simulator only draws addresses from 0 to `asize - 1`.
