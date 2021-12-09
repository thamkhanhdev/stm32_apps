#ifndef COMPILER_MACROS_H
#define COMPILER_MACROS_H

#ifdef __cplusplus
extern "C"{
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/

/*==================================================================================================
*                              SOURCE FILE VERSION INFORMATION
==================================================================================================*/


/*==================================================================================================
*                                     FILE VERSION CHECKS
==================================================================================================*/


/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
#define _ASM_KEYWORD    __asm
/**
 * @brief C/C++ compiler macros for placing code or variables in the defined section
 */
#ifdef __ICCARM__
    #define PLACE_IN_SECTION_HELPER_0(X) #X
    #define PLACE_IN_SECTION_HELPER_1(SECTION_NAME) PLACE_IN_SECTION_HELPER_0(location = #SECTION_NAME)
    #define PLACE_IN_SECTION_HELPER_2(SECTION_NAME) PLACE_IN_SECTION_HELPER_1(.SECTION_NAME)
    #define PLACE_IN_SECTION(SECTION_NAME) _Pragma(PLACE_IN_SECTION_HELPER_2(SECTION_NAME))
#else
    #define PLACE_IN_SECTION_HELPER(SECTION_NAME) __attribute__ (( section(#SECTION_NAME) ))
    #define PLACE_IN_SECTION(SECTION_NAME) PLACE_IN_SECTION_HELPER(.SECTION_NAME)
#endif

/****************************** GCC ******************************/
#ifdef __GNUC__
    #define _THUMB .thumb
    #define _THUMB2 .thumb
    #define _SYNTAX_UNIFIED .syntax unified

    #define _TYPE(name, ident) .type name, %ident

    #define _WORD .word
    #define _SHORT .short

    #define _EXPORT .global
    #define _EXTERN .global

    #define label object

    #define _ALIGN_POWER_OF_TWO(POWER) .align POWER
    #define _LTORG .ltorg

    #define _ALLOC_BYTES(N) .space N

    #define _INCBIN(file) .incbin file

    /* has to be at the beginning of a line */
    #define _SET(NAME,VALUE) .set NAME, VALUE
    /* has to be at the beginning of a line */
    #define _DEFINE(NAME,VALUE) .set NAME, VALUE

    #define _SECTION_EXEC(SECTION_NAME) .section .SECTION_NAME,"ax"
    #define _SECTION_DATA(SECTION_NAME) .section .SECTION_NAME,"aw"
    #define _SECTION_CONST(SECTION_NAME) .section .SECTION_NAME,"a"
    #define _SECTION_EXEC_W(SECTION_NAME) .section .SECTION_NAME,"awx"
    #define _SECTION_DATA_UNINIT(SECTION_NAME) .section .SECTION_NAME,"aw"

    #define _SECTION_EXEC_ALIGN(SECTION_NAME, ALIGNMENT) _SECTION_EXEC(SECTION_NAME) ; _ALIGN_POWER_OF_TWO(ALIGNMENT)
    #define _SECTION_DATA_ALIGN(SECTION_NAME, ALIGNMENT) _SECTION_DATA(SECTION_NAME) ; _ALIGN_POWER_OF_TWO(ALIGNMENT)
    #define _SECTION_CONST_ALIGN(SECTION_NAME, ALIGNMENT) _SECTION_CONST(SECTION_NAME) ; _ALIGN_POWER_OF_TWO(ALIGNMENT)
    #define _SECTION_EXEC_W_ALIGN(SECTION_NAME, ALIGNMENT) _SECTION_EXEC_W(SECTION_NAME) ; _ALIGN_POWER_OF_TWO(ALIGNMENT)
    #define _SECTION_DATA_UNINIT_ALIGN(SECTION_NAME, ALIGNMENT) _SECTION_DATA_UNINIT(SECTION_NAME) ; _ALIGN_POWER_OF_TWO(ALIGNMENT)

    #define _BYTES_2   1
    #define _BYTES_4   2
    #define _BYTES_8   3
    #define _BYTES_16  4
    #define _BYTES_32  5
    #define _BYTES_64  6
    #define _BYTES_128 7
    #define _BYTES_256 8
    #define _BYTES_512 9

    #define _ALIGN_BYTES_2 _ALIGN_POWER_OF_TWO(1)
    #define _ALIGN_BYTES_4 _ALIGN_POWER_OF_TWO(2)
    #define _ALIGN_BYTES_8 _ALIGN_POWER_OF_TWO(3)
    #define _ALIGN_BYTES_16 _ALIGN_POWER_OF_TWO(4)
    #define _ALIGN_BYTES_32 _ALIGN_POWER_OF_TWO(5)
    #define _ALIGN_BYTES_64 _ALIGN_POWER_OF_TWO(6)
    #define _ALIGN_BYTES_128 _ALIGN_POWER_OF_TWO(7)
    #define _ALIGN_BYTES_256 _ALIGN_POWER_OF_TWO(8)
    #define _ALIGN_BYTES_512 _ALIGN_POWER_OF_TWO(9)

    #define _OPCODE16(OPCODE) _SHORT OPCODE
    #define _OPCODE32(OPCODE) _WORD OPCODE

    #define MOV_S MOV
    #define MVN_S MVN
    #define AND_S AND
    #define EOR_S EOR
    #define ORR_S ORR
    #define BIC_S BIC
    #define ROR_S ROR
    #define LSL_S LSL
    #define LSR_S LSR
    #define ASR_S ASR
    #define ADD_S ADD
    #define ADC_S ADC
    #define SUB_S SUB
    #define SBC_S SBC
    #define NEG_S NEG
    #define MUL_S MUL

    #define APSR_nzcvq APSR_nzcvq

    #define _FILE_END

/******************************** IAR Systems ********************************/
#elif defined(__ICCARM__) || defined(__IASMARM__)
    #define _THUMB THUMB
    #define _THUMB2 THUMB
    #define _SYNTAX_UNIFIED

    /* Labels within a Thumb area have bit 0 set to 1 */
    #define _TYPE(name, ident)

    #define _WORD DCD
    #define _SHORT DC16

    #define _EXPORT PUBLIC
    #define _EXTERN EXTERN

    #define _ALIGN_POWER_OF_TWO(POWER) ALIGN POWER
    #define _LTORG LTORG

    /* reserves block of data initialized to zero in bytes */
    #define _ALLOC_BYTES(N) DS8 N

    #define _DEFINE(NAME,VALUE) NAME: DEFINE VALUE
    #define _SET(NAME,VALUE) NAME EQU VALUE

    #define _SECTION_EXEC(SECTION_NAME) SECTION .SECTION_NAME :CODE
    #define _SECTION_DATA(SECTION_NAME) SECTION .SECTION_NAME :DATA
    #define _SECTION_CONST(SECTION_NAME) SECTION .SECTION_NAME :CONST
    #define _SECTION_EXEC_W(SECTION_NAME) SECTION .SECTION_NAME :DATA
    #define _SECTION_DATA_UNINIT(SECTION_NAME) SECTION .SECTION_NAME :DATA

    #define _SECTION_EXEC_ALIGN(SECTION_NAME, ALIGNMENT) _SECTION_EXEC(SECTION_NAME) (ALIGNMENT)
    #define _SECTION_DATA_ALIGN(SECTION_NAME, ALIGNMENT) _SECTION_DATA(SECTION_NAME) (ALIGNMENT)
    #define _SECTION_CONST_ALIGN(SECTION_NAME, ALIGNMENT) _SECTION_CONST(SECTION_NAME) (ALIGNMENT)
    #define _SECTION_EXEC_W_ALIGN(SECTION_NAME, ALIGNMENT) _SECTION_EXEC_W(SECTION_NAME) (ALIGNMENT)
    #define _SECTION_DATA_UNINIT_ALIGN(SECTION_NAME, ALIGNMENT) _SECTION_DATA_UNINIT(SECTION_NAME) (ALIGNMENT)

    #define _BYTES_2   1
    #define _BYTES_4   2
    #define _BYTES_8   3
    #define _BYTES_16  4
    #define _BYTES_32  5
    #define _BYTES_64  6
    #define _BYTES_128 7
    #define _BYTES_256 8

    #define _ALIGN_BYTES_2 DATA ALIGNROM 1
    #define _ALIGN_BYTES_4 DATA ALIGNROM 2
    #define _ALIGN_BYTES_8 DATA ALIGNROM 3
    #define _ALIGN_BYTES_16 DATA ALIGNROM 4
    #define _ALIGN_BYTES_32 DATA ALIGNROM 5
    #define _ALIGN_BYTES_64 DATA ALIGNROM 6
    #define _ALIGN_BYTES_128 DATA ALIGNROM 7
    #define _ALIGN_BYTES_256 DATA ALIGNROM 8

    #define _OPCODE16(OPCODE) _SHORT OPCODE
    #define _OPCODE32(OPCODE) _WORD OPCODE

    #define MOV_S MOVS
    #define MVN_S MVNS
    #define AND_S ANDS
    #define EOR_S EORS
    #define ORR_S ORRS
    #define BIC_S BICS
    #define ROR_S RORS
    #define LSL_S LSLS
    #define LSR_S LSRS
    #define ASR_S ASRS
    #define ADD_S ADDS
    #define ADC_S ADCS
    #define SUB_S SUBS
    #define SBC_S SBCS
    #define NEG_S NEGS
    #define MUL_S MULS

    #define _FILE_END END
#else
    #error "Unsupported compiler detected!"
#endif

/*==================================================================================================
*                                             ENUMS
==================================================================================================*/


/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/


/*==================================================================================================
*                                GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/


/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/


#ifdef __cplusplus
}
#endif

#endif /*_COMPILER_MACROS_H*/

/** @} */
