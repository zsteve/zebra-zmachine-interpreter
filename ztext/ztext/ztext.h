/** ztext.h
 * z-machine text functions
 **/

#ifndef ZTEXT_H
#define ZTEXT_H

#include <exception>
#include "../../zglobal/zglobal.h"
#include "../../zmemory/zmemory/zmemory.h"
#include "../../zerror/zerror/zerror.h"

extern ZError zErrorLogger;

class IllegalZCharString : std::exception{
    public:
    IllegalZCharString(){
        zErrorLogger.addError("Error : IllegalZCharString thrown");
    }
};

class IllegalZCharException : std::exception{
    public:
    IllegalZCharException(){
        zErrorLogger.addError("Error : IllegalZCharException thrown");
    }
};

/**
 * returns the length of a z-character string in zwords
 * @param str pointer to string
 * @return number of zwords
 */
int zCharStrLen(zword* str);

/**
 * returns the length of a ZSCII character string in zbytes
 * @param str pointer to string
 * @return number of zbytes
 */
int ZSCIIStrLen(zchar* str);

/**
 * concatacenates two ZSCII strings. synonymous to cstdlib's strcat()
 * @param src source string (string which will be appended to)
 * @param cat second string to append to first string
 */
void ZSCIIStrCat(zchar* src, zchar* cat);

/** copies a ZSCII string
 * @param src source string
 * @param dest target memory destination
 */
void ZSCIIStrCpy(zchar* src, zchar* dest);

/**
 * returns the resident alphabet (0, 1, 2) of a certain ZSCII character
 * @param zch ZSCII character in question
 * @throw IllegalZCharException if input character is invalid
 * @return the resident alphabet, an integer of value 0, 1, or 2
 */
int ZSCIIGetResidentAlphabet(zchar zch) throw (IllegalZCharException);

/**
 * returns the correct z-char alphabet shift character for switching from one alphabet to another
 * @param currentAlpha index (0, 1, 2) of the current alphabet
 * @param desiredAlpha index (0, 1, 2) of the desired alphabed
 * @param shiftLock true if you want a shift-lock character returned (only z-machine versions 1 & 2)
 * @return shift character
 */
zchar getZCharAlphaShiftCharacter(int currentAlpha, int desiredAlpha, bool shiftLock=false);

/**
 * converts a string of z-characters to ZSCII
 * return value MUST be freed by the CALLER
 * @param zCharString pointer to string of zwords
 * @param zMem reference to ZMemory object
 * @return a string of ZSCII zchars
 */
zchar* zCharStringtoZSCII(zword* zCharString, ZMemory& zMem);

/**
 * helper function for zCharStringtoZSCII
 * converts a string of z-characters to ZSCII
 * return value MUST be freed by the CALLER
 * @param zCharString pointer to string of expanded zwords (zchars)
 * @param zStringLength length in zchars of zcharString
 * @param zMem reference to ZMemory object
 * @param resetShifts if this is true, function resets all alphabet shifts and returns without doing anything
 * @return a string of ZSCII zchars
 */
zchar* zCharStringtoZSCIIHelper(zchar* zCharString, ulong zStringLength, ZMemory& zMem, bool resetShifts=false);

/**
 * converts a 3 or less ZSCII characters to a single z-character
 * return value MUST be freed by the CALLER
 * @param zscii pointer to string of ZSCII chars
 * @param bytesConverted reference to int to hold number of bytes converted
 * @param resetShifts if this is true, function resets all alphabet shifts and returns without doing anything
 * @return a single zword, the encoded result of 3 or less ZSCII chars
 */
zword ZSCIItoZChar(zchar* zscii, int& bytesConverted, bool resetShifts=false);

/**
 * converts a ZSCII string to a string of z-characters (zwords)
 * return value MUST be freed by the CALLER
 * @param zscii pointer to string of ZSCII chars
 * @return string of z-characters (zwords)
 */
zword* ZSCIItoZCharString(zchar* zscii);

/** converts an ordinary z-character string to a
 * dictionary encoded z-character string
 * return value MUST be freed by the CALLER
 * @param zstring pointer to string of zwords
 * @return pointer to string of zwords of encoded string
 */
zword* zChartoDictionaryZCharString(zword* zstring);

zword* dictionaryZCharStringtoZCharString(zword* zstring);

#endif
