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

- **BitBoard**: 64-bit unsigned integer
- **PackedPC**: 4-bit unsigned integer

  | Field Name |       Field Type       |
  |:----------:|:----------------------:|
  |   Piece    | 3-bit unsigned integer |
  |   Color    | 1-bit unsigned integer |

- **PackedPCArray**: 16 byte array (type: PackedPC, count: 32)

  | Index | Element  |
  |:-----:|:--------:|
  |   0   | PackedPC |
  |   1   | PackedPC |
  |  ...  | PackedPC |
  |  31   | PackedPC |

- **PackedPosition**: 24 byte data structure

  |  Field Name  |  Field Type   |
  |:------------:|:-------------:|
  |  Occupancy   |   BitBoard    |
  | PiecesColors | PackedPCArray |

  ```python
  Occupancy = Board[White] | Board[Black]
    
  i = 0
  for sq in Occupancy:
    p = Piece at sq
    c = Color at sq
    PiecesColors[i] = PackedPC { p, c }
    i += 1
  ```
  
- **Result**: 2-bit unsigned integer [enum]

  | Name | Decimal Value | Binary Value |
  |:----:|:-------------:|:------------:|
  | Win  |       0       |      00      |
  | Loss |       1       |      01      |
  | Draw |       2       |      10      |

- **Move**: 16-bit unsigned integer

  |   Field Name   |       Field Type       |
  |:--------------:|:----------------------:|
  |   FromSquare   | 6-bit unsigned integer |
  |    ToSquare    | 6-bit unsigned integer |
  | PromotionPiece | 3-bit unsigned integer |

> [!CAUTION]
> The last bit is reserved.

- **Stalk**: 4 byte data structure

  | Field Name |       Field Type        |
  |:----------:|:-----------------------:|
  |    Next    |          Move           |
  |   Score    | 16-bit unsigned integer |

- **BoardState**: 8-bit unsigned integer

  |       Field Name        |       Field Type       |
  |:-----------------------:|:----------------------:|
  | BlackQueenCastleAllowed |     1-bit boolean      |
  | BlackKingCastleAllowed  |     1-bit boolean      |
  | WhiteQueenCastleAllowed |     1-bit boolean      |
  | WhiteKingCastleAllowed  |     1-bit boolean      |
  |       ColorToMove       | 1-bit unsigned integer |

> [!CAUTION]
> The last 3 bits are reserved.

- **Seed**: 32 byte data structure

  |     Field Name      |       Field Type       |
  |:-------------------:|:----------------------:|
  |      Position       |     PackedPosition     |
  |        State        |       BoardState       |
  |     GameResult      |         Result         |
  | RemainingStalkCount | 6-bit unsigned integer |
  |       Initial       |         Stalk          |

> [!CAUTION]
> The last 2 bytes are reserved.