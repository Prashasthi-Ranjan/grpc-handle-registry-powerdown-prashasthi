#pragma once
// Hard boundary v5 - includes FlatMem, ModuleState, PowerClass, MaxPower per dashboard commits
#define LOWER_IDENTIFIER 0
#define LOWER_FLAT_MEM_OFFSET 2
#define LOWER_FLAT_MEM_BIT 7
#define LOWER_MODULE_STATE_OFFSET 3
#define LOWER_MODULE_STATE_MASK 0x0E
#define LOWER_MODULE_STATE_SHIFT 1
#define LOWER_MEDIA_TYPE 85
#define LOWER_MEDIA_INTERFACE 86

#define PAGE_00H_SIZE 128
#define PAGE_00H_VENDOR_NAME_OFFSET 1
#define PAGE_00H_VENDOR_NAME_LEN 16
#define PAGE_00H_VENDOR_OUI_OFFSET 17
#define PAGE_00H_VENDOR_OUI_LEN 3
#define PAGE_00H_VENDOR_PN_OFFSET 20
#define PAGE_00H_VENDOR_PN_LEN 16
#define PAGE_00H_VENDOR_SN_OFFSET 36
#define PAGE_00H_VENDOR_SN_LEN 16
#define PAGE_00H_POWER_OFFSET 72
#define PAGE_00H_POWER_CLASS_MASK 0xE0
#define PAGE_00H_POWER_CLASS_SHIFT 5
#define PAGE_00H_MAX_POWER_MASK 0x1F

#define PAGE_01H_SIZE 128
#define PAGE_01H_HOST_LANE_COUNT 14
#define PAGE_01H_MEDIA_LANE_COUNT 15
#define PAGE_01H_HOST_ASSIGN_MASK 20
#define PAGE_01H_MEDIA_ASSIGN_MASK 21
#define PAGE_01H_SUPPORTED_PAGES_MASK 22
#define PAGE_01H_CDB_MASK 23
#define PAGE_01H_DURATION_BYTE 39

// Table 8-46
#define HAS_PAGE_10H (1<<0)
#define HAS_PAGE_11H (1<<1)
#define HAS_PAGE_12H (1<<2)
#define HAS_PAGE_14H (1<<3)
#define HAS_PAGE_20H (1<<4)
#define HAS_PAGE_16H (1<<5)
#define HAS_PAGE_04H (1<<6)

// Table 8-54 CDB
#define CDB_SUPPORTED (1<<0)
#define CDB_BG_MASK 0x06
#define CDB_BG_SHIFT 1
#define CDB_FULL_PAGE_READ (1<<3)

// Vendor - Avocado weak: AsciiTrim trailing spaces/nulls ONLY, FormatOUI uppercase colon
// Power - Avocado weak: PowerClass raw 0..7 +1 => 1..8, MaxPower raw 0..31 *100
