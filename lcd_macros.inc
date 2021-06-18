#ifndef __LCD_HIGH_INC__
#define __LCD_HIGH_INC__

; --------------------------------------------------
; macros

_LCD_CHECK_ADDR_DEF MACRO
  #ifndef LCD_ADDR
    error "LCD_ADDR must be defined as the I2C address of the LCD"
  #endif
  _I2C_CHECK_ADDR7  (addr)
    ENDM

LCD_LOAD_ADDR   MACRO addr
        I2C_CHECK_ADDR7 (addr)
        STATUS_SAVE _lcd_shr
        pagesel _LCD_PAGE
        banksel _LCD_MEM
        movlw   ((addr) << 1) & 0xFE
        movwf   _lcd_addr
        STATUS_RSTR_FLG _lcd_shr
    ENDM

LCD_LOAD_ADDR_W MACRO
        STATUS_SAVE _lcd_shr
        pagesel _LCD_PAGE
        banksel _LCD_MEM
        bcf     STATUS, Z
        movwf   _lcd_addr
        rlf     _lcd_addr, F
        STATUS_RSTR _lcd_shr
    ENDM

LCD_INIT    MACRO   addr
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        ; banksel _LCD_MEM done in _lcd_init
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_init
        ; _lcd_init restores STATUS
        pagesel $
    ENDM

; --------------------------------------------------
; command macros

; send LCD command literal `lcmd` to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_CMD_L   MACRO   lcmd
        pagesel _LCD_PAGE
        movlw   (lcmd)
        ; banksel _LCD_MEM done in call
        call    _lcd_write_byte_w_cmd_ss
        pagesel $
    ENDM

; send LCD command byte at `bcmd` to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_CMD_B   MACRO   bcmd
        pagesel _LCD_PAGE
        movf    (bcmd),W
        ; banksel _LCD_MEM done in call
        call    _lcd_write_byte_w_cmd_ss
        pagesel $
    ENDM

; send LCD command held in W to LCD at `addr`
; preserves STATUS
LCD_CMD_W  MACRO
        pagesel _LCD_PAGE
        call    _lcd_write_byte_w_cmd_ss
        pagesel $
    ENDM

; send LCD command pointed by IRP:FSR to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_CMD_I   MACRO   addr
        pagesel _LCD_PAGE
        call    _lcd_write_byte_i_cmd_ss
        pagesel $
    ENDM

; send LCD command at `cmd_ptr` to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_CMD_P   MACRO   cmd_ptr
        pagesel _LCD_PAGE
        bankisel  (cmd_ptr)
        movlw   (cmd_ptr)
        call    _lcd_write_byte_p_cmd_ss
        pagesel $
    ENDM

; send LCD command literal `lcmd` to LCD at `addr`
; preserves STATUS
LCDA_CMD_L  MACRO   addr, lcmd
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_cmd_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        movlw   (lcmd)
        call    _lcd_write_byte_w
        pagesel $
    ENDM

; send LCD command byte at `bcmd` to LCD at `addr`
; preserves STATUS
LCDA_CMD_B  MACRO   addr, bcmd
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_cmd_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        movf    (bcmd),W
        call    _lcd_write_byte_w
        pagesel $
    ENDM

; send LCD command held in W to LCD at `addr`
; preserves STATUS
LCDA_CMD_W MACRO   addr
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        call    _lcd_cmd_byte_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_write_byte_a
        pagesel $
    ENDM

; send LCD command held pointed by IRP:FSR to LCD at `addr`
; preserves STATUS
LCDA_CMD_I  MACRO   addr
        I2C_CHECK_ADD7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_write_byte_i_cmd_ss       ; STATUS saved in routine
        pagesel $
    ENDM

; send LCD command at `cmd_ptr` to LCD at `addr`
; preserves STATUS
LCDA_CMD_P  MACRO   addr, cmd_ptr
        I2C_CHECK_ADD7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_cmd_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        bankisel  (cmd_ptr)
        movlw   (cmd_ptr)
        call    _lcd_write_byte_p
        pagesel $
    ENDM

; --------------------------------------
; char macros

; send literal character `char` to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_CHAR_L  MACRO   lchar
        pagesel _LCD_PAGE
        movlw   (lchar)
        call    _lcd_write_byte_w_data_ss
        pagesel $
    ENDM

; send character byte at `bchar` to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_CHAR_B  MACRO   bchar
        pagesel _LCD_PAGE
        movf   (bchar),W
        call    _lcd_write_byte_w_data_ss
        pagesel $
    ENDM

; send character byte held in W to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_CHAR_W  MACRO
        pagesel _LCD_PAGE
        call    _lcd_write_byte_w_data_ss
        pagesel $
    ENDM

; send character pointed by IRP:FSR to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_CHAR_I  MACRO   addr
        pagesel _LCD_PAGE
        call    _lcd_write_byte_i_data_ss
        pagesel $
    ENDM

; send character at `char_ptr` to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_CHAR_P  MACRO   char_ptr
        pagesel _LCD_PAGE
        bankisel  (char_ptr)
        movlw   (char_ptr)
        call    _lcd_write_byte_p_data_ss
        pagesel $
    ENDM

; send character literal `lchar` to LCD at `addr`
; preserves STATUS
LCDA_CHAR_L   MACRO   addr, lchar
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_data_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        movlw   (lchar)
        call    _lcd_write_byte_w
        pagesel $
    ENDM

; send character byte at `bchar` to LCD at `addr`
; preserves STATUS
LCDA_CHAR_B   MACRO   addr, bchar
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_data_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        movf    (bchar),W
        call    _lcd_write_byte_w
        pagesel $
    ENDM

; send character held in W to LCD at `addr`
; preserves STATUS
LCDA_CHAR_W MACRO   addr
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        call    _lcd_data_byte_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_write_byte_a
        pagesel $
    ENDM

; send character pointed by IRP:FSR to LCD at `addr`
; preserves STATUS
LCDA_CHAR_I MACRO   addr
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_write_byte_i_data_ss     ; STATUS saved in routine
        pagesel $
    ENDM

; send character at `char_ptr` to LCD at `addr`
; preserves STATUS
LCDA_CHAR_P MACRO   addr, char_ptr
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_data_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        bankisel  (char_ptr)
        movlw   (char_ptr)
        call    _lcd_write_byte_p
        pagesel $
    ENDM

; --------------------------------------
; string macros

; send string pointed by IRP:FSR to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_STR_I    MACRO   len
        pagesel _LCD_PAGE
        movlw   (len)
        call    _lcd_write_buf_l_ss
        pagesel $
    ENDM

; send string at `str` with length `len` to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_STR_P    MACRO   str, len
        pagesel _LCD_PAGE
        movlw   (len)
        call     _lcd_data_len_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        bankisel  (str)
        movlw   (str)
        call    _lcd_write_buf_f
        pagesel $
    ENDM

; send C-style string pointed by IRP:FSR to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_STR_IC    MACRO
        pagesel _LCD_PAGE
        call    _lcd_write_bufc_ss
        pagesel $
    ENDM

; send C-style string at `str` to LCD at preloaded address (_lcd_addr)
; preserves STATUS
LCD_STR_PC    MACRO   str
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_data_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        bankisel  (str)
        movlw   (str)
        call    _lcd_write_bufc_f
        pagesel $
    ENDM

; send string pointed by IRP:FSR to LCD at LCD_ADDR
; preserves STATUS
LCDA_STR_I   MACRO   addr, len
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_data_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        movlw   (len)
        call    _lcd_write_buf_l
        pagesel $
    ENDM

; send string at `str` with length `len` to LCD at `addr`
; preserves STATUS
LCDA_STR_P   MACRO   addr, str, len
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_data_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        bankisel  (str)
        movlw   (str)
        movwf   FSR

        movlw   (len)
        call    _lcd_write_buf_l
        pagesel $
    ENDM

; send C-style string pointed by IRP:FSR to LCD at `addr`
; preserves STATUS
LCDA_STR_IC   MACRO   addr, str
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_data_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        call    _lcd_write_bufc
        pagesel $
    ENDM

; send C-style string at `str` to LCD at `addr`
; preserves STATUS
LCDA_STR_PC   MACRO   addr, str
        I2C_CHECK_ADDR7 (addr)
        pagesel _LCD_PAGE
        movlw   ((addr) << 1) & 0xFE
        call    _lcd_data_addr_setup_ss
        ; STATUS now saved and _LCD_BANK selected
        bankisel  (str)
        movlw   (str)
        call    _lcd_write_bufc_f
        pagesel $
    ENDM

; ---------------------------------
; LCD control macros

LCD_CLEAR       MACRO
        LCD_CMD_L   LCD_CLEAR_CMD
        pagesel _LCD_PAGE
        call _lcd_delay_1500_us
        pagesel $
    ENDM

; hd44780 display-control cmd related

LCD_DISP_ON         MACRO
        pagesel _LCD_PAGE
        movlw   LCD_DISP_EN
        call  _lcd_disp_ctrl_or_ss
        pagesel $
    ENDM

LCD_DISP_OFF        MACRO
        pagesel _LCD_PAGE
        movlw   ~LCD_DISP_EN
        call  _lcd_disp_ctrl_and_ss
        pagesel $
    ENDM

LCD_SHOW_CURS       MACRO
        pagesel _LCD_PAGE
        movlw   LCD_DISP_CURS
        call  _lcd_disp_ctrl_or_ss
        pagesel $
    ENDM

LCD_HIDE_CURS       MACRO
        pagesel _LCD_PAGE
        movlw   ~LCD_DISP_CURS
        call  _lcd_disp_ctrl_and_ss
        pagesel $
    ENDM

LCD_BLINK_ON        MACRO
        pagesel _LCD_PAGE
        movlw   LCD_DISP_BLINK
        call  _lcd_disp_ctrl_or_ss
        pagesel $
    ENDM

LCD_BLINK_OFF       MACRO
        pagesel _LCD_PAGE
        movlw   ~LCD_DISP_BLINK
        call  _lcd_disp_ctrl_and_ss
        pagesel $
    ENDM

; hd44780 entry-mode cmd related

LCD_ENTRY_SHIFT_ON  MACRO
        pagesel _LCD_PAGE
        movlw   LCD_ENTRY_SHIFT
        call  _lcd_entry_ctrl_or_ss
        pagesel $
    ENDM

LCD_ENTRY_SHIFT_OFF MACRO
        pagesel _LCD_PAGE
        movlw   ~LCD_ENTRY_SHIFT
        call  _lcd_entry_ctrl_and_ss
        pagesel $
    ENDM

LCD_ENTRY_SET_INC   MACRO
        pagesel _LCD_PAGE
        movlw   LCD_ENTRY_INC
        call  _lcd_entry_ctrl_or_ss
        pagesel $
    ENDM

LCD_ENTRY_SET_DEC   MACRO
        pagesel _LCD_PAGE
        movlw   ~LCD_ENTRY_INC
        call    _lcd_entry_ctrl_and_ss
        pagesel $
    ENDM

; hd44780 shift-control cmd related

LCD_SHIFT_DISP_L    MACRO
        pagesel _LCD_PAGE
        movlw   (LCD_SHIFT_CMD | LCD_SHIFT_DISP)
        call    _lcd_shift_ctrl_set_ss
        pagesel $
    ENDM

LCD_SHIFT_DISP_R    MACRO
        pagesel _LCD_PAGE
        movlw   (LCD_SHIFT_CMD | LCD_SHIFT_DISP | LCD_SHIFT_RIGHT)
        call    _lcd_shift_ctrl_set_ss
        pagesel $
    ENDM

LCD_SHIFT_CURS_L    MACRO
        pagesel _LCD_PAGE
        movlw   LCD_SHIFT_CMD
        call    _lcd_shift_ctrl_set_ss
        pagesel $
    ENDM

LCD_SHIFT_CURS_R    MACRO
        pagesel _LCD_PAGE
        movlw   (LCD_SHIFT_CMD | LCD_SHIFT_RIGHT)
        call    _lcd_shift_ctrl_set_ss
        pagesel $
    ENDM

LCD_CHECK_POS       MACRO   col, row
  if (0 > (col)) || ((col) >= LCD_COLS)
    error "LCD column maximum exceeded, check LCD_COLS"
    exitm
  endif
  if (0 > (row)) || ((row) >= LCD_ROWS)
    error "LCD row maximum exceeded, check LCD_ROWS"
    exitm
  endif
    ENDM

LCD_SET_POS         MACRO   col, row
  LCD_CHECK_POS (col), (row)

  ; 20x4 LCD:
  ; row 0: 0x00 offset
  ; row 1: 0x40 offset
  ; row 2: 0x14 offset
  ; row 3: 0x54 offset

  local row_offset = (((row) % 2) * 0x40) + (((row) / 2) * 0x14)
  local c_addr = row_offset + (col)

        LCD_CMD_L (LCD_SET_DDRAM_CMD + c_addr)
    ENDM

LCDA_SET_POS        MACRO   addr, col, row
  LCD_CHECK_POS (col), (row)

  ; 20x4 LCD:
  ; row 0: 0x00 offset
  ; row 1: 0x40 offset
  ; row 2: 0x14 offset
  ; row 3: 0x54 offset

  local row_offset = (((row) % 2) * 0x40) + (((row) / 2) * 0x14)
  local c_addr = row_offset + (col)
        LCDA_CMD (addr), (LCD_SET_DDRAM_CMD + c_addr)
    ENDM

#endif ; __LCD_HIGH_INC__


