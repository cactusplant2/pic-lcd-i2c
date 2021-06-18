#ifndef __I2C_HIGH_INC__
#define __I2C_HIGH_INC__

#include "deps/i2c.inc"
#include "deps/i2c_low.inc"

#define I2C_GEN_ADDR        0x00
#define I2C_GEN_RESET_CMD   0x06

I2C_CHECK_ADDR7 MACRO   addr
    local a = (addr)
    if (0 > a) || (a > 127)
      error "I2C 7-bit address invalid, valid range: 0..=127"
    endif
    ENDM

I2C_CHECK_ADDR10 MACRO   addr
  if (0 > (addr)) || ((addr) > 1023)
    error "I2C 7-bit address invalid, valid range: 0..=1023"
  endif
    ENDM

; TODO:
;   # addr check in all macros with addrs
;   # setup routines to combine setup and reduce
;   # use STATUS store and restore on all macros
;     - can we place 'restore' sections at tail of routines??
;     - will need dedicated shared var for STATUS
;     - review shared mem, can we reduce usage
;   # make macro naming scheme same as lcd code (suffixes etc)

I2C_INIT        MACRO
        pagesel _I2C_LOW_PAGE
        call    i2c_op_setup_ss
        call    i2c_init_hw
        call    i2c_restore_status
        pagesel $
    ENDM

I2C_STA         MACRO
        pagesel _I2C_LOW_PAGE
        call    i2c_op_setup_ss
        call    i2c_bus_rel
        call    i2c_start_tbuf
        call    i2c_restore_status
        pagesel $
    ENDM

I2C_STO         MACRO
        pagesel _I2C_LOW_PAGE
        call    i2c_op_setup_ss
        call    i2c_bus_hold
        call    i2c_stop_cond
        call    i2c_restore_status
        pagesel $
    ENDM

I2C_STA_REP   MACRO
        pagesel _I2C_LOW_PAGE
        call    i2c_op_setup_ss
        call    i2c_bus_rel
        call    i2c_start_tsu
        call    i2c_restore_status
        pagesel $
    ENDM

; send a single byte on I2C bus, I2C addressing not handled
I2C_TX_W        MACRO
        pagesel _I2C_LOW_PAGE
        call    i2c_op_setup_ss
        call    i2c_tx_byte_w
        call    i2c_restore_status
        pagesel $
    ENDM

; writes W then a single byte from addr pointed to by IRP:FSR
I2C_TX_WI1      MACRO
        pagesel _I2C_LOW_PAGE

        call    i2c_op_setup_ss

        clrf    i2c_cnt
        bsf     i2c_cnt,0               ; buf-len: i2c_cnt == 1 (avoid W clobber)
        call    i2c_tx_w_buf

        call    i2c_restore_status
        pagesel $
    ENDM

I2C_TX_WB1  MACRO   b1
        local   exit
        pagesel _I2C_LOW_PAGE

        movwf   _i2c_buf1
        movf    (b1),W
        movwf   _i2c_buf2

        call    i2c_op_setup_ss

        movf    i2c_buf1,W
        call    i2c_tx_byte_w
        btfsc   STATUS,Z
        goto    exit

        movf    i2c_buf2,W
        call    i2c_tx_byte_w
exit:
        call    i2c_restore_status
        pagesel $
    ENDM

I2C_TX_B2  MACRO   b1, b2
        local   exit
        pagesel _I2C_LOW_PAGE

        movf    (b2),W
        movwf   _i2c_buf2
        movf    (b1),W

        call    i2c_op_setup_ss

        call    i2c_tx_byte_w
        btfsc   STATUS,Z
        goto    exit

        movf    _i2c_buf2,W
        call    i2c_tx_byte_w
exit:
        call    i2c_restore_status
        pagesel $
    ENDM

I2C_WRITE_L1     MACRO   addr, l1
        local   exit
        pagesel _I2C_LOW_PAGE

        call    i2c_op_setup_ss

        movlw   ((addr) << 1) & 0xFE              ; load address, read bit cleared
        call    i2c_tx_byte_w
        btfsc   STATUS,Z
        goto    exit

        movlw   (l1)
        call    i2c_tx_byte_w
exit:
        call    i2c_restore_status
        pagesel $
    ENDM

; broken? op_setup means bank might not match macro vars'
I2C_WRITE_L2     MACRO   addr, l1, l2
        local   exit
        pagesel _I2C_LOW_PAGE

        call    i2c_op_setup_ss

        movlw   ((addr) << 1) & 0xFE              ; load address, read bit cleared
        call    i2c_tx_byte_w
        btfsc   STATUS,Z
        goto    exit

        movlw   (l1)
        call    i2c_tx_byte_w
        btfsc   STATUS,Z
        goto    exit

        movlw   (l2)
        call    i2c_tx_byte_w
exit:
        call    i2c_restore_status
        pagesel $
    ENDM

I2C_WRITE_BUF_1 MACRO   addr, buf
        local   exit
        pagesel _I2C_LOW_PAGE

        movlw   ((addr) << 1) & 0xFE              ; load address, read bit cleared

        call    i2c_op_setup_ss
        call    i2c_tx_byte_w
        btfsc   STATUS,Z
        goto    exit

        call    i2c_restore_status
        movlw   (buf)

        call    i2c_op_setup_ss
        bankisel  (buf)
        call    i2c_tx_byte
exit:
        call    i2c_restore_status
        pagesel $
    ENDM

I2C_WRITE_BUF   MACRO   addr, buf, len
        local   exit
        pagesel _I2C_LOW_PAGE

        movlw   len
        movwf   i2c_cnt

        movlw   (buf)
        movwf   FSR

        call    i2c_op_setup_ss
        bankisel (buf)

        movlw   ((addr) << 1) & 0xFE              ; load address, read bit cleared
        call    i2c_tx_w_buf
exit:
        call    i2c_restore_status
        pagesel $
    ENDM

I2C_READ_BUF    MACRO   addr, count, buf
        local   exit
        pagesel _I2C_LOW_PAGE

        movlw   ((addr) << 1) | 1               ; load address, set read bit

        call    i2c_op_setup_ss
        call    i2c_tx_byte_w
        btfsc   STATUS,Z
        goto    exit

        call    i2c_restore_status
        movlw   (buf)
        movwf   FSR

        movlw   (count)
        movwf   i2c_cnt

        call    i2c_op_setup_ss
        bankisel (buf)
        call    i2c_rx_buf

exit:
        call    i2c_restore_status
        pagesel $
    ENDM

I2C_PING_ADDR   MACRO   addr
        pagesel _I2C_LOW_PAGE
        call    i2c_op_setup_ss
        movlw   ((addr) << 1) & 0xFE              ; load address, read bit cleared
        call    i2c_tx_byte_w

        call    i2c_restore_status
        pagesel $
    ENDM


; issues a general reset command to all targets on the bus
I2C_GEN_RESET   MACRO
        local   exit
        pagesel _I2C_LOW_PAGE

        ; TODO: implement helper routine
        call    i2c_op_setup_ss

        ; STOP cond.
        call    i2c_bus_hold
        call    i2c_stop_cond

        ; START cond.
        call    i2c_bus_rel
        call    i2c_start_tbuf

        movlw   I2C_GEN_ADDR
        call    i2c_tx_byte_w

        btfsc   STATUS,Z
        goto    exit

        movlw   I2C_GEN_RESET_CMD
        call    i2c_tx_byte_w

exit:
        call    i2c_stop_cond
        call    i2c_restore_status
        pagesel $
    ENDM

; ------------------------------------------------------------------------------
; target specific: move to new .inc files
;

#endif ; __I2C_HIGH_INC__
