# About

Star Flight is a 1987 DOS game, created by Binary Systems and Published by EA. It's notable for being a fairly expansive open word game, with hundreds of different planets and star systems. It's also notable for being written in a mix of FORTH and ASM. As FORTH is an interpreted language, it means that quite a lot of data can be found in the files themselves, which makes decompiling it much easier then it would be for other languages. 

Of course it's not exactly a simple process, as there are a number of challenges as well as some obfuscation of the meta data that is stored in the code as well. As a simple breakdown, FORTH is made up of a collection of words, which can be either ASM code or a list of other FORTH words to execute. The names of the words are usually stored in the compiled data, but not always. In the case of Star flight, many words have had their name stripped away, while others have been truncated. This makes it somewhat a challenge to understand what the code is doing, even as it's been rebuilt.

The code itself it split up into two parts. The main code, stored in the STARFLT file, and a number of overlays stored in the STARA and STARB files. Most of the names have been stripped from the overlay data, and the names in the STARFLT file has been obfuscated. 

One important thing to note, the FORETHOUGHT Engine, which is the FORTH engine the game uses, is old enough that it only supports Block based file IO. This means that all data is read in 1024 byte blocks. Because of this, any form of structured data (IE: Anything record based, such is the different data files, or the instance data) can not cross a block. This results in chunks of unused data near the 1024 byte boundary in the raw files. 

## Renaming

The rename1.csv file provides the renaming rules when decompiling Star Flight 1. This allows the unnamed words to be given more logical names, as well as filling in the truncated names. This file also allows for the definition of words that the deforth tool couldn't otherwise find, as well as providing extra data type information that couldn't be determined from the code itself. 

## AFIELDS and IFIELDS

AFIELDS and IFIELD is how the game stores and manages data about the game world. IFIELDS are references into the instance data, and based on the current record loaded at the time. AFIELDS are data stored in the data files, and unlike IFIELDS have a record number. 

## DEFORTH

The deforth tool does all the work of decompilling all the FORTH code, as well as extracting all the other data files stored in the STARA and STARB files, including image and instance data. For the moment the tool itself is only designed to work with Star Flight 1, but support for 2 is planed for the future.

## Instance Data

The Instance data is where much of the dynamic data of the game is stored, including all the information about the universe, the crew, string data and the general state of the universe. It's a tree of data, with each node having both data, as well as possible children. Most of the string data in the file is usually stored compressed using simple huffman compression. 

## Disassembly  

The system uses a rough version of my own disassembly library, which is a tad overpowered for this project. The full version will hopefully be released sooner or later, once I deal with MAP4. 

## Images Data

There are three different ways that images data can be stored, which are then used to build the different image file formats. In all cases any extra pixel data is ignored. 

### 16-Color Image Data

This is the simplest of the image file formats, being a stream of bytes, with each byte representing two pixels, with the High Nibble being the first value, and the low nibble being the second value. The colors are stored using the CGA/RGB color values, and when being written in EGA mode need ho have extra mapping to map them to the correct EGA color. 

The size of the image are provided by the image file.

### 1-Bit Image Data

A series of 16-bit words, where each bit indicates if the pixel is activated. 

The color for active pixels, and size of the image are provided by the image file. This is the format used for images stored inside the FORTH code.

### 1-Bit RLE Image Data

This formate supports Run Length Encoded pixel data. The data is formatted as a series of bytes, with each byte representing a number of activate or inactivated pixels. The current state (Active or inactive) will flip each byte. A count of zero is valid, and results in no pixels being written, skipping directly to the next byte. The processing ends once full data stream is processed, which can be before the full size of the image.

The color for active pixels, the size of the run length encoded data, and size of the image are provided by the image file.

## Image Files

There are a surprising number of different ways to wrap the image data, depending on the file and the usage of the data. Some are nearly the same as others, but with small and surprising differences. 

### Images in FORTH Data

This isn't quite what you would call an image format, as the 1-bit image data itself is stored as a FORTH Word. ABLT is the address of the image data, while WBLT and LBLT are the length and width. 

### Full Screen images (SPLASH & CREDITS)

These two entries are both 160x200 images stored as a 16-Color image in reverse line order, that is the last line of the image is the first line of the file, and so on. The image it self is just the raw data, without any meta data.

### Starport Crew pictures

These are 100x54 images stored as 1-Bit RLE data. The only meta data is a word that holds the length of the data, so will need to be adjusted to the length of the RLE data.

### Animated Images (PHAZES & GALAXY)

The animated images are stored as a series of frames, each frame itself is a 16-Color image. The placement of the image and the pause between frames is provided by the FORTH code. Each image has a 16-bit header which contains, among other things, the height and width of the image (and bytes 10 and 6 respectively).

#### PHAZES

![](\sf1-output\img\PHAZES.gif)

These are Render over the SPLASH image and create the image of the spinning planet. Each image is placed at 11x103 on the splash image, and has a pause of about 500 ms. Each frame is aligned at 512 bytes.

#### GALAXY

![](\sf1-output\img\GALAXY.gif)

These are rendered over the CREDITS image, and creates the animation of the zooming in galaxy. The y Position of each image is 70, the x position is based on the MUSIC:XOFFS table, and the pause between frames is from the MUSIC:PAUSEOFF table. 

### ICONS (ICON1:1, ICON1:2, ICON1:4)

ICONS are a series of 18 bytes records, each record representing a single icon, and records can not cross 1024 byte blocks. 

Each icon Record contains two images. Each image is a byte providing the color, followed by 8x8 1-Bit image data. The colors are stored using the CGA/RGB color values, and when being written in EGA mode need ho have extra mapping to map them to the correct EGA color. 

### Layer Images (MED-PIC, all CPICS, TPORT-PIC & BPORT-PIC)

Layered images provide a number of different image 1-Bit image data, each in a different color, to create a full color image. Each layer is the same size, and layers can be stored in either RLE or none RLE format.

The data starts with a 5 byte header, consisting of the length as a word, followed by the image Hight, Width and layer count.

Each layer has a 4 byte header. The first word being the length of the layer, a byte for the color, and a byte indicated the storage type. If the encoding is 0 the image data is 1-Bit image data. If it's 0x1 then the image data is stored as RLE data, with a leading word that stores the length of the RLE data.

The colors are stored using the CGA/RGB color values, and when being written in EGA mode need ho have extra mapping to map them to the correct EGA color. 

### Star system icons (LSYSICON, MSYSICON, SSYSICON)

The system icons are stored in a slightly different version of the layered image format. 

Just like with the layer format, The data starts with a 5 byte header, consisting of the length as a word, followed by the image Hight, Width and layer count.

Each layer stored in the 1-bit image data format, with a leading byte indicated the color used by this layer. 

The colors are stored using the CGA/RGB color values, and when being written in EGA mode need ho have extra mapping to map them to the correct EGA color. 

### Vessel silhouettes (VES-BLT)

Vessel data are 55x41 B&W images stored as 1-Bit BLT image data, with a leading word holding the length of the BLT data. The VES-BLT file holds a number of images, the offset to any given image can be found in the Vessel data.

