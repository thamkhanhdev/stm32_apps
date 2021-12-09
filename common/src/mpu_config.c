#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                         INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include <stdint.h>
#include "core_cm7.h"

/*==================================================================================================
*                                        LOCAL MACROS
==================================================================================================*/
#define MPU_RASR_XN(i)                  (((uint32_t)(i) & 0x01UL) << 28U)
#define MPU_RASR_AP(i)                  (((uint32_t)(i) & 0x07UL) << 24U)
#define MPU_RASR_TEX(i)                 (((uint32_t)(i) & 0x07UL) << 19U)
#define MPU_RASR_S(i)                   (((uint32_t)(i) & 0x01UL) << 18U)
#define MPU_RASR_C(i)                   (((uint32_t)(i) & 0x01UL) << 17U)
#define MPU_RASR_B(i)                   (((uint32_t)(i) & 0x01UL) << 16U)
#define MPU_RASR_SRD(i)                 (((uint32_t)(i) & 0xFFUL) <<  8U)
#define MPU_RASR_SIZE(i)                (((uint32_t)(i) & 0x1FUL) <<  1U)
#define MPU_RASR_EN(i)                  (((uint32_t)(i) & 0x01UL) <<  0U)

#define MPU_RASR_MEM_EXECUTE_NEVER      (MPU_RASR_XN(1))

#define MPU_RASR_AP_PRIV_NO_USR_NO      (MPU_RASR_AP(0U))
#define MPU_RASR_AP_PRIV_RW_USR_NO      (MPU_RASR_AP(1U))
#define MPU_RASR_AP_PRIV_RW_USR_RO      (MPU_RASR_AP(2U))
#define MPU_RASR_AP_PRIV_RW_USR_RW      (MPU_RASR_AP(3U))
#define MPU_RASR_AP_PRIV_RO_USR_NO      (MPU_RASR_AP(5U))
#define MPU_RASR_AP_PRIV_RO_USR_RO      (MPU_RASR_AP(6U))

#define MPU_RASR_MEM_STRONGLY_ORDERED   (MPU_RASR_TEX(0U) | MPU_RASR_C(0U) | MPU_RASR_B(0U))    /* Shared strongly ordered memory */
#define MPU_RASR_MEM_DEVICE_SHARED      (MPU_RASR_TEX(0U) | MPU_RASR_C(0U) | MPU_RASR_B(1U))    /* Shared device memory */
#define MPU_RASR_MEM_NORMAL_WT          (MPU_RASR_TEX(0U) | MPU_RASR_C(1U) | MPU_RASR_B(0U))    /* Outer and inner write-through */
#define MPU_RASR_MEM_NORMAL_WB_NO_WA    (MPU_RASR_TEX(0U) | MPU_RASR_C(1U) | MPU_RASR_B(0U))    /* Outer and inner write back. No write-allocate */
#define MPU_RASR_MEM_NORMAL_NO_CACHE    (MPU_RASR_TEX(1U) | MPU_RASR_C(0U) | MPU_RASR_B(0U))    /* Outer and inner non-cacheable */
#define MPU_RASR_MEM_NORMAL_WB_RA_WA    (MPU_RASR_TEX(1U) | MPU_RASR_C(1U) | MPU_RASR_B(1U))    /* Outer and inner write-back. Read-allocate, write-allocate. */
#define MPU_RASR_MEM_DEVICE_NONSHARED   (MPU_RASR_TEX(2U) | MPU_RASR_C(0U) | MPU_RASR_B(0U))    /* Non-shared device memory */

#define MPU_RASR_SIZE_4KB              (MPU_RASR_SIZE(11U))
#define MPU_RASR_SIZE_8KB              (MPU_RASR_SIZE(12U))
#define MPU_RASR_SIZE_16KB              (MPU_RASR_SIZE(13U))
#define MPU_RASR_SIZE_32KB              (MPU_RASR_SIZE(14U))
#define MPU_RASR_SIZE_64KB              (MPU_RASR_SIZE(15U))
#define MPU_RASR_SIZE_128KB             (MPU_RASR_SIZE(16U))
#define MPU_RASR_SIZE_256KB             (MPU_RASR_SIZE(17U))
#define MPU_RASR_SIZE_512KB             (MPU_RASR_SIZE(18U))
#define MPU_RASR_SIZE_1MB               (MPU_RASR_SIZE(19U))
#define MPU_RASR_SIZE_2MB               (MPU_RASR_SIZE(20U))
#define MPU_RASR_SIZE_4MB               (MPU_RASR_SIZE(21U))
#define MPU_RASR_SIZE_8MB               (MPU_RASR_SIZE(22U))
#define MPU_RASR_SIZE_128MB             (MPU_RASR_SIZE(26U))
#define MPU_RASR_SIZE_256MB             (MPU_RASR_SIZE(27U))
#define MPU_RASR_SIZE_512MB             (MPU_RASR_SIZE(28U))
#define MPU_RASR_SIZE_1GB               (MPU_RASR_SIZE(29U))
#define MPU_RASR_SIZE_2GB               (MPU_RASR_SIZE(30U))
#define MPU_RASR_SIZE_4GB               (MPU_RASR_SIZE(31U))

#define MPU_RASR_MEM_SHAREABLE          (MPU_RASR_S(1))
#define MPU_RASR_MEM_NON_SHAREABLE      (MPU_RASR_S(0))

#define MPU_RASR_REGION_ENABLED         (MPU_RASR_EN(1U))
#define MPU_RASR_REGION_DISABLED        (MPU_RASR_EN(0U))

/*==================================================================================================
*                                      FILE VERSION CHECKS
==================================================================================================*/

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                       GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                       GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Get size exponent from size
 * @param u32Size   Memory region size
 *
 * @return
 */
static uint32_t mpuGetSizeExp(uint32_t u32Size)
{
    uint32_t u32Exp;

    __asm("CLZ %0,%1" : "=r"(u32Exp) : "r"(u32Size) : );
    u32Exp = (31UL - u32Exp) & 0x1FUL;

    return u32Exp;
}

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
/**
 * @brief mpuInit
 * @details MPU initialization
 */
void mpuInit(void)
{
    uint32_t u32MpuType = SCB.MPU_TYPE;
    uint32_t u32RegionNum = (u32MpuType >> 8U) & 0xFF;
    uint32_t i = 0UL;

    /* Program MPU Region - ITCM
     * Base = 0x00000000
     * Limit = 0x0000FFFF
     * Size = 64kB
     * SH = 0   ; Non-shareable
     * AP = 3   ; Read-write priv/user.
     * XN = 1   ; Execute never
     * EN = 1   ; Region enabled
     */
    SCB.MPU_RNR = i++;
    SCB.MPU_RBAR = 0x0UL;
    SCB.MPU_RASR  = MPU_RASR_AP_PRIV_RW_USR_RW |
                    MPU_RASR_MEM_NORMAL_WT |
                    MPU_RASR_MEM_NON_SHAREABLE |
                    MPU_RASR_SIZE_64KB |
                    MPU_RASR_MEM_EXECUTE_NEVER |
                    MPU_RASR_REGION_ENABLED;

    /* Program MPU Region - FLASH
     * Base = 0x08000000
     * Limit = 0x081FFFFF
     * Size = 2MB
     * SH = 0   ; Non-shareable
     * AP = 1   ; Read-only priv/user.
     * XN = 0   ; Execute
     * EN = 1   ; Region enabled
     */
    SCB.MPU_RNR = i++;
    SCB.MPU_RBAR = 0x8000000UL;
    SCB.MPU_RASR  = MPU_RASR_AP_PRIV_RO_USR_RO |
                    MPU_RASR_MEM_NORMAL_WT |
                    MPU_RASR_MEM_NON_SHAREABLE |
                    MPU_RASR_SIZE_2MB |
                    MPU_RASR_REGION_ENABLED;

    /* Program MPU Region - DTCM
     * Base = 0x20000000
     * Limit = 0x2001FFFF
     * Size = 128kB
     * SH = 0   ; Non-Shareable
     * AP = 3   ; Read-write priv/user.
     * XN = 1   ; Execute never
     * EN = 1   ; Region enabled
     */
    SCB.MPU_RNR = i++;
    SCB.MPU_RBAR = 0x20000000UL;
    SCB.MPU_RASR  = MPU_RASR_AP_PRIV_RW_USR_RW |
                    MPU_RASR_MEM_NORMAL_WB_RA_WA |
                    MPU_RASR_MEM_NON_SHAREABLE |
                    MPU_RASR_SIZE_128KB |
                    MPU_RASR_MEM_EXECUTE_NEVER |
                    MPU_RASR_REGION_ENABLED;

    /* Program MPU Region - AXI SRAM
     * Base = 0x24000000
     * Limit = 0x2407FFFF
     * Size = 512kB
     * SH = 0   ; Non-Shareable
     * AP = 3   ; Read-write priv/user.
     * XN = 1   ; Execute never
     * EN = 1   ; Region enabled
     */
    SCB.MPU_RNR = i++;
    SCB.MPU_RBAR = 0x24000000UL;
    SCB.MPU_RASR  = MPU_RASR_AP_PRIV_RW_USR_RW |
                    MPU_RASR_MEM_NORMAL_WB_RA_WA |
                    MPU_RASR_MEM_NON_SHAREABLE |
                    MPU_RASR_SIZE_512KB |
                    MPU_RASR_MEM_EXECUTE_NEVER |
                    MPU_RASR_REGION_ENABLED;

    /* Program MPU Region - SRAM1
     * Base = 0x30000000
     * Limit = 0x3001FFFF
     * Size = 128kB
     * SH = 0   ; Non-Shareable
     * AP = 3   ; Read-write priv/user.
     * XN = 1   ; Execute never
     * EN = 1   ; Region enabled
     */
    SCB.MPU_RNR = i++;
    SCB.MPU_RBAR = 0x30000000UL;
    SCB.MPU_RASR  = MPU_RASR_AP_PRIV_RW_USR_RW |
                    MPU_RASR_MEM_NORMAL_WB_RA_WA |
                    MPU_RASR_MEM_NON_SHAREABLE |
                    MPU_RASR_SIZE_128KB |
                    MPU_RASR_MEM_EXECUTE_NEVER |
                    MPU_RASR_REGION_ENABLED;

    /* Program MPU Region - SRAM2
     * Base = 0x30020000
     * Limit = 0x3003FFFF
     * Size = 128kB
     * SH = 0   ; Non-Shareable
     * AP = 3   ; Read-write priv/user.
     * XN = 1   ; Execute never
     * EN = 1   ; Region enabled
     */
    SCB.MPU_RNR = i++;
    SCB.MPU_RBAR = 0x30020000UL;
    SCB.MPU_RASR  = MPU_RASR_AP_PRIV_RW_USR_RW |
                    MPU_RASR_MEM_NORMAL_WB_RA_WA |
                    MPU_RASR_MEM_NON_SHAREABLE |
                    MPU_RASR_SIZE_128KB |
                    MPU_RASR_MEM_EXECUTE_NEVER |
                    MPU_RASR_REGION_ENABLED;

    /* Program MPU Region - SRAM3
     * Base = 0x30040000
     * Limit = 0x30047FFF
     * Size = 32kB
     * SH = 0   ; Non-Shareable
     * AP = 3   ; Read-write priv/user.
     * XN = 1   ; Execute never
     * EN = 1   ; Region enabled
     */
    SCB.MPU_RNR = i++;
    SCB.MPU_RBAR = 0x30040000UL;
    SCB.MPU_RASR  = MPU_RASR_AP_PRIV_RW_USR_RW |
                    MPU_RASR_MEM_NORMAL_WB_RA_WA |
                    MPU_RASR_MEM_NON_SHAREABLE |
                    MPU_RASR_SIZE_32KB |
                    MPU_RASR_MEM_EXECUTE_NEVER |
                    MPU_RASR_REGION_ENABLED;

    /* Program MPU Region - SRAM4
     * Base = 0x38000000
     * Limit = 0x3800FFFF
     * Size = 64kB
     * SH = 0   ; Non-Shareable
     * AP = 3   ; Read-write priv/user.
     * XN = 1   ; Execute never
     * EN = 1   ; Region enabled
     */
    SCB.MPU_RNR = i++;
    SCB.MPU_RBAR = 0x38000000UL;
    SCB.MPU_RASR  = MPU_RASR_AP_PRIV_RW_USR_RW |
                    MPU_RASR_MEM_NORMAL_WB_RA_WA |
                    MPU_RASR_MEM_NON_SHAREABLE |
                    MPU_RASR_SIZE_64KB |
                    MPU_RASR_MEM_EXECUTE_NEVER |
                    MPU_RASR_REGION_ENABLED;

    /* Program MPU Region - Backup SRAM
     * Base = 0x38800000
     * Limit = 0x38800FFF
     * Size = 4kB
     * SH = 0   ; Non-Shareable
     * AP = 3   ; Read-write priv/user.
     * XN = 1   ; Execute never
     * EN = 1   ; Region enabled
     */
    SCB.MPU_RNR = i++;
    SCB.MPU_RBAR = 0x38800000UL;
    SCB.MPU_RASR  = MPU_RASR_AP_PRIV_RW_USR_RW |
                    MPU_RASR_MEM_NORMAL_WB_RA_WA |
                    MPU_RASR_MEM_NON_SHAREABLE |
                    MPU_RASR_SIZE_4KB |
                    MPU_RASR_MEM_EXECUTE_NEVER |
                    MPU_RASR_REGION_ENABLED;

    /* Program MPU Region - Peripherals
     * Base = 0x40000000
     * Limit = 0x60000000
     * Size = 512MB
     * SH = 1   ; Shareable
     * AP = 3   ; Read-write priv/user.
     * XN = 1   ; Execute never
     * EN = 1   ; Region enabled
     */
    SCB.MPU_RNR = i++;
    SCB.MPU_RBAR = 0x40000000UL;
    SCB.MPU_RASR  = MPU_RASR_AP_PRIV_RW_USR_RW |
                    MPU_RASR_MEM_STRONGLY_ORDERED |
                    MPU_RASR_MEM_DEVICE_SHARED |
                    MPU_RASR_MEM_EXECUTE_NEVER |
                    MPU_RASR_SIZE_512MB |
                    MPU_RASR_REGION_ENABLED;

    /* Disable all other regions */
    for( ; i < u32RegionNum; i++ )
    {
        SCB.MPU_RNR = i;
        SCB.MPU_RBAR = 0UL;
        SCB.MPU_RASR = MPU_RASR_REGION_DISABLED;
    }

    SCB.MPU_CTRL = 7UL;  /* PRIVDEFENA, HFNMIENA, ENABLE */
    __asm
    (
        "ISB SY\n"
        "DSB SY\n"
    );
}

#ifdef __cplusplus
}
#endif
