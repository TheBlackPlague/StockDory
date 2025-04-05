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

> [!CAUTION]
> Bits or bytes that are unused are marked as reserved since they are currently not used by the format. They may be used 
> by future versions of the format. However, for ease of reading, they must have a zero value. **They are not padding.**

#### BitBoard: 64-bit unsigned integer
#### PieceColor: 4-bit unsigned integer

| Field Name |       Field Type       |
|:----------:|:----------------------:|
|   Piece    | 3-bit unsigned integer |
|   Color    | 1-bit unsigned integer |

#### PieceColorArray: 16 byte array (type: PieceColor, count: 32)

| Index |  Element   |
|:-----:|:----------:|
|   0   | PieceColor |
|   1   | PieceColor |
|  ...  | PieceColor |
|  31   | PieceColor |

#### PackedPosition: 24 byte data structure

|  Field Name  |   Field Type    |
|:------------:|:---------------:|
|  Occupancy   |    BitBoard     |
| PiecesColors | PieceColorArray |

```python
Occupancy = Board[White] | Board[Black]
    
i = 0
for sq in Occupancy:
    p: Piece = Board[sq]
    c: Color = Board[sq]
    PiecesColors[i] = PackedPC { p, c }
    i += 1
```
  
#### Result: 2-bit unsigned integer [enum]

| Name | Decimal Value | Binary Value |
|:----:|:-------------:|:------------:|
| Win  |       0       |      00      |
| Loss |       1       |      01      |
| Draw |       2       |      10      |

#### Move: 16-bit unsigned integer

|   Field Name   |       Field Type       |
|:--------------:|:----------------------:|
|   FromSquare   | 6-bit unsigned integer |
|    ToSquare    | 6-bit unsigned integer |
| PromotionPiece | 3-bit unsigned integer |

> [!CAUTION]
> The last bit is reserved.

#### Stalk: 4 byte data structure

| Field Name |       Field Type        |
|:----------:|:-----------------------:|
|    Next    |          Move           |
|   Score    | 16-bit unsigned integer |

#### BoardState: 8-bit unsigned integer

|       Field Name        |       Field Type       |
|:-----------------------:|:----------------------:|
| BlackQueenCastleAllowed |     1-bit boolean      |
| BlackKingCastleAllowed  |     1-bit boolean      |
| WhiteQueenCastleAllowed |     1-bit boolean      |
| WhiteKingCastleAllowed  |     1-bit boolean      |
|       ColorToMove       | 1-bit unsigned integer |

> [!CAUTION]
> The last 3 bits are reserved.

#### Seed: 32 byte data structure

|     Field Name      |       Field Type       |
|:-------------------:|:----------------------:|
|      Position       |     PackedPosition     |
|        State        |       BoardState       |
|     GameResult      |         Result         |
| RemainingStalkCount | 6-bit unsigned integer |
|       Initial       |         Stalk          |

> [!CAUTION]
> The last 2 bytes are reserved.

#### Grain: Variable-length data structure
> [!IMPORTANT]
> This is a variable length data structure. However, there are constraints on its size.
> 
> 32 bytes <= Size <= 284 bytes

|  Field Name  | Field Type |
|:------------:|:----------:|
|     Base     |    Seed    |
| Continuation |  Stalk(s)  |

> [!TIP]
> In most programming languages, `Continuation` will most likely be represented as a dynamically resizeable array. 
> However, when possible, it's recommended to use a fixed-size array for performance. 
> 
> Given the constraints, it will at maximum have 63 elements. A counter to the number of elements is stored as 
> `Base.RemainingStalkCount` so when serializing (writing to disk), you can only write the `Base.RemainingStalkCount` 
> elements. 
> 
> The only downside to using a fixed-size array is that in memory, you will be using the maximum memory of the data
> structure even if the `Grain` doesn't require it. It is recommended to be considerate of this depending on the system.
