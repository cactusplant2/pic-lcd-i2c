#ifndef __I2C_LOW_INC__
#define __I2C_LOW_INC__

#include "deps/delay.inc"
#include "deps/util.inc"


; IMPORTANT NOTES:
;   * RAM banks: bank 1 is assumed in all code in the low-level I2C code below,
;     except for 'public' routines (identifiable by their PascalCase style
;     labels), which always use the necessary banksel directive. The active bank
;     is normally bank 1 when returning routines in this file, although this is
;     not a guaranatee and caller should ensure the correct bank is restored for
;     their usage.
;

; timing notation:
;           |> tXXXX    : previous instr. begins interval
;              tXXXX <| : previous instr. ends interval (SM:FM)
;   where SM: 100kHz minimum, and FM: 400kHz minimum (both in microseconds)

        ; TODO top level routine that is given pointer (in W?) to a data
        ; structure as follows:
        ; | ctrl | tx_len | rx_len | tx_buf[0..tx_len] | rx_buf[0..rx_len]
        ;
        ; this will allow single instr. calls, maybe with first rx byte (if any)
        ; returned in W
        ; would be good to have a single instr. way to check success though?

; timing macros all times in nanoseconds

#ifdef  I2C_SPEED_100KHZ
    #define I2C_TLOW        4700    ; min
    #define I2C_THIGH       4000    ; min

    #define I2C_TRISE       1000    ; max
    #define I2C_TFALL       300     ; max

    #define I2C_TSU_STA     4700    ; min
    #define I2C_TSU_DAT     250     ; min
    #define I2C_TSU_STO     4000    ; min
    #define I2C_TBUF        4700    ; min

    #define I2C_THD_STA     4000    ; min
    #define I2C_THD_DAT     0       ; min

    #define I2C_VD_DAT      3450    ; max
    #define I2C_VD_ACK      3450    ; max
#else
#ifdef  I2C_SPEED_BP_FRIENDLY
    #define I2C_TLOW        9400    ; min
    #define I2C_THIGH       8000    ; min

    #define I2C_TRISE       2000    ; max
    #define I2C_TFALL       600     ; max

    #define I2C_TSU_STA     9400    ; min
    #define I2C_TSU_DAT     500     ; min
    #define I2C_TSU_STO     8000    ; min
    #define I2C_TBUF        9400    ; min

    #define I2C_THD_STA     8000    ; min
    #define I2C_THD_DAT     0       ; min

    #define I2C_VD_DAT      6900    ; max
    #define I2C_VD_ACK      6900    ; max
#else
#ifdef  I2C_SPEED_400KHZ
    #define I2C_TLOW        1300    ; min
    #define I2C_THIGH       600     ; min

    #define I2C_TRISE       300     ; max
    #define I2C_TFALL       300     ; max

    #define I2C_TSU_STA     600     ; min
    #define I2C_TSU_DAT     100     ; min
    #define I2C_TSU_STO     600     ; min
    #define I2C_TBUF        1300    ; min

    #define I2C_THD_STA     600     ; min
    #define I2C_THD_DAT     0       ; min

    #define I2C_VD_DAT      900     ; max
    #define I2C_VD_ACK      900     ; max
#else
#ifdef  I2C_SPEED_1000KHZ
    #define I2C_TLOW        500     ; min
    #define I2C_THIGH       260     ; min

    #define I2C_TRISE       120     ; max
    #define I2C_TFALL       120     ; max

    #define I2C_TSU_STA     260     ; min
    #define I2C_TSU_DAT      50     ; min
    #define I2C_TSU_STO     260     ; min
    #define I2C_TBUF        500     ; min

    #define I2C_THD_STA     260     ; min
    #define I2C_THD_DAT     0       ; min

    #define I2C_VD_DAT      450     ; max
    #define I2C_VD_ACK      450     ; max
#else
    error "I2C speed not defined, please define _one_ of: I2C_SPEED_{100|400|1000}KHZ"
#endif
#endif
#endif
#endif

I2C_DLY     MACRO   ns, adj
    DELAY_NS_ADJ     (ns), (adj)
    ENDM

I2C_DLY_MAX MACRO   ns_1, adj_1, ns_2, adj_2
    #ifndef SYSCLK_HZ
      error "I2C_DLY_MAX macro requires SYSCLK_HZ to be defined"
    #endif

    local ns_pc = (1000000000 / (SYSCLK_HZ / 4))      ; nanos per instr. cycle

    local cycs_1 = ((ns_1) + (ns_pc - 1)) / ns_pc
    if cycs_1 > (adj_1)
      cycs_1 -= (adj_1)
    endif

    local cycs_2 = ((ns_2) + (ns_pc - 1)) / ns_pc
    if cycs_2 > (adj_2)
      cycs_2 -= (adj_2)
    endif

    if cycs_1 > cycs_2
      DELAY_CYCLES cycs_1
    else
      DELAY_CYCLES cycs_2
    endif

    ENDM

; aligns TX data byte ready for optimised transmission
;   intr count depends on value of I2C_SDA_BIT, worst case: 6 instrs (4
I2C_TX_SHIFT    MACRO   dreg
        ; SDA pos:  7 6 5 4 3 2 1 0 C
        ;
        ; to get bit 7 into SDA position:
        ;   SDA bit     operation
        ;       0       rlf 2
        ;       1       rlf 3
        ;       2       rlf 4
        ;       3       rrf 4
        ;       4       rrf 3
        ;       5       rrf 2
        ;       6       rrf 1
        ;       7       ---

    if (I2C_SDA_BIT) <= 2
      local rl = (I2C_SDA_BIT) + 2
      while rl > 0
        rlf     (dreg),F
        rl -= 1
      endw
    else
      local rr = 7 - (I2C_SDA_BIT)
      while rr > 0
        rrf     (dreg),F
        rr -= 1
      endw
    endif

    ENDM

I2C_LOW_PAGE    CODE

_I2C_LOW_PAGE   ; used by i2c routines for pagesel

; new simple APIs:

; low level:
;   designed for macros and advanced use, must ensure correct bank
;   selection, page selection etc
;
;   i2c_start
;   i2c_stop
;   i2c_write_byte  : byte to write is passed in W
;   i2c_write_buf
;   i2c_read_byte   : byte read is returned in W
;   i2c_read_buf    :
;
;   high:   usable from any bank/location etc
;
;   ...

; --------------------------------------
; macro support

; common setup for i2c high level routines/macros
i2c_op_setup_ss:
        STATUS_SAVE i2c_status_shr
i2c_op_setup:
        ; why, oh why does clrf set Z???
        clrf  i2c_stat
        bcf   STATUS,Z
        banksel _I2C_LOW_BANK
        return

i2c_restore_status:
        STATUS_RSTR_FLG i2c_status_shr, STATUS, Z, Z
        return

; --------------------------------------
; TODO: decide if high level API?
i2c_init_hw:
    ; pre:  bank1
    ; post: bank1, SCL high, SDA high

        bsf     _SCL                        ; SCL released (hi-z)
        bsf     _SDA                        ; SDA released (hi-z)
        clrf    SSPSTAT

        bcf     STATUS, RP0                 ; bank0

        movlw   ~(_I2C_PINS_MASK)
        andwf   I2C_PORT,F

        movlw   b'00111001'                 ; pic16f88: i2c controller, SCL high
        movwf   SSPCON
        bsf     STATUS, RP0                 ; bank1

        clrf    i2c_ctrl

        return

i2c_sda_rel:
    ; pre: bank1
        bsf     _SDA
        return

; pull both SCL and SDA low without generating a START cond
i2c_bus_hold:
    ; pre: bank1
        bcf     _SCL
        bcf     _SDA
        return

; release both SDA and SCL without generating a STOP condition
i2c_bus_rel:
    ; pre: bank1
        bsf     _SDA
        bsf     _SCL
        return

;-----------------------
; i2c_start_... routines
; pre:  bank1, SCL high, SDA high
; post: bank1, SCL low,  SDA low

; use for START when last [SDA HI] edge is unknown
i2c_start_tbuf:
                                                I2C_DLY I2C_TBUF,7
        goto    i2c_start_raw       ; goto included in delay adj above

; use for repeated START when last [SCL HI] edge is unknown
i2c_start_tsu:
    ; pre:  bank1, SCL high, SDA high
    ; post: bank1, SCL low,  SDA low
                                                I2C_DLY I2C_TSU_STA,5
; use with care: caller ensures any delays for tSU:STA, tBUF, tHIGH
i2c_start_raw:
    ; pre:  bank1, SCL high, SDA high,
    ; post: bank1, SCL low,  SDA low
        bcf     _SDA        ; [SDA LO] tSU:STA >| (4.7:0.6)
                            ;             tBUF >| (4.7:1.3)
                            ;          tVD:ACK >| (3.45:0.9)
                            ;                   |> tHD:STA

                                                I2C_DLY_MAX I2C_THIGH,6, I2C_THD_STA,1
        bcf     _SCL        ;          tHD:STA >| (4.0:0.6)
                            ;                   |> tLOW
        return

i2c_stop_cond:
    ; pre: bank1, SCL low, SDA low
    ; pre: bank1, SCL low, SDA low
        bsf     _SCL        ; [SCL HI]    tLOW >|
                            ;                   |> tSU:STO
                                                I2C_DLY I2C_TSU_STO,1
        bsf     _SDA        ; [SDA HI]  tSU:STO>|
                            ;                   |> tBUF
        return

;#define TX_DEBUG_EN

TX_DEBUG    MACRO   bit
  #ifdef TX_DEBUG_EN
        btfss   I2C_TRIS,I2C_SDA_BIT
        bcf     i2c1_debug_data_rbld,(bit)
        btfsc   I2C_TRIS,I2C_SDA_BIT
        bsf     i2c1_debug_data_rbld,(bit)


        movf    i2c1_debug_data_orig,W
        andlw   (0xFF << (bit)) & 0xFF
        xorwf   i2c1_debug_data_rbld,W

        call    debug_step

        btfss   STATUS,Z
        call    debug_mismatch
        incf    i2c_debug_state,F
  #endif
  ENDM

  #ifdef TX_DEBUG_EN
debug_step:
        nop
        nop
        nop
        nop
        nop
        return

debug_mismatch:
        nop
        nop
        nop
        nop
        nop
        return
  #endif

; tx routines

i2c_tx_byte_i:
        movf    INDF,W
i2c_tx_byte_w:
        movwf   i2c_data
;i2c_tx_byte:                ; byte already loaded into i2c_data (disabled: i2c_data now bank1)
        movlw   1
        movwf   i2c_cnt
        decf    FSR,F        ; compensate for incf at tail of loop

        goto    _i2c_tx_impl

; pre: bank 1, SCL low, TX bytes in IRP:FSR, first byte in W
;      i2c_cnt is buf len
;      useful for macros/routines using dynamic addressing
i2c_tx_w_buf:
        movwf   i2c_data

        ; adjust for first TX byte being W
        decf    FSR,F
        incf    i2c_cnt,F

        goto    _i2c_tx_impl

i2c_tx_buf_lenw:
        movwf   i2c_cnt
; tx byte-loop:
i2c_tx_buf:
    ; pre: bank 1, SCL low, TX bytes in IRP:FSR, TX count in i2c_cnt
    ; post: SCL low, , STATUS flags unchanged except Z (set/clear on error/OK)
    ; note: ordering of bit-tests is chosen so that SDA goes higher 2 cycles
    ;       earlier to compensate for rising edge being slower on I2C

        movf    INDF,W
        movwf   i2c_data

_i2c_tx_impl:
        ; load mask for bit 7
        ; andwf with indf, store in w
        ; iorwf with tris
        ; bit 7 (MSB)
;            movlw   (1 << 7)
;            andwf   INDF,W
;
;            movf    indf,w
;            do some aritmetic op to set/clear correct bit, store in w
;            andlw   sda mask
;            iorwf   trisreg

        ; multi-bit rotate in single instr:
        ;   masked val: 1000 0000    (bit 7)
        ;   target:     0000 0010    (bit 1)
        ;   add:        1000 0001
        ; hmm, aint gonna work...

        ; per byte:
        ;   load byte into temp                (2)
        ;   rotate to align 'one-off'   (worst: 4)
        ;
        ; per bit:
        ;   rot into W
        ;   store W
        ;   andlw sda_mask
        ;   iorwf tris reg, f

        ; DEBUG
    #ifdef TX_DEBUG_EN
        movf    i2c_data,W
        movwf   i2c1_debug_data_orig
        clrf    i2c1_debug_data_rbld
        clrf    i2c_debug_state
    #endif


        I2C_TX_SHIFT    i2c_data                ; 0..4 instrs, 3 when SDA is PORTx:1

        ; tLOW minimum delay adjustment assumptions here:
        ;   2 instrs from a previous `return`
        ;   2 instrs call overhead
        ;   2 instrs local above
        ;  loop instrs back here from last [SCL LO] : 8

        ; i2c transmissions notes:
        ;   challenge is to make best use of gaps between SCL transitions,
        ;   least number of instrs to load each bit and place on the bus seems
        ;   to be 4, but this involves two back-to-back bit-test conditionals
        ;   (during SCL low), causes SDA timing to vary depending on 0 or 1,
        ;   and leaves SCL high period unused.
        ;   another approach is to do it in 5 instrs using 2 XOR ops with mask
        ;   AND op, rotating the tx data in the SCL-high gap, ready for the next
        ;   bit. there is a small setup cost and requires an extra byte of ram
        ;   to rotate/discard but it gives even timing for SDA, this approach is
        ;   used below:

        ; bit 7 (MSB)
        movf    i2c_data,W
        xorwf   I2C_TRIS,W
        andlw   _SDA_MASK
        xorwf   I2C_TRIS,F  ; [SDA xx] tHD_DAT >| (CBUS only, ignored)
                            ;                   |> tSU:DAT
                                                I2C_DLY_MAX I2C_TLOW,11, I2C_TSU_DAT,1
        TX_DEBUG 7
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:1.3)
                            ;          tSU:DAT >| (0.25:0.1)
                            ;                   |> tHIGH

#ifdef I2C_SCL_WAIT_TX_BYTE
        ; tx byte-interval clock-stretch check
        bcf     STATUS,RP0  ; bank 0
        btfss   _SCL_IN
        goto    _i2c_tx_scl_byte_check
_i2c_tx_scl_byte_resume:
        bsf     STATUS,RP0  ; bank 1
                                                I2C_DLY I2C_THIGH,3
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW
        rlf     i2c_data,F                          ; rotate for bit 6
#else
        rlf     i2c_data,F                          ; rotate for bit 6
                                                I2C_DLY I2C_THIGH,2
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW
#endif

        ; bit 6
        movf    i2c_data,W
        xorwf   I2C_TRIS,W
        andlw   _SDA_MASK
        xorwf   I2C_TRIS,F  ; [SDA xx] tHD_DAT >| (CBUS only, ignored)
                            ;                   |> tSU:DAT
                                                I2C_DLY_MAX I2C_TLOW,5, I2C_TSU_DAT,1
        TX_DEBUG 6
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:1.3)
                            ;          tSU:DAT >| (0.25:0.1)
                            ;                   |> tHIGH
        rlf     i2c_data,F                          ; rotate for bit 5
                                                I2C_DLY I2C_THIGH,2
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW

        ; bit 5
        movf    i2c_data,W
        xorwf   I2C_TRIS,W
        andlw   _SDA_MASK
        xorwf   I2C_TRIS,F  ; [SDA xx] tHD_DAT >| (CBUS only, ignored)
                            ;                   |> tSU:DAT
                                                I2C_DLY_MAX I2C_TLOW,5, I2C_TSU_DAT,1
        TX_DEBUG 5
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:1.3)
                            ;          tSU:DAT >| (0.25:0.1)
                            ;                   |> tHIGH
        rlf     i2c_data,F                          ; rotate for bit 4
                                                I2C_DLY I2C_THIGH,2
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW

        ; bit 4
        movf    i2c_data,W
        xorwf   I2C_TRIS,W
        andlw   _SDA_MASK
        xorwf   I2C_TRIS,F  ; [SDA xx] tHD_DAT >| (CBUS only, ignored)
                            ;                   |> tSU:DAT
                                                I2C_DLY_MAX I2C_TLOW,5, I2C_TSU_DAT,1
        TX_DEBUG 4
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:1.3)
                            ;          tSU:DAT >| (0.25:0.1)
                            ;                   |> tHIGH
        rlf     i2c_data,F                          ; rotate for bit 3
                                                I2C_DLY I2C_THIGH,2
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW

        ; bit 3
        movf    i2c_data,W
        xorwf   I2C_TRIS,W
        andlw   _SDA_MASK
        xorwf   I2C_TRIS,F  ; [SDA xx] tHD_DAT >| (CBUS only, ignored)
                            ;                   |> tSU:DAT
                                                I2C_DLY_MAX I2C_TLOW,5, I2C_TSU_DAT,1
        TX_DEBUG 3
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:1.3)
                            ;          tSU:DAT >| (0.25:0.1)
                            ;                   |> tHIGH
        rlf     i2c_data,F                          ; rotate for bit 2
                                                I2C_DLY I2C_THIGH,2
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW

        ; bit 2
        movf    i2c_data,W
        xorwf   I2C_TRIS,W
        andlw   _SDA_MASK
        xorwf   I2C_TRIS,F  ; [SDA xx] tHD_DAT >| (CBUS only, ignored)
                            ;                   |> tSU:DAT
                                                I2C_DLY_MAX I2C_TLOW,5, I2C_TSU_DAT,1
        TX_DEBUG 2
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:1.3)
                            ;          tSU:DAT >| (0.25:0.1)
                            ;                   |> tHIGH
        rlf     i2c_data,F                          ; rotate for bit 1
                                                I2C_DLY I2C_THIGH,2
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW

        ; bit 1
        movf    i2c_data,W
        xorwf   I2C_TRIS,W
        andlw   _SDA_MASK
        xorwf   I2C_TRIS,F  ; [SDA xx] tHD_DAT >| (CBUS only, ignored)
                            ;                   |> tSU:DAT
                                                I2C_DLY_MAX I2C_TLOW,5, I2C_TSU_DAT,1
        TX_DEBUG 1
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:1.3)
                            ;          tSU:DAT >| (0.25:0.1)
                            ;                   |> tHIGH
        rlf     i2c_data,F                          ; rotate for bit 0
                                                I2C_DLY I2C_THIGH,2
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW

        ; bit 0
        movf    i2c_data,W
        xorwf   I2C_TRIS,W
        andlw   _SDA_MASK
        xorwf   I2C_TRIS,F  ; [SDA xx] tHD_DAT >| (CBUS only, ignored)
                            ;                   |> tSU:DAT
                                                I2C_DLY_MAX I2C_TLOW,5, I2C_TSU_DAT,1
        TX_DEBUG 0
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:1.3)
                            ;          tSU:DAT >| (0.25:0.1)
                            ;                   |> tHIGH
        ; rlf     i2c_data,F ; pointless rotation if we're discarding...
                                                I2C_DLY I2C_THIGH,1
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW

        ; check ACK
        movlw   _SDA_MASK                           ; load mask for ACK check
        bsf     _SDA        ; [SDA HI]
                                                I2C_DLY I2C_TLOW,3
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:1.3)
                            ;                   |> tHIGH
        bcf     STATUS,RP0                          ; bank 0
        andwf   I2C_PORT,W                          ; read port
        bsf     STATUS,RP0                          ; bank 1
                                                I2C_DLY I2C_THIGH,4
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW
        bcf     _SDA        ; [SDA LO]  (not necessary but keeps signal tidy)

        btfss   STATUS,Z                            ; if SDA was low: ACK received
        goto    _i2c_tx_nack                           ; NACK condition
_i2c_tx_nack_resume:

        ; tx loop control
        incf    FSR,F
        decfsz  i2c_cnt,F
        goto    i2c_tx_buf

_i2c_tx_exit_ok:
        bcf     STATUS,Z
        return

_i2c_tx_nack:
        bsf      i2c_stat,_I2C_ERR_NACK
        btfsc    i2c_ctrl,_I2C_IGN_NACK
        goto     _i2c_tx_nack_resume
        bsf     STATUS,Z            ; indicate error
        return                      ; exit: NACK

#ifdef I2C_SCL_WAIT_TX_BYTE
; tx byte-level SCL stretch check
_i2c_tx_scl_byte_check:
        btfsc   _SCL_IN             ; check again (slow rise time?)
        goto    _i2c_tx_scl_byte_resume

        call    _i2c_scl_wait
        btfss   i2c_stat,_I2C_ERR_SCL_TO
        goto    _i2c_tx_scl_byte_resume

        return  ; clock-stretch timeout error, STATUS,Z already set
#endif

#ifdef I2C_SCL_WAIT_RX_BYTE
; tx byte-level SCL stretch check
_i2c_rx_scl_byte_check:
        btfsc   _SCL_IN             ; check again (slow rise time?)
        goto    _i2c_rx_scl_byte_resume

        call    _i2c_scl_wait
        btfss   i2c_stat,_I2C_ERR_SCL_TO
        goto    _i2c_rx_scl_byte_resume

        return  ; clock-stretch timeout error, STATUS,Z already set
#endif

; --------------------------------------
; receive routines
;   pre: bank 1, SCL low, SDA high, SCL/SDA PORTx latch bits MUST be 0
i2c_rx_byte:
        movlw   1
        movwf   i2c_cnt

        movlw   (i2c_data - 1)      ; decrement so incf below is correct (hacky)
        movwf   FSR                 ; IRP irrelevant since unbanked?

_i2c_rx_loop:
        incf    FSR,F
i2c_rx_buf:
        bsf     _SDA        ; [SDA HI]
        clrf    INDF

        ; bit 7 (MSB)
        movlw   _SDA_MASK
                                                I2C_DLY I2C_TLOW,8
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:0.6)
                            ;                   |> tHIGH

        bcf     STATUS,RP0                          ; bank 0
#ifdef I2C_SCL_WAIT_RX_BYTE
        ; rx byte-interval clock-stretch check
        btfss   _SCL_IN
        goto    _i2c_rx_scl_byte_check
_i2c_rx_scl_byte_resume:
#endif
        andwf   I2C_PORT,W                          ; read port
        bsf     STATUS,RP0                          ; bank 1
                                                I2C_DLY I2C_THIGH,4
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW
        btfss   STATUS,Z                            ; store incoming bit
        bsf     INDF,7

        ; bit 6
        movlw   _SDA_MASK
                                                I2C_DLY I2C_TLOW,4
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:0.6)
                            ;                   |> tHIGH
        bcf     STATUS,RP0                          ; bank 0
        andwf   I2C_PORT,W                          ; read port
        bsf     STATUS,RP0                          ; bank 1
                                                I2C_DLY I2C_THIGH,4
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW
        btfss   STATUS,Z                            ; store incoming bit
        bsf     INDF,6

        ; bit 5
        movlw   _SDA_MASK
                                                I2C_DLY I2C_TLOW,4
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:0.6)
                            ;                   |> tHIGH
        bcf     STATUS,RP0                          ; bank 0
        andwf   I2C_PORT,W                          ; read port
        bsf     STATUS,RP0                          ; bank 1
                                                I2C_DLY I2C_THIGH,4
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW
        btfss   STATUS,Z                            ; store incoming bit
        bsf     INDF,5

        ; bit 4
        movlw   _SDA_MASK
                                                I2C_DLY I2C_TLOW,4
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:0.6)
                            ;                   |> tHIGH
        bcf     STATUS,RP0                          ; bank 0
        andwf   I2C_PORT,W                          ; read port
        bsf     STATUS,RP0                          ; bank 1
                                                I2C_DLY I2C_THIGH,4
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW
        btfss   STATUS,Z                            ; store incoming bit
        bsf     INDF,4

        ; bit 3
        movlw   _SDA_MASK
                                                I2C_DLY I2C_TLOW,4
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:0.6)
                            ;                   |> tHIGH
        bcf     STATUS,RP0                          ; bank 0
        andwf   I2C_PORT,W                          ; read port
        bsf     STATUS,RP0                          ; bank 1
                                                I2C_DLY I2C_THIGH,4
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW
        btfss   STATUS,Z                            ; store incoming bit
        bsf     INDF,3

        ; bit 2
        movlw   _SDA_MASK
                                                I2C_DLY I2C_TLOW,4
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:0.6)
                            ;                   |> tHIGH
        bcf     STATUS,RP0                          ; bank 0
        andwf   I2C_PORT,W                          ; read port
        bsf     STATUS,RP0                          ; bank 1
                                                I2C_DLY I2C_THIGH,4
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW
        btfss   STATUS,Z                            ; store incoming bit
        bsf     INDF,2

        ; bit 1
        movlw   _SDA_MASK
                                                I2C_DLY I2C_TLOW,4
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:0.6)
                            ;                   |> tHIGH
        bcf     STATUS,RP0                          ; bank 0
        andwf   I2C_PORT,W                          ; read port
        bsf     STATUS,RP0                          ; bank 1
                                                I2C_DLY I2C_THIGH,4
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW
        btfss   STATUS,Z                            ; store incoming bit
        bsf     INDF,1

        ; bit 0 (LSB)
        movlw   _SDA_MASK
                                                I2C_DLY I2C_TLOW,4
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:0.6)
                            ;                   |> tHIGH
        bcf     STATUS,RP0                          ; bank 0
        andwf   I2C_PORT,W                          ; read port
        bsf     STATUS,RP0                          ; bank 1
                                                I2C_DLY I2C_THIGH,4
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW

        ; ACK 1st stage, pull SDA low asap
        bcf     _SDA        ; [SDA LO] tHD_DAT >| (CBUS only, ignored)
                            ;                   |> tSU:DAT

        btfss   STATUS,Z                            ; store incoming bit
        bsf     INDF,0


        ; ACK 2nd stage
                                                I2C_DLY_MAX I2C_TLOW,4, I2C_TSU_DAT,3
        bsf     _SCL        ; [SCL HI]    tLOW >| (4.7:0.6)
                            ;          tSU:DAT >| (0.25:0.1)
                            ;                   |> tHIGH
        bcf     STATUS,Z            ; use this slot to indicate success
                                                I2C_DLY I2C_THIGH,2
        bcf     _SCL        ; [SCL LO]   tHIGH >| (4.0:0.6)
                            ;                   |> tLOW

        decfsz  i2c_cnt,F
        goto    _i2c_rx_loop

        return

#ifdef _I2C_SCL_WAIT

  #ifndef I2C_SCL_TIMEOUT_US
    #define   I2C_SCL_TIMEOUT_US  5000000 ; default 5 seconds
  #endif

;  if (0 > I2C_SCL_TIMEOUT_US) || (I2C_SCL_TIMEOUT_US >

  ; clock-stretch timeout stuff...

;  ; standard way to calculate when overflow/precision are no concern
;  constant wait_iters = I2C_SCL_TIMEOUT_US * ((SYSCLK_HZ / 4) / 1000000) / 16
;
;  if I2C_SCL_TIMEOUT_US <= 1073
;    constant wait_cyc = (I2C_SCL_TIMEOUT_US * 4 * 1000000) / SYSCLK_HZ
;  else
;  if I2C_SCL_TIMEOUT_US <= 4294
;    constant wait_cyc = ((I2C_SCL_TIMEOUT_US * 1000000) / SYSCLK_HZ) * 4
;  else
;  if I2C_SCL_TIMEOUT_US <= 1073741824
;    constant wait_cyc = (I2C_SCL_TIMEOUT_US * 4) / SYSCLK_HZ) * 1073741824

  ; calculate loop params   TODO this all needs more thought...
  constant cyc_per_us = ((SYSCLK_HZ / 4) / 1000000)
  if cyc_per_us > 0
    ; >= 1MHz insr clock
    if I2C_SCL_TIMEOUT_US < 4096
      constant loop_iters = (cyc_per_us * I2C_SCL_TIMEOUT_US) / 16
    else
      constant loop_iters = cyc_per_us * (I2C_SCL_TIMEOUT_US / 16)
    endif
  else
    ; low freq instr clock
    if I2C_SCL_TIMEOUT_US < 4294967
      constant ns_per_cyc = (1000000000 / (SYSCLK_HZ / 4)
      constant timeout_cyc = (I2C_SCL_TIMEOUT_US * 1000) / ns_per_cyc
    else
      constant timeout_cyc = (I2C_SCL_TIMEOUT_US / ((1000000 * 4) / SYSCLK_HZ)) / 16
    endif
    constant loop_iters = timeout_cyc / 16
  endif

  ; messg "I2C_SCL_TIMEOUT_US in cycles: "#v(I2C_SCL_TIMEOUT_US)
  ; messg "loop_iters: "#v(loop_iters)

  ; example: 4000hz processor
  ; wants 10ms timeout
  ; this means 1 cyc is 1000us so we need 10 cycles delay, make sure to round up


; split to 3 byte counters, (where 1: 0 iters, and 0: 255 iters)
#define _I2C_SCL_TO_CNT_H   ((((I2C_SCL_TIMEOUT_US / 10) >> 16) + 1) & 0xFF)
#define _I2C_SCL_TO_CNT_M   ((((I2C_SCL_TIMEOUT_US / 10) >>  8) + 1) & 0xFF)
#define _I2C_SCL_TO_CNT_L   ((((I2C_SCL_TIMEOUT_US / 10) >>  0) + 1) & 0xFF)

; handle target clock-stretching, used by I2C_SCL_WAIT_... macros
    ; pre: bank 0, SCL released/high
    ; post:
    ;   on timeout both STATUS,Z and i2c_stat,i2c_stat,_I2C_ERR_SCL_TO are set
    ;   W preserved
_i2c_scl_wait:
        btfsc   _SCL_IN         ; try again before incurring overhead
        return
        movwf   i2c0_w_tmp     ; save W
        swapf   STATUS,W
        movwf   i2c0_status_tmp

        ; clock-stretch timed with 24-bits, 1 incr is 16 instr cycles
        movlw   _I2C_SCL_TO_CNT_L
        movwf   i2c0_cnt_l
        movlw   _I2C_SCL_TO_CNT_M
        movwf   i2c0_cnt_m
        movlw   _I2C_SCL_TO_CNT_H

_i2c_scl_wait_poll1:
        bcf     i2c_stat,_I2C_ERR_SCL_TO    ; these 2 instr ok to repeat and...
        bcf     STATUS,Z                    ; ...use as delay, saves 1 instr
_i2c_scl_wait_poll2:
        goto    $+1
        nop
_i2c_scl_wait_poll3:
        btfsc   _SCL_IN
        goto    _i2c_scl_wait_exit          ; exit: SCL released

        goto    $+1
        goto    $+1

        btfsc   i2c_ctrl,_I2C_SCL_TO_INF    ; check for infinite timeout
        goto    _i2c_scl_wait

        decfsz  i2c0_cnt_l,F
        goto    _i2c_scl_wait_poll1
        decfsz  i2c0_cnt_m,F
        goto    _i2c_scl_wait_poll2
        addlw   255 ;  decr outer count
        btfss   STATUS,Z
        goto    _i2c_scl_wait_poll3

        bsf     i2c_stat,_I2C_ERR_SCL_TO    ; exit: SCL timeout

_i2c_scl_wait_exit:
        swapf   i2c0_status_tmp,W
        movwf   STATUS
        swapf   i2c0_w_tmp,F                  ; restore W
        swapf   i2c0_w_tmp,W
        return
#endif ; _I2C_SCL_WAIT

#endif ; __I2C_LOW_INC__

