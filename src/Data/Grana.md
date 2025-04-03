# Grana Format Specification

**Format Version: 0.1**

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
    Promotion : 4-bit unsigned integer
    ```
- **Stalk**: 4 byte data structure
    ```yaml
    NextMove : Move
    Score    : 16-bit signed integer
    ```
- **Seed**: 32 byte data structure
    ```yaml
    Position   : PackedPosition
    FirstMove  : Move
    Score      : 16-bit signed integer
    GameResult : Result
    StalkCount : 6-bit unsigned integer
    ```
