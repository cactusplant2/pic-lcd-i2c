#ifndef __UTIL_INC__
#define __UTIL_INC__

; general purpose macros and routines

; output pin macros

#define PIN_OUT_PD   0
#define PIN_OUT_PU   1
#define PIN_OUT_PP   2

PIN_OUT_DEF    MACRO    name, port_reg, tris_reg, pin, mode
    name#v(0)_port  equ port_reg
    name#v(0)_tris  equ tris_reg
    name#v(0)_pin   equ pin
    name#v(0)_mode  equ mode
    ENDM

PIN_OUT_INIT  MACRO     name
    if PIN_OUT_PD > name#v(0)_mode || name#v(0)_mode > PIN_OUT_PP
      error "PIN_OUT_INIT: invalid mode selected, use {PIN_OUT_PD, PIN_OUT_PU, PIN_OUT_PP}"
    endif
    if name#v(0)_mode == PIN_OUT_PD
        banksel name#v(0)_tris
        bsf     name#v(0)_tris,name#v(0)_pin
        banksel name#v(0)_port
        bcf     name#v(0)_port,name#v(0)_pin
    endif
    if name#v(0)_mode == PIN_OUT_PU
        banksel name#v(0)_tris
        bsf     name#v(0)_tris,name#v(0)_pin
        banksel name#v(0)_port
        bsf     name#v(0)_port,name#v(0)_pin
    endif
    if name#v(0)_mode == PIN_OUT_PP
        banksel name#v(0)_port
        bcf     name#v(0)_port,name#v(0)_pin
        banksel name#v(0)_tris
        bcf     name#v(0)_tris,name#v(0)_pin
    endif
    ENDM

PIN_OUT_HI      MACRO   name
    if name#v(0)_mode == PIN_OUT_PD
        banksel name#v(0)_tris
        bsf     name#v(0)_tris,name#v(0)_pin
    endif
    if name#v(0)_mode == PIN_OUT_PU
        banksel name#v(0)_tris
        bcf     name#v(0)_tris,name#v(0)_pin
    endif
    if name#v(0)_mode == PIN_OUT_PP
        banksel name#v(0)_port
        bsf     name#v(0)_port,name#v(0)_pin
    endif
    ENDM

PIN_OUT_RAW_HI  MACRO   name
    if name#v(0)_mode == PIN_OUT_PD
        bsf     name#v(0)_tris,name#v(0)_pin
    endif
    if name#v(0)_mode == PIN_OUT_PU
        bcf     name#v(0)_tris,name#v(0)_pin
    endif
    if name#v(0)_mode == PIN_OUT_PP
        bsf     name#v(0)_port,name#v(0)_pin
    endif
    ENDM

PIN_OUT_LO      MACRO   name
    if name#v(0)_mode == PIN_OUT_PD
        banksel name#v(0)_tris
        bcf     name#v(0)_tris,name#v(0)_pin
    endif
    if name#v(0)_mode == PIN_OUT_PU
        banksel name#v(0)_tris
        bsf     name#v(0)_tris,name#v(0)_pin
    endif
    if name#v(0)_mode == PIN_OUT_PP
        banksel name#v(0)_port
        bcf     name#v(0)_port,name#v(0)_pin
    endif
    ENDM

PIN_OUT_RAW_LO  MACRO   name
    if name#v(0)_mode == PIN_OUT_PD
        bcf     name#v(0)_tris,name#v(0)_pin
    endif
    if name#v(0)_mode == PIN_OUT_PU
        bsf     name#v(0)_tris,name#v(0)_pin
    endif
    if name#v(0)_mode == PIN_OUT_PP
        bcf     name#v(0)_port,name#v(0)_pin
    endif
    ENDM

PIN_OUT_TOG     MACRO   name
    local mask = 1 << #v(name#v(0)_pin)
        movlw   mask
    if name#v(0)_mode == PIN_OUT_PD
        banksel name#v(0)_tris
        xorwf   name#v(0)_tris,F
    endif
    if name#v(0)_mode == PIN_OUT_PU
        banksel name#v(0)_tris
        xorwf   name#v(0)_tris,F
    endif
    if name#v(0)_mode == PIN_OUT_PP
        banksel name#v(0)_port
        xorwf   name#v(0)_port,F
    endif
    ENDM

; STATUS saving/restore macros

; saves m1 to m2 leaving m2 previous contents in W
MEM_SWAPW     MACRO   m1, m2
        movf    (m1),W
        xorwf   (m2),F
        xorwf   (m2),W
        xorwf   (m2),F
    ENDM

; saves m1 to m2 leaving W unmodified
MEM_SAVE      MACRO   m1, m2
        movwf   (m2)
        MEM_SWAPW (m1),(m2)
    ENDM

STATUS_SAVE   MACRO   m
    MEM_SAVE    STATUS, (m)
    ENDM

STATUS_RSTR   MACRO   m
        movf    (m),W
        movwf   STATUS
    ENDM

; restore m to STATUS with fl_reg,fl_bit merged to STATUS,st_bit
STATUS_RSTR_FLG MACRO   m, fl_reg, fl_bit, st_bit
        bcf     (m),(st_bit)
        btfsc   (fl_reg),(fl_bit)
        bsf     (m),(st_bit)

        STATUS_RSTR (m)
    ENDM




UTIL_PAGE     CODE
_UTIL_PAGE:



#endif ; __UTIL_INC__
