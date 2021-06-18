#ifndef __LCD_INC__
#define __LCD_INC__

; HD44780 compatible LCD via I2C 8-bit expander support
;
;   * compatible with HD44780 LCD controller and most clones
;   * compatible with common 8-bit I2C expanders 'backpacks' including
;     TI PCF8574 and Microchip MCP23008

#include "deps/util.inc"

#ifndef LCD_BANK
  #define LCD_BANK 1
#endif

  if (0 > LCD_BANK || LCD_BANK > 3)
    error "invalid bank selected"
  endif

  if LCD_BANK == 0
    #define _LCD_BANK LCD_BANK0
  endif
  if LCD_BANK == 1
    #define _LCD_BANK LCD_BANK1
  endif
  if LCD_BANK == 2
    #define _LCD_BANK LCD_BANK0
  endif
  if LCD_BANK == 3
    #define _LCD_BANK LCD_BANK1
  endif
; e.g. use with the following in .lkr script:
; SECTION    NAME=LCD_BANK0     RAM=gpr0
; SECTION    NAME=LCD_BANK1     RAM=gpr1
; SECTION    NAME=LCD_BANK2     RAM=gpr2
; SECTION    NAME=LCD_BANK3     RAM=gpr3
; etc...

_LCD_SHR      UDATA_SHR
    _lcd_shr        res 1

_LCD_BANK     UDATA
    errorlevel -231
    _LCD_MEM        res 0
    errorlevel +231
    _lcd_addr       res 1
    _lcd_data_h     res 1
    _lcd_data_l     res 1
    _lcd_buf_cnt    res 1
    _lcd_tx_ctrl    res 1
    _lcd_bkl_rs     res 1   ; backlight and RS

    _lcd_disp_ctrl  res 1   ; display control state (disp. en, cursor visible, blink) TODO merge with entry?
    _lcd_entry_ctrl res 1
    _lcd_shift_ctrl res 1

; _lcd_bkl_rs/TX-byte bit positions
#define _LCD_RS               0
#define _LCD_RW               1
#define _LCD_EN               2
#define _LCD_BKL              3

; masks
#define _LCD_RS_MASK          (1 << _LCD_RS)
#define _LCD_RW_MASK          (1 << _LCD_RW)
#define _LCD_EN_MASK          (1 << _LCD_EN)
#define _LCD_BKL_MASK         (1 << _LCD_BKL)

; _lcd_tx_ctrl bit positions
#define _LCD_C_STR            7
#define _LCD_BYTE_OP          6 ; ??? unused?
#define _LCD_TXCNT_4          2
#define _LCD_TXCNT_2          1
#define _LCD_TXCNT_1          0

#define LCD_COLS              20
#define LCD_ROWS              4

; cmd values

#define LCD_CLEAR_CMD         0x01

#define LCD_HOME_CMD          0x02 ; cursor back to 0,0, reset shift

#define LCD_ENTRY_CMD         0x04 ; base
  #define LCD_ENTRY_INC       0x02 ; [INCrement] cursor vs decrement
  #define LCD_ENTRY_SHIFT     0x01 ; [SHIFT] display vs no shift

#define LCD_DISP_CMD          0x08 ; base
  #define LCD_DISP_EN         0x04 ; [ENable] vs disable
  #define LCD_DISP_CURS       0x02 ; [CURSor] visible vs invisible
  #define LCD_DISP_BLINK      0x01 ; [BLINK] cursor vs static cursor

#define LCD_SHIFT_CMD         0x10 ; base
  #define LCD_SHIFT_DISP      0x08 ; vs shift display
  #define LCD_SHIFT_RIGHT     0x04 ; vs shift left

#define LCD_FUNC_CMD          0x20 ; base
  #define LCD_FUNC_DL         0X10 ; [DL] 8-bit vs 4-bit
  #define LCD_FUNC_N          0x08 ; [N] 2 lines vs 1-line
  #define LCD_FUNC_F          0x04 ; [F] 5x10 dots vs 5x7

#define LCD_SET_CGRAM_CMD     0x40 ; base

#define LCD_SET_DDRAM_CMD     0x80 ; base
  #define LCD_OFFSET_R0       0x00
  #define LCD_OFFSET_R1       0x40
  #define LCD_OFFSET_R2       0x14
  #define LCD_OFFSET_R3       0x54

  #define LCD_POS_CMD_ROW0        (LCD_SET_DDRAM_CMD + LCD_OFFSET_R0)
  #define LCD_POS_CMD_ROW1        (LCD_SET_DDRAM_CMD + LCD_OFFSET_R1)
  #define LCD_POS_CMD_ROW2        (LCD_SET_DDRAM_CMD + LCD_OFFSET_R2)
  #define LCD_POS_CMD_ROW3        (LCD_SET_DDRAM_CMD + LCD_OFFSET_R3)


; timing
#define LCD_EN_DELAY_US       37

; LCD bytes:
; |    7    |    6    |    5    |    4    |    3    |    2    |    1    |    0    |
; |---------|---------|---------|---------|---------|---------|---------|---------|
; |   nib3  |  nib2   |   nib1  |  nib0   |   BKL   |   EN    |   R/W   |    RS   |


; ---------------------------------

#include "deps/lcd_macros.inc"

LCD_PAGE    CODE

_LCD_PAGE

_lcd_delay_50_ms:
        DELAY_MS  50
        return

_lcd_delay_5_ms:
        DELAY_MS  5
        return

_lcd_delay_1500_us:
        DELAY_MS  5
        return

_lcd_disp_ctrl_or_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
_lcd_disp_ctrl_or:
        iorwf   _lcd_disp_ctrl,F
        movf    _lcd_disp_ctrl,W

        bcf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_byte_w

_lcd_disp_ctrl_and_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
_lcd_disp_ctrl_and:
        andwf   _lcd_disp_ctrl,F
        movf    _lcd_disp_ctrl,W

        bcf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_byte_w

_lcd_entry_ctrl_or_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
_lcd_entry_ctrl_or:
        iorwf   _lcd_entry_ctrl,F
        movf    _lcd_entry_ctrl,W

        bcf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_byte_w

_lcd_entry_ctrl_and_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
_lcd_entry_ctrl_and:
        andwf   _lcd_entry_ctrl,F
        movf    _lcd_entry_ctrl,W

        bcf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_byte_w

_lcd_shift_ctrl_or_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
_lcd_shift_ctrl_or:
        iorwf   _lcd_shift_ctrl,F
        movf    _lcd_shift_ctrl,W

        bcf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_byte_w

_lcd_shift_ctrl_and_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
_lcd_shift_ctrl_and:
        andwf   _lcd_shift_ctrl,F
        movf    _lcd_shift_ctrl,W

        bcf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_byte_w

_lcd_shift_ctrl_set_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
_lcd_shift_ctrl_set:
        movwf   _lcd_shift_ctrl
        bcf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_byte_w


; initialise LCD at address held in W
;   TODO probably doable with a lot less instrs.
_lcd_init:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        movwf   _lcd_addr

        ; default lcd control nibble:
        ;    backlight on, EN set, write, CMD mode
        movlw   (_LCD_BKL_MASK | _LCD_EN_MASK)
        movwf   _lcd_bkl_rs

        movlw   (LCD_DISP_CMD | LCD_DISP_EN) ; TODO: merge remove cmd bits and merge with entry
        movwf   _lcd_disp_ctrl

        movlw   (LCD_ENTRY_CMD | LCD_ENTRY_INC) ; TODO: merge remove cmd bits and merge with disp
        movwf   _lcd_entry_ctrl

        bcf     _lcd_bkl_rs,_LCD_RS

        movlw   0x00
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit
        call    _lcd_delay_50_ms

        movlw   0x03
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit
        call    _lcd_delay_5_ms

        movlw   0x03
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit
        call    _lcd_delay_5_ms

        movlw   0x03
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit
        call    _lcd_delay_5_ms

        movlw   0x02
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit

        movlw   0x02
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit

        movlw   0x08
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit

        movlw   0x00
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit

        movlw   0x0C
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit

        movlw   0x00
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit

        movlw   0x06
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit

        movlw   0x06
        call    _lcd_write_byte_w
        btfsc   STATUS,Z
        goto    _lcd_init_exit

        bcf     STATUS,Z
_lcd_init_exit:
        STATUS_RSTR_FLG _lcd_shr, STATUS, Z, Z
        return

; reference doc comment TODO: apply this to routines...?

; writes a single [addr,byte] pair to LCD at LCD_ADDR, using payload byte W
; pre:
;   i2c transaction active (START cond sent)
;   cmd/data already selected in _lcd_blk_rs
;   backlight state already selected in _lcd_blk_rs
; post:
;   STATUS,Z set on error, clear on OK
;   STATUS otherwise untouched, i.e. RP{0,1} are preserved
;   I2C lib's i2c_stat error flags set on i2c error

; single byte write shims

; address in W, data in IRP:FSR, STATUS unsaved (special case)
_lcd_write_byte_i_data_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bsf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_byte_i
_lcd_write_byte_i_cmd_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bcf     _lcd_bkl_rs,_LCD_RS
        ; common case: continue...
_lcd_write_byte_i:
        movlw   1
        movwf   _lcd_buf_cnt
        clrf    _lcd_tx_ctrl
        goto    _lcd_write_raw_i

; address loaded, data in W, STATUS unsaved
_lcd_write_byte_w_data_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bsf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_byte_w
_lcd_write_byte_w_cmd_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bcf     _lcd_bkl_rs,_LCD_RS
        ; common case: continue...
_lcd_write_byte_w:
        call    _lcd_load_byte
        movlw   1
        decf    FSR,F               ; compensate for incf FSR in loop
        movwf   _lcd_buf_cnt
        clrf    _lcd_tx_ctrl
        goto    _lcd_write_raw

; address loaded, W holds data FSR
_lcd_write_byte_p_data_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bsf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_byte_p
_lcd_write_byte_p_cmd_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bcf     _lcd_bkl_rs,_LCD_RS
        ; common case: continue...
_lcd_write_byte_p:
        movwf   FSR
        movlw   1
        movwf   _lcd_buf_cnt
        clrf    _lcd_tx_ctrl
        goto    _lcd_write_raw_i

; address in W, data loaded into _lcd_data_{h,l}, STATUS saved
_lcd_write_byte_a:
        movwf   _lcd_addr
        movlw   1
        movwf   _lcd_buf_cnt
        clrf    _lcd_tx_ctrl
        goto    _lcd_write_raw

; setup routines
;   these allow macros to save on inlined instr count by combining: storing
;   STATUS, selcting ram bank, selecting cmd vs data, and setting up one
;   parameter

_lcd_cmd_addr_setup_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bcf     _lcd_bkl_rs,_LCD_RS
        movwf   _lcd_addr
        return

_lcd_data_addr_setup_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bsf     _lcd_bkl_rs,_LCD_RS
        movwf   _lcd_addr
        return

_lcd_data_len_setup_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bsf     _lcd_bkl_rs,_LCD_RS
        movwf   _lcd_buf_cnt
        return

_lcd_cmd_fsr_setup_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bcf     _lcd_bkl_rs,_LCD_RS
        movwf   FSR
        return

_lcd_data_fsr_setup_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bsf     _lcd_bkl_rs,_LCD_RS
        movwf   FSR
        return

_lcd_data_byte_setup_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bsf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_load_byte

_lcd_cmd_byte_setup_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
        bcf     _lcd_bkl_rs,_LCD_RS
        ;goto    _lcd_load_byte
        ; common case: continue...
_lcd_load_byte:
        movwf   _lcd_data_h
        swapf   _lcd_data_h,W
        movwf   _lcd_data_l

        movlw   0xF0                    ; clear lower bits
        andwf   _lcd_data_h,F
        andwf   _lcd_data_l,F

        movf   _lcd_bkl_rs,W             ; set lcd control nibble
        iorwf   _lcd_data_h,F
        iorwf   _lcd_data_l,F
        return

; writes [addr,byte] pairs to I2C address in _lcd_addr, taking bytes starting
; from IRP:FSR, with the count passed in W
; pre:
;   i2c transaction active
;   i2c addr in _lcd_addr
;   payload byte count in W
;   IRP:FSR points to payload data
; post:
;   FSR points to last byte written (advanced by one less than value in W)
;   STATUS,Z set on error, clear on OK
;   STATUS otherwise untouched, i.e. RP{0,1} are preserved
;   I2C lib's i2c_stat error flags set on i2c error

; buffer write shims (always  data/char mode)

; W holds buf len
_lcd_write_buf_l_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
_lcd_write_buf_l:
        movwf   _lcd_buf_cnt
        clrf    _lcd_tx_ctrl
        bsf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_raw_i

; W holds FSR ptr (low 8-bits)
_lcd_write_buf_f:
        movwf   FSR
        clrf    _lcd_tx_ctrl
        bsf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_raw_i

; W ignored
_lcd_write_bufc:
        clrf    _lcd_tx_ctrl
        bsf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_raw_i

; W holds FSR
_lcd_write_bufc_f_ss:
        STATUS_SAVE _lcd_shr
        banksel _LCD_MEM
_lcd_write_bufc_f:
        bsf     _lcd_tx_ctrl,_LCD_C_STR
        movwf   FSR
        clrf    _lcd_tx_ctrl
        bsf     _lcd_bkl_rs,_LCD_RS
        goto    _lcd_write_cstr_check

; --------------------------------------
; main write routine block

_lcd_write_buf_loop:
        incf    FSR,F
        ; skip counter if C-string checking enabled
        btfss   _lcd_tx_ctrl,_LCD_C_STR
        goto    _lcd_write_decr_cnt
_lcd_write_cstr_check:
        movf    INDF,W
        btfss   STATUS,Z
        goto    _lcd_write_raw_w
        incf    FSR,F
        goto    _lcd_write_ok

_lcd_write_decr_cnt:
        decf    _lcd_buf_cnt,F
        btfsc   STATUS,Z
        goto    _lcd_write_ok
        ; load next payload byte
_lcd_write_raw_i:
        movf    INDF,W
_lcd_write_raw_w:
        call    _lcd_load_byte
_lcd_write_raw:
        bsf     _lcd_tx_ctrl,_LCD_TXCNT_4   ; set to 4, always 0 preceding this
        goto    _lcd_write_en_hi

_lcd_write_en_lo:                           ; head of inner loop iters 2 & 4
        bcf     _lcd_data_h,_LCD_EN         ; EN low on 2nd iter of each nib
_lcd_write_en_hi:                           ; head of inner loop iters 1 & 3
        ; write [addr, byte] to i2c bus
        I2C_STA
        I2C_TX_B2   _lcd_addr, _lcd_data_h
        I2C_STO

        ; loop control
        decf    _lcd_tx_ctrl,F

        btfsc   _lcd_tx_ctrl,0        ; check for iterations 1, 3...
        goto    _lcd_write_en_lo      ; next tx is EN low

        btfsc   STATUS,Z              ; check for 4 iters complete...
        goto    _lcd_write_buf_loop   ; ...if so: src next byte

        movf    _lcd_data_l,W         ; flip to second payload nibble
        movwf   _lcd_data_h           ;
        goto    _lcd_write_en_hi      ; new nibble starts with EN high

_lcd_write_err:
        I2C_STO
        pagesel _LCD_PAGE
        banksel _LCD_MEM ; belt & braces

        bsf   STATUS,Z
        goto  _lcd_write_exit
_lcd_write_ok:
        bcf   STATUS,Z
_lcd_write_exit:
        STATUS_RSTR_FLG _lcd_shr, STATUS, Z, Z
        return


#endif ; __LCD_INC__


