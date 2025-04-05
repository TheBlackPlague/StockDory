<h1 align="center">Grana Format Specification</h1>

<h3 align="center">From Grana, all things take root.</h3>

> [!IMPORTANT]
> **Format Version: Radix**

> [!NOTE]
> The format follows the LE (Little Endian) layout for all data unless specified otherwise. Most modern day architectures use
Little Endian, and the goal of this format is to be read or reinterpreted from one memory to another by a memory copy
operation (however that may be implemented for the relevant architecture).

### Data Types:
Below are some of the data types that make understanding Grana easier. It is important to however note that not each of
the data types must have an equivalent implementation in code. They are design details that can be implemented during
runtime in whatever manner one wants as long as the overall specification is followed.

- **BitBoard**: 64-bit unsigned integer
- **PackedPC**: 4-bit unsigned integer
    ```yaml
    Piece : 3-bit unsigned integer
    Color : 1-bit unsigned integer
    ```
- **PackedPCArray**: 16 byte array (type: PackedPC, count: 32)
    ```yaml
    0  : PackedPC
    1  : PackedPC
    ...
    31 : PackedPC
    ```
- **PackedPosition**: 24 byte data structure
    ```yaml
    Occupancy    : BitBoard
    PiecesColors : PackedPCArray
    ```
    ```python
    Occupancy = BitBoard of all occupied squares belonging to white or black
    
    i = 0
    for sq in Occupancy:
        p = Piece at sq
        c = Color at sq
        PiecesColors[i] = PackedPC { p, c }
        i += 1
    ```
- **Result**: 2-bit unsigned integer [enum]
    ```yaml
    Win  : base10(0) -- binary(00)
    Loss : base10(1) -- binary(01)
    Draw : base10(2) -- binary(10)
    ```
- **Move**: 16-bit unsigned integer
    ```yaml
    From      : 6-bit unsigned integer
    To        : 6-bit unsigned integer
    Promotion : 3-bit unsigned integer

    --------------------------------------
    RESERVED  : 1-bit
    ```
- **Stalk**: 4 byte data structure
    ```yaml
    NextMove : Move
    Score    : 16-bit signed integer
    ```
- **Seed**: 32 byte data structure
    ```yaml
    Position               : PackedPosition
    CastlingAndColorToMove : 8-bit unsigned integer
    GameResult             : Result
    RemainingStalkCount    : 6-bit unsigned integer
    Initial                : Stalk
    --------------------------------------
    RESERVED               : 2 bytes
    ```
