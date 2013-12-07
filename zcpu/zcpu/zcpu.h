#ifndef ZCPU_H
#define ZCPU_H

#include "../../zmemory/zmemory/zmemory.h"
#include "../../zmemory/zmemory/zobject.h"
#include "../../zstack/zstack/zstack.h"
#include "../../zglobal/zglobal.h"
#include "../../ztext/ztext/ztext.h"

/**
 * namespace for z-cpu enums
 */
namespace ZCpuOps
{
    enum ZOpcodeType{
        ZOPTYPE_LONG, ZOPTYPE_SHORT, ZOPTYPE_EXT, ZOPTYPE_VAR
    };

    enum ZOperandCount{
        ZOPCOUNT_0, ZOPCOUNT_1, ZOPCOUNT_2, ZOPCOUNT_VAR
    };

    enum ZOperandType{
        ZOPERANDTYPE_LARGE_CONST, ZOPERANDTYPE_SMALL_CONST, ZOPERANDTYPE_VAR, ZOPERAND_OMITTED
    };

    enum ZOperandName{
        // 2 OPS
        JE,
        JL,
        JG,
        DEC_CHK,
        INC_CHK,
        JIN,
        TEST,
        OR,
        AND,
        TEST_ATTR,
        SET_ATTR,
        CLEAR_ATTR,
        STORE,
        INSERT_OBJ,
        LOADW,
        LOADB,
        GET_PROP,
        GET_PROP_ADDR,
        GET_NEXT_PROP,
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        CALL_2S,
        CALL_2N,
        SET_COLOUR_FB,
        SET_COLOUR_FBW,
        THROW,

        // 1 OP

        JZ,
        GET_SIBLING,
        GET_CHILD,
        GET_PARENT,
        GET_PROP_LEN,
        INC,
        DEC,
        PRINT_ADDR,
        CALL_1S,
        REMOVE_OBJ,
        PRINT_OBJ,
        RET,
        JUMP,
        PRINT_PADDR,
        LOAD,
        NOT,
        CALL_1N,

        // O OP

        RTRUE,
        RFALSE,
        PRINT,
        PRINT_RET,
        NOP,
        SAVE_LABEL,
        SAVE_RESULT,
        RESTORE_LABEL,
        RESTORE_RESULT,
        RESTART,
        RET_POPPED,
        POP,
        CATCH,
        QUIT,
        NEW_LINE,
        SHOW_STATUS,
        VERIFY,
        EXTENDED,
        PIRACY,

        // VAR OP

        CALL,
        CALL_VS,
        STOREW,
        STOREB,
        PUT_PROP,
        SREAD_P,
        SREAD_PTR,
        SREAD_PTRR,
        AREAD,
        PRINT_CHAR,
        PRINT_NUM,
        RANDOM,
        PUSH,
        PULL_VAL,
        PULL_VAR,
        SPLIT_WINDOW,
        SET_WINDOW,
        CALL_VS2,
        ERASE_WINDOW,
        ERASE_LINE,
        ERASE_LINE_PIX,
        SET_CURSOR_LC,
        SET_CURSOR_LCW,
        GET_CURSOR,
        SET_TEXT_STYLE,
        BUFFER_MODE,
        OUTPUT_STREAM_N,
        OUTPUT_STREAM_NT,
        OUTPUT_STREAM_NTW,
        INPUT_STREAM,
        SOUND_EFFECT,
        READ_CHAR,
        SCAN_TABLE,
        NOT_V,
        CALL_VN,
        CALL_VN2,
        TOKENISE,
        ENCODE_TEXT,
        COPY_TABLE,
        PRINT_TABLE,
        CHECK_ARG_COUNT,

        // EXT OP

        SAVE,
        RESTORE,
        LOG_SHIFT,
        ART_SHIFT,
        SET_FONT,
        DRAW_PICTURE,
        PICTURE_DATA,
        ERASE_PICTURE,
        SET_MARGINS,
        SAVE_UNDO,
        RESTORE_UNDO,
        PRINT_UNICODE,
        CHECK_UNICODE,
        MOVE_WINDOW,
        WINDOW_SIZE,
        WINDOW_STYLE,
        GET_WIND_PROP,
        SCROLL_WINDOW,
        POP_STACK,
        READ_MOUSE,
        MOUSE_WINDOW,
        PUSH_STACK,
        PUT_WIND_PROP,
        PRINT_FORM,
        MAKE_MENU,
        PICTURE_TABLE
    };

};

class ZCpu
{

};

#endif