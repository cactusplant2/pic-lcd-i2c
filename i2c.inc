#ifndef __I2C_INC__
#define __I2C_INC__

; shared (unbanked) RAM for passing args. return values, and status
I2C_SHR         UDATA_SHR
    ; never modified by i2c_low routines
    ; modified by i2c_low routines
    _i2c_buf1           res 1  ; used by macros
    _i2c_buf2           res 1  ; used by macros
    i2c_stat            res 1   ; I2C status flags
    i2c_cnt             res 1   ; send/receive count
    i2c_status_shr      res 1
  #ifdef TX_DEBUG_EN
    i2c_debug_state     res 1
  #endif

; banked RAM areas for low-level API and internal use
I2C_BANK0       UDATA
    i2c0_w_tmp          res 1
    i2c0_status_tmp     res 1
    i2c0_cnt_m          res 1
    i2c0_cnt_l          res 1

I2C_BANK1       UDATA           ; general purpose local var
    i2c_ctrl            res 1   ; I2C control register
    i2c_data            res 1   ; general purpose TX/RX var
;    i2c_bit_cnt         res 1   ; TX/RX bit count
    i2c1_debug_data_orig res 1
    i2c1_debug_data_rbld res 1


; i2c_control flags
    _I2C_SCL_TO_INF     equ 7   ; no timeout when SCL stretched
    _I2C_IGN_NACK       equ 6   ; for debugging

; i2c_stat flags
    _I2C_ERR_NACK       equ 2   ; NACK condition
    _I2C_ERR_BUSY       equ 1   ; bus was busy at start of operation
    _I2C_ERR_SCL_TO     equ 0   ; timeout while SCL was held low by target

#define _SCL            I2C_TRIS,I2C_SCL_BIT
#define _SCL_IN         I2C_PORT,I2C_SCL_BIT

#define _SDA            I2C_TRIS,I2C_SDA_BIT
#define _SDA_IN         I2C_PORT,I2C_SDA_BIT

#define _SCL_MASK       (1 << I2C_SCL_BIT)
#define _SDA_MASK       (1 << I2C_SDA_BIT)
#define _I2C_PINS_MASK  (_SCL_MASK | _SDA_MASK)

; ---------------------------------------------------------------------------
; // I2C configuration
; // -----------------
;
; To configure the I2C provide definitions for the following in client code
; that including this library. For performance reasons SCL and SDA pins must
; belong to the same port.

 ; #define I2C_PORT        ; IO port data-register used for SCL and SDA pins
 ; #define I2C_SCL_BIT     ; bit position for SCL pin in I2C port regs.
 ; #define I2C_SDA_BIT     ; bit position for SDA pin in data/tri-state regs.

    #ifndef I2C_PORT
        error "I2C_PORT must be defined according to port used for I2C pins"
    #endif

    #ifndef I2C_SCL_BIT
        error "I2C_SCL_BIT must be defined according to bit position of SCL pin"
    #endif

    #ifndef I2C_SDA_BIT
        error "I2C_SDA_BIT must be defined according to bit position of SDA pin"
    #endif

    if I2C_PORT == PORTA
        I2C_TRIS equ TRISA
    else
    if I2C_PORT == PORTB
        I2C_TRIS equ TRISB
    else
    if I2C_PORT == PORTC
        I2C_TRIS equ TRISC
    else
    if I2C_PORT == PORTD
        I2C_TRIS equ TRISD
    else
    if I2C_PORT == PORTE
        I2C_TRIS equ TRISE
    else
        error "I2C_PORT value invalid, use one of: PORT{A,B,C,D,E}"
    endif
    endif
    endif
    endif
    endif

_I2C_LOW_BANK    equ I2C_TRIS



#ifdef I2C_SCL_WAIT_BIT
  #ifndef I2C_SCL_WAIT_BYTE
    #define I2C_SCL_WAIT_BYTE
  #endif
#endif

#ifdef I2C_SCL_WAIT_BYTE
  #ifndef I2C_SCL_WAIT_TX_BYTE
    #define I2C_SCL_WAIT_TX_BYTE
  #endif
  #ifndef I2C_SCL_WAIT_RX_BYTE
    #define I2C_SCL_WAIT_RX_BYTE
  #endif
#endif

#ifdef I2C_SCL_WAIT_TX_BIT
  #ifndef I2C_SCL_WAIT_TX_BYTE
    #define I2C_SCL_WAIT_TX_BYTE
  #endif
#endif

#ifdef I2C_SCL_WAIT_RX_BIT
  #ifndef I2C_SCL_WAIT_RX_BYTE
    #define I2C_SCL_WAIT_RX_BYTE
  #endif
#endif

#ifdef I2C_SCL_WAIT_TX_BYTE
  #ifndef _I2C_SCL_WAIT
    #define _I2C_SCL_WAIT
  #endif
#endif

#ifdef I2C_SCL_WAIT_RX_BYTE
  #ifndef _I2C_SCL_WAIT
    #define _I2C_SCL_WAIT
  #endif
#endif

#endif ; __I2C_INC__
