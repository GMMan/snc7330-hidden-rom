/**
* Copyright 2020, SONIX Corporation
* All Rights Reserved.
*
* This is UNPUBLISHED PROPRIETARY SOURCE CODE of SONIX Corporation;
* the contents of this file may not be disclosed to third parties, copied
* or duplicated in any form, in whole or in part, without the prior
* written permission of SONIX Corporation.
*/
/** @file snc_load_table.c
*
* @version 1.00
* @date 2020/07/19
* @license
* @description
*/

#include <stdint.h>

//#include ".\ez_config\ez_config.h"
#define CONFIG_FLASH_SIZE (0x1000000)
#define LOAD_TABLE_ADDR (0x60000000)
#define PRAM_LOAD_ADDR (0x60001000)
#define PRAM_CODE_SIZE (0x10000)
#define SRAM_LOAD_ADDR (0)
#define SRAM_CODE_SIZE (0)
#define CORE1_LOAD_ADDR (0)
#define CORE1_CODE_SIZE (0)
#define EXT_LOAD_ADDR (0)
#define EXT_CODE_SIZE (0)
#define ENABLE_ENCRYPTER 0


//=============================================================================
//                                ROM BOOT CONFIG
//=============================================================================

/* #.Load Config (0x08) -------------------------------------------------------------------------*/
typedef struct
{
    union
    {
        uint32_t D;
        struct
        {
            uint32_t    ENCRYPTED_BOOT_CODE :   1;  /* 0- None, 1- Decrypt before bootup */
            uint32_t    CHECK_BOOT_CODE     :   1;  /* 0- None, 1- CRC checksum of SIZE_USERCODE at offset 0x24*/
            uint32_t    CHECK_LOAD_TABLE    :   1;
            uint32_t    NO_SYS_RESET        :   1;
            uint32_t    USE_PLL_SPEEDUP     :   1;  /* if USE PLL 162Mhz */
            uint32_t    USE_EXT_CLK         :   1;  /* 0- Use IHRC PLL 162Mhz, 1- Use HXTAL PLL 162Mhz */
            uint32_t    RESERVED_6          :   2;
            uint32_t    SYS_CLK_MHz         :   8;  /* config sysclk in xMhz, available when pll on */
            uint32_t    MULTIBOOT_MSK       :  12;  /* 0xFFF- Enable priority boot check, else- None */
            uint32_t    RESERVED_28         :   4;
        };
    };
} loadcfg_t;

/* #.PLL connfigurarion (0x18) ------------------------------------------------------------------*/
typedef struct
{
    uint32_t    PLL_INTETGER                :   6;
    uint32_t    PAD0                        :   2;
    uint32_t    PLL_FRACTION                :  20;
    uint32_t    PAD1                        :   4;
} PLL_cfg_t;

/* #.SPIFC configuration (0x60) -----------------------------------------------------------------*/
typedef struct
{
    struct{
        uint32_t    CLK_DIV                 :   2;  /*  0- SYS/1,
                                                     *  1- SYS/2,
                                                     *  2- SYS/4,
                                                     *  3- SYS/8,  */
        uint32_t    READ_TYPE               :   2;  /*  0- use cmd read (03h),
                                                     *  1- use cmd fast read (0Bh),
                                                     *  2- use cmd 2read (BBh),
                                                     *  3- use cmd 4read (EBh),  */
        uint32_t    QUAD_EN_BIT             :   3;  /* Quad_io mode status register bit */
        uint32_t    STAY_QUAD_MODE          :   1;  /* Quad_io mode status register bit */
        
        uint32_t    INIT_STATUS_REG         :   1;
        uint32_t    RESERVED                :  22;
    };
    
    uint32_t        STATUS_REG;                     /* Flash */
    uint32_t        MANUAL_TABLE_ADDR;              /* Load table at "MANUAL_TABLE_ADDR" also supports the priority boot. */
    uint32_t        PAD;
} sfc_set_t;

/* #.NF/SD0/SD1 configuration (0x70) ------------------------------------------------------------*/
typedef struct
{
    struct
    {
        uint32_t    SDNF_DIV            :   2;  /*  0- SYS/1,
                                                 *  1- SYS/2,
                                                 *  2- SYS/4,
                                                 *  3- SYS/8, */
        uint32_t    SD1_DIV             :   1;  /* SD1 CLK = SYS/2/(SD1_DIV+1) */
        uint32_t    CS_PULL_HIGH        :   1;  /* CS pin has internal pull high (effectively in multi-boot) */
        uint32_t    RESERVED3           :   5;
        uint32_t    SD0_DIV             :   8;  /* SD0 CLK = SYS/SDNF_DIV/(SD0_DIV + 2) */
        uint32_t    SD1_SPEED           :   8;
    };
    
    struct
    {
        uint32_t    SPIDIV              :   8;
        uint32_t    SPIPRS              :   3;
        uint32_t    RESERVED1           :  21;
    };
    
    uint32_t        PAD[2];
} sd_nf_set_t;

/* #. Encrypter CFG (0x80) ----------------------------------------------------------------------*/
typedef struct
{
    uint8_t     MARK[4];
    uint32_t    CRC_SUM_EXT;                   /* CRC Checksum of the Ext_code */
    uint32_t    CRC_SUM_DPD;                   /* CRC Checksum of the DPD load code */

    uint32_t    ACTION_FINISH       :   2;     /* Select action after encryption:
                                                  1 - Reset after finish
                                                  else - pending                  */

    uint32_t    IO_PROCESSING       :   8;     /* The GPIO number to instruct processing.
                                                  0 = P0.0, 1 = P0.1 .... 35 = P2.4, else None */

    uint32_t    IO_OK               :   8  ;   /* The GPIO number to instruct encrypt OK.
                                                  0 = P0.0, 1 = P0.1 .... 35 = P2.4, else None */

    uint32_t    IO_FAIL             :   8;     /* The GPIO number to instruct encrypt FAIL.
                                                  0 = P0.0, 1 = P0.1 .... 35 = P2.4, else None */

    uint32_t    IO_ACTIVE_STATE     :   2;     /* The IO polarity when event happends.
                                                  0 - None.
                                                  1 - Active=High, Idle=Low.
                                                  2 - Active=Low , Idle=High. */
    uint32_t                        :   4;

    uint32_t*   ADDR_SRAM_CODE;                 /* Offset: 0x10 */
    uint32_t*   SIZE_SRAM_CODE;                 /* Offset: 0x14 */
    uint32_t*   ADDR_DPD_CODE;                  /* Offset: 0x18 */
    uint32_t*   SIZE_DPD_CODE;                  /* Offset: 0x1C */
} Encrypter_t;

/* #.VerB priority boot (28B) -------------------------------------------------------------------*/
typedef struct
{
    union
    {
        struct
        {
            uint32_t OFFSET;
            char FILE_NAME[24];
        };
        struct
        {
            uint32_t FLASH;
            uint32_t NONE;
        };
    };
} dev_addr_t;

/* #.VerB priority boot (0xD0) ------------------------------------------------------------------*/
typedef struct
{
    uint32_t        DEVICE;                 /*  0x01- FLASH,
                                             *  0x02- SDC1,
                                             *  0x04- NAND,
                                             *  0x08- SPI_NAND,
                                             *  0x10- SDC0,
                                             */
    dev_addr_t      ADDR;                   /* 28 Bytes */
} vb_pri_boot_t;

/* #.Manual Load (0x140)  -----------------------------------------------------------------------*/
typedef struct
{
    uint32_t        DES_ADDR;
    uint32_t        SRC_ADDR;
    uint32_t        SIZE;
    uint32_t        CRC_CHECKSUM;
} load_sec_t;

typedef struct
{
    uint32_t        num;                    /* 1~10 */
    load_sec_t      sec[10];
    uint32_t        PAD[3];
} manual_load_t;

/* #.Information (0x1F0) ------------------------------------------------------------------------*/
typedef struct
{
    uint32_t        BOOT_CODE_VERSION_ADR;  /* Offset:  */
    uint32_t        BOOT_CODE_VERSION;      /* Offset:  */
    
    uint32_t        TABLE_VERSION;          /* Offset:  */
    uint32_t        TABLE_CHK_SUM;          /* Offset:  */
} info_t;


/********************************* Sonix ROM boot config struct **********************************/
typedef struct
{/* SONiX BOOT_ROM information, 0x0 - 0x200 */
    union
    {
        struct
        {
            /* Basic setting - 96 Byte */
            char            MARK[8];                /* Offset: 0x00 - Fixed */
            loadcfg_t       LOAD_CFG;               /* Offset: 0x08 - Fixed */
            uint32_t        RESERVED_0C;            /* Offset: 0x0C */
            
            uint32_t        ADDR_USERCODE;          /* Offset: 0x10 - Fixed */
            uint32_t        SIZE_USERCODE;          /* Offset: 0x14 */
            
            PLL_cfg_t       PLL_CONFIG;             /* Offset: 0x18 */
            uint32_t        RESERVED_1C;            /* Offset: 0x1C */
            uint32_t        FLM_DIS_AUTOBURN;       /* Offset: 0x20 */
            uint32_t        CRC_CHK_SUM;            /* Offset: 0x24 - Fixed */
            
            uint32_t        AES_KEY[8];             /* Offset: 0x28 - Fixed */
            
            uint32_t        RESERVED_48[6];         /* Offset: 0x48 */
            
            /* SPI Flash - 16 Byte */
            sfc_set_t       FLASH_SET;              /* Offset: 0x60 */
            
            /* SDNF - 16 Byte */
            sd_nf_set_t     SDC_NF_CONFIG;          /* Offset: 0x70 */
            
            /* AES Extension - 64 Byte*/
            Encrypter_t     ENCRYPTER;              /* Offset: 0x80 */
            uint32_t        RESERVED_A0[8];         /* Offset: 0xA0 */
            
            /* Boot Priority - 128 Byte */
            vb_pri_boot_t   BOOT_PRIORITY[4];       /* Offset: 0xC0 */
            
            /* MANUAL_LOAD - 176 Byte*/
            manual_load_t   MANUAL_LOAD;            /* Offset: 0x140 */
            
            /* Code version - 16 Byte */
            info_t          INFO;                   /* Offset: 0x1F0 */
        };
        
        uint8_t   size_limit[0x200];
    };
} cfg_rom_t;


//=============================================================================
//                      Type Definitions of sonix fw_info
//=============================================================================

//These item be used to calculate the crc.
typedef struct
{
    uint32_t *DES_ADDR;
    uint32_t *SRC_ADDR;
    uint32_t *SIZE;
    uint32_t CRC_CHECKSUM;
} crc_region;

typedef struct
{
    union
    {
        struct
        {
            /**
             *   Necessary Settings
             *   Default: Load the beginning 64KB of base_fw to the PRAM region and reboot.
             **/
            char        MARK[8];
            char        version_fw[8];
            uint32_t    base_fw;            //Instruct the base address of user PRAM
            uint32_t    status_update;
            
            /* CRC settings */
            uint32_t    en_check_crc;       //Enable checksum if the value is not zero.
            uint32_t    crc_check_sum;      //crc checksum
            crc_region  region[8];          //maximun- 8 regions
            
        };
        
        uint8_t   size_limit[0x1000];
    };
} snx_fw_info;

typedef struct
{/* SONiX boot loader(sw) information, 0x200 - 0x400 */
    union
    {
        struct
        {
            char            MARK[8];
            snx_fw_info     *addr_fw_info[8];
            //Reserve
        };
        
        uint8_t   size_limit[0x200];
    };
} cfg_loader_t;

typedef struct
{/* USB information(sw), 0x400 - 0x600 */
    union
    {
        struct
        {
            uint16_t        vid;
            uint16_t        pid;
            uint8_t         mfrs[50];
            uint8_t         prd[50];
            uint8_t         sn[50];
        };
        
        uint8_t   size_limit[0x200];
    };
} info_usb_sw_t;

typedef struct
{/* USB information(platform), 0x600 - 0x700 */
    union
    {
        struct
        {
            uint32_t        vid                           :  16;      /* Offset: 0x600 */
            uint32_t        pid                           :  16;      /* Offset: 0x602 */
            uint8_t         manufacturers[48];                        /* Offset: 0x604 */
            uint8_t         product[48];                              /* Offset: 0x634 */
            uint8_t         serial_number[48];                        /* Offset: 0x664 */
        };
        
        uint8_t   size_limit[0x100];
    };
} info_usb_plat_t;

typedef struct
{
    uint32_t        DATA_ADDRESS;          /* Offset: 0x800 - Fixed */
    uint32_t        DATA_SIZE;
} data_info_t;

typedef struct
{/* ISP Update struct information, 0x800 - 0x900 */
    union
    {
        struct
        {
            uint32_t        PRAM_ADDRESS;          /* Offset: 0x800 - Fixed */
            uint32_t        PRAM_SIZE;             /* Offset: 0x804 */
            uint32_t        SRAM_ADDRESS;          /* Offset: 0x808 - Fixed */
            uint32_t        SRAM_SIZE;             /* Offset: 0x80C */
            uint32_t        CORE1_ADDRESS;         /* Offset: 0x810 - Fixed */
            uint32_t        CORE1_SIZE;            /* Offset: 0x814 */
            uint32_t        EXT_ADDRESS;           /* Offset: 0x818 - Fixed */
            uint32_t        EXT_SIZE;              /* Offset: 0x81C */
            uint32_t        FLASH_SIZE;            /* Offset: 0x820 - Fixed */
            uint32_t        LOADTABLE_ADDRESS;     /* Offset: 0x824 */
            data_info_t     DATA_INFO[27];         /* Offset: 0x828 ~ 0x8FF */
        };
        
        uint8_t   size_limit[0x100];
    };
} fw_info_t;


//=============================================================================
//                     Type Definitions of SNX Load Table
//=============================================================================
typedef struct
{
    /* From 0x6000_0000 - 0x6000_0200 */
    cfg_rom_t           cfg_rom;

    /* From 0x6000_0200 - 0x6000_0400 */
    cfg_loader_t        cfg_loader;

    /* From 0x6000_0400 - 0x6000_0600, reserved for sw team */
    //info_usb_sw_t       info_usb;
    uint8_t             RES_400[0x200];
    
    /* From 0x6000_0600 - 0x6000_0700 */
    info_usb_plat_t     info_usb;
    
    /* RESERVE */
    uint8_t             RES_700[0x100];
    
    /* From 0x6000_0800 - 0x6000_0900 */
    fw_info_t           fw_info;
    
    /* RESERVE, PADDING until 0x1000(4KB) */
    uint8_t             PAD[0x700];
} load_table_t;


const load_table_t load_table __attribute((used)) =
{
    .cfg_rom =
    {/* SONiX BOOT_ROM information, 0x0 - 0x200 */
        // * --- Sonix 7330 rom code identify tag ---
        .MARK                       = "SONIXDEV",       /* SNC73XX Series */
        .INFO.TABLE_VERSION         = 0x5A5A0033,       /* LoadTable Version */
        .INFO.TABLE_CHK_SUM         = 0xFFFFFFFF,       /* tool will replace CRC checksum */
      
        // * --- User Code loading information ---
        .ADDR_USERCODE              = PRAM_LOAD_ADDR,   /* Pram binary file start address */
        .SIZE_USERCODE              = PRAM_CODE_SIZE,   /* Pram binary file maximun size */
        
        // * --- Load configuration ---
        .LOAD_CFG.USE_PLL_SPEEDUP   = 1,
        .FLASH_SET.CLK_DIV          = 2,
        .FLASH_SET.READ_TYPE        = 1,
        .SDC_NF_CONFIG.SD1_SPEED    = 2,
        .LOAD_CFG.NO_SYS_RESET      = 1,
        
        // * --- CRC Setting ---
        #if (ENABLE_ENCRYPTER == 1)                     /* AES Decryption Setting  */
            .LOAD_CFG.ENCRYPTED_BOOT_CODE = 1,          /* Set .LOAD_CFG.ENCRYPTED_BOOT_CODE = 1 to enable decrypt Boot */
        #else
            .LOAD_CFG.ENCRYPTED_BOOT_CODE = 0,
        #endif
        
        .LOAD_CFG.CHECK_BOOT_CODE   = 0,
        .CRC_CHK_SUM                = 0xFFFFFFFF,       /* tool will replace CRC checksum */
        
        // * --- Encryption Settings ---
        .ENCRYPTER =
        {
            #if (ENABLE_ENCRYPTER == 1)
                .MARK           = "EN__",               /* Set .MARK = "EN__" to trigger the encryption mode */
            #endif
            
            /**
             *   Enable the .ENCRYPTER setting allows BOOT_ROM to encrypt/decrypt PRAM data over 64KB
             *   by using additional SRAM as the code base instead of scatter_load by __main.
             *   
             *   BOOT_ROM decrypts the ADDR_SRAM_CODE from flash to specified SRAM (0x18000000 + SIZE_SRAM_CODE) during power-on reset;
             *   The ADDR_SRAM_CODE scatter setting is descript in the linker.sct.
             **/

            .ADDR_SRAM_CODE  = (void*)SRAM_LOAD_ADDR,    /* Align with 0x1000 (scatter file) */
            .SIZE_SRAM_CODE  = (void*)SRAM_CODE_SIZE,    /* Align with 0x1000 (scatter file) Maximun = 0x18000 */

            .ACTION_FINISH   = 1,                        /* 1    - Do System reset if success,
                                                            else - pending */
            .IO_PROCESSING   = 99,                       /* 0~36 - IO from P0.0 to P2.3, else none */
            .IO_OK           = 99,                       /* 0~36 - IO from P0.0 to P2.3, else none */
            .IO_FAIL         = 99,                       /* 0~36 - IO from P0.0 to P2.3, else none */
            .IO_ACTIVE_STATE = 1,                        /* 0 - None,
                                                            1 - (Active=High, Idle=Low),
                                                            2 - (Active=Low , Idle=High). */
            .CRC_SUM_EXT     = 0xFFFFFFFF,               /* tool will replace CRC checksum */
        },
    },
    
    .cfg_loader =
    {/* SONiX boot loader(sw) information, 0x200 - 0x400 */
        0
    },
    
    .info_usb =
    {/* USB information(platform), 0x600 - 0x700 */
        .vid = 0x0C45,
        .pid = 0x7300,
        .manufacturers = "Sonix Technology Co., Ltd.",
    },
    
    .fw_info =
    {/* ISP Update struct information, 0x800 - 0x900 */
        /* PRAM code */
        .PRAM_ADDRESS   = PRAM_LOAD_ADDR,
        .PRAM_SIZE      = PRAM_CODE_SIZE,
        
        /* SRAM code */
        .SRAM_ADDRESS   = SRAM_LOAD_ADDR,
        .SRAM_SIZE      = SRAM_CODE_SIZE,
        
        /* CORE_1 code */
        .CORE1_ADDRESS  = CORE1_LOAD_ADDR,
        .CORE1_SIZE     = CORE1_CODE_SIZE,
        
        /* User Ext code */
        .EXT_ADDRESS    = EXT_LOAD_ADDR,
        .EXT_SIZE       = EXT_CODE_SIZE,
        
        .FLASH_SIZE                 = CONFIG_FLASH_SIZE,
        .LOADTABLE_ADDRESS          = LOAD_TABLE_ADDR,
		
        .DATA_INFO[0].DATA_ADDRESS  = 0,
        .DATA_INFO[0].DATA_SIZE     = 0,
    },
};

