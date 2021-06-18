#ifndef __DELAY_INC__
#define __DELAY_INC__

; accurate delay macros/routines

; temp: should be defined on command line?
#ifndef DELAY_BANK
  #define DELAY_BANK 0
#endif

  if (0 > DELAY_BANK) || (DELAY_BANK > 3)
    error "invalid bank defined"
  endif

#if DELAY_BANK == 0
  #define _DELAY_BANK DELAY_BANK0
#endif
#if DELAY_BANK == 1
  #define _DELAY_BANK DELAY_BANK1
#endif
#if DELAY_BANK == 2
  #define _DELAY_BANK DELAY_BANK2
#endif
#if DELAY_BANK == 3
  #define _DELAY_BANK DELAY_BANK3
#endif

DELAY_SHR   UDATA_SHR
    _delay_shr  res 1
_DELAY_BANK  UDATA
    _delay_tmp     res 1

    _delay_cnt_h   res 1
    _delay_cnt_m   res 1
    _delay_cnt_l   res 1

#define DELAY_SETUP_CYCLES  12 ; _delay_setup call cost, update if modified
#define DELAY_MAX_CYCLES    ((255 * 1048576) + (255 * 4096) + 4096 + (DELAY_SETUP_CYCLES + 5))

;-------------------------------------------------------------------------------

; helper macro for selecting correct bank for delay state in non-shared ram
; inlines 2 instr words
DELAY_BANKSEL           MACRO
    if ((DELAY_BANK) % 2) == 1
        bsf     STATUS,RP0
    else
        bcf     STATUS,RP0
    endif

    if (DELAY_BANK) >= 2
        bsf     STATUS,RP1
    else
        bcf     STATUS,RP1
    endif
    ENDM

; TODO: try to use these 3 macros again:

;-------------------------------------------------------------------------------
NANOS_TO_CYCS_ADJ       MACRO   ns, adj
    #ifndef SYSCLK_HZ
        error "NANOS_TO_CYCS_... macros require SYSCLK_HZ to be defined"
    #endif

    local ns_pc = (1000000000 / (SYSCLK_HZ / 4))      ; nanos per cycle
    local cycs = (((ns) + (ns_pc - 1)) / ns_pc)
    if cycs > (adj)
        (cycs - (adj))
    else
        0
    endif
    ENDM

;-------------------------------------------------------------------------------
NANOS_TO_CYCS           MACRO   ns
    NANOS_TO_CYCS_ADJ (ns), 0
    ENDM

;-------------------------------------------------------------------------------
NANOS_TO_CYCS_ADJ_MAX   MACRO   ns_1, ns_2
    local cycs_1 = NANOS_TO_CYCS_ADJ (ns_1)
    local cycs_2 = NANOS_TO_CYCS_ADJ (ns_2)

    if cycs_1 > cycs_2
        cycs_1
    else
        cycs_2
    endif
    ENDM

;-------------------------------------------------------------------------------
; Spin for `ns` nanoseconds rounded up to the nearest instruction cycle
; interval less `adj` cycle intervals.
; Inlines instructions as per SPIN_CYCLES above.
; STATUS register is preseved, W is overwritten.
; This macro relies on the symbol SYSCLK_HZ evaluating to an accurate system
; oscillator frequency.
;
; Assume that after this macro PCLATH:3 (and PCLATH:4 if ROM > 4K) will reflect
; the local address invoking the macro, this is a best effort to minimise
; inconvenience to caller. Avoid setting up PCLATH in preparation for a
; call/goto before invoking this macro, instead wait until after invocation to
; prepare PCLATH.
;
; delays n instr. cycles, call depth <= 1, preserves STATUS and W
;   instr. overhead for values of n:
;     1,2,4 ..=      16 -> 1 instr. word
;      ..=       ->  instr. word

DELAY_NS_ADJ        MACRO   ns, adj
        #ifndef SYSCLK_HZ
          error "DELAY_xS... macros require SYSCLK_HZ to be defined"
        #endif
        local ns_pc = ((1000 * 1000 * 1000) / ((SYSCLK_HZ) / 4))    ; ns per cycle
        local cycs = ((ns) + (ns_pc - 1)) / ns_pc
        if cycs > (adj)
            DELAY_CYCLES (cycs - (adj))
        endif
    ENDM

DELAY_US_ADJ        MACRO   us, adj
        #ifndef SYSCLK_HZ
          error "DELAY_xS... macros require SYSCLK_HZ to be defined"
        #endif
        local ns_pc = ((1000 * 1000 * 1000) / ((SYSCLK_HZ) / 4))    ; us per cycle
        local cycs = (((us) * 1000) + (ns_pc - 1)) / ns_pc
        if cycs > (adj)
            DELAY_CYCLES (cycs - (adj))
        endif
    ENDM

;-------------------------------------------------------------------------------
; Spin for `ns` nanoseconds rounded up to the nearest instruction cycle.
; Inlines instructions as per DELAY_CYCLES above.
; STATUS register is preseved, W is overwritten.
; This macro relies on the symbol SYSCLK_HZ evaluating to an accurate system
; oscillator frequency.
DELAY_NS            MACRO   ns
    DELAY_NS_ADJ (ns), 0
    ENDM

DELAY_US            MACRO   us
    DELAY_US_ADJ ((us) * 1000), 0
    ENDM

DELAY_MS            MACRO   ms
    DELAY_US_ADJ ((ms) * 1000), 0
    ENDM

DELAY_S             MACRO   s
    DELAY_US_ADJ ((s) * (1000 * 1000)), 0
    ENDM

;------------------------
; delay debug macro
_DELAY_DBG          MACRO   n, n_rem, cnt_h, cnt_m, cnt_l, cnt_mod, overhead
;    messg   "n:         "(n)
    messg   "#n:        "#v((n))

;    messg   " n_rem:    "(n_rem)
    messg   "#n_rem:    "#v((n_rem))

;    messg   " cnt_h:    "(cnt_h)
    messg   "#cnt_h:    "#v((cnt_h))

;    messg   " cnt_m:    "(cnt_m)
    messg   "#cnt_m:    "#v((cnt_m))

;    messg   " cnt_l:    "(cnt_l)
    messg   "#cnt_l:    "#v((cnt_l))

;    messg   " cnt_mod:  "(cnt_mod)
    messg   "#cnt_mod:  "#v((cnt_mod))

;    messg   " overhead: "(cnt_mod)
    messg   "#overhead: "#v((overhead))
    ENDM

;-------------------------------------------------------------------------------
; main delay macro
DELAY_CYCLES       MACRO   n
    if n > DELAY_MAX_CYCLES
        error "DELAY_CYCLES: cycle count max exceeded: " #v(n)
        exitm
    endif

    if n < 0
      messg "DELAY_CYCLES: warning, negative cycle count"
    endif

    if n <= 0
      messg "DELAY_CYCLES: [DEBUG] --- 0 cycle delay ---"
      exitm
    endif

    ; note: if 32 > n > (32 + DELAY_SETUP_CYCLES + 1) then we just use 3
    ; _delay_x instructions where x is 16,16,(n % 16) in the next section
    ; which is ok unless _delay_setup grows above 15 cycles

    ; similarly. if 4^12 < n < (4^12 + 2)...TODO explain this

    ; loop overheads:
    local OH_L  = DELAY_SETUP_CYCLES + 1
    local OH_M  = DELAY_SETUP_CYCLES + 1 + 2
    local OH_H  = DELAY_SETUP_CYCLES + 1 + 2 + 2

    ; loop depth thresholds:
    local DELAY_THRESH_L = 32 + OH_L
    local DELAY_THRESH_M = (4096 + OH_M) - 16
    local DELAY_THRESH_H = (1048576 + OH_H) - 16

    local overhead

    if (n) >= DELAY_THRESH_L
      overhead = OH_L
    else
    if (n) >= DELAY_THRESH_M
      overhead = OH_M
    else
    if (n) >= DELAY_THRESH_H
      overhead = OH_H
    else
      overhead = 0
    endif
    endif
    endif

    local n_rem = (n) - overhead

    ; *** temp: PCLATH/pagesel fix: ***
    ; emergency fix for delay calls going AWOL when crossing code pages
    ; TODO this is bodged, need to take these pagesels into account...
    if (n) > 3
      n_rem -= 2 ; only accurate for ROM size <= 4K

                pagesel _DELAY_PAGE
    endif

    ; small delays:

    if (n) < DELAY_THRESH_L
      local n_delt
      while n_rem >= 16
        n_delt = 16
        if (n_rem % 16) == 3    ; avoid awkward remainder of 3
          n_delt = 15
        endif
                call    _delay_#v(n_delt)
          n_rem -= n_delt
      endw
      if n_rem >= 4
                call    _delay_#v(n_rem)
        n_rem -= n_rem
      else
        if n_rem >= 2
                goto    $+1
          n_rem -= 2
        endif
        if (n_rem % 2) == 1
                nop
          n_rem -= 1
        endif
      endif

      if n_rem != 0
        error "DELAY_CYCLES: n_rem not zero at end"
      endif

      ; *** temp: PCLATH/pagesel fix: ***
      if (n) > 3
                pagesel $
      endif

      exitm
    endif

    ; looped delays

    local cnt_h   = 0
    local cnt_m   = 0
    local cnt_l   = 0
    local cnt_mod = 0

    local hm_adj = 0
    if (n) >= DELAY_THRESH_M
      hm_adj = 16
    endif

    cnt_h   = n_rem / 1048576
    cnt_m   = (n_rem % 1048576) / 4096
    n_rem  -= ((cnt_h * 1048576) + (cnt_m * 4096)) - hm_adj

    ; lower/inner loop can be configured from 16 to 4096 + mod cycles,
    ; i.e. 16 * ((0..=255) + 1) + (0..=15) == 16..=4111
    if (n_rem < 16) || (n_rem > 4111)
      error "DELAY_CYCLES: n_rem out of range"
      _DELAY_DBG  n, n_rem, cnt_h, cnt_m, cnt_l, cnt_mod, ovehead
      exitm
    endif

    cnt_l   = (n_rem / 16) % 256
    cnt_mod = n_rem % 16

;    n_rem -= ((cnt_l + 1) % 257 ) + cnt_mod
    n_rem -= (((cnt_l + 255) % 256) + 1) * 16 + cnt_mod

    if n_rem != 0
      error "DELAY_CYCLES: n_rem not zero at end"
      _DELAY_DBG  n, n_rem, cnt_h, cnt_m, cnt_l, cnt_mod, overhead
      exitm
    endif

    ; recalc check....
    local cycs_hm = (cnt_h * 1048576) + (cnt_m * 4096) - hm_adj
    local cycs_l  = ((((cnt_l + 255) % 256) + 1) * 16)
    local recalc = overhead + cycs_hm + cycs_l + cnt_mod

    ; *** temp: PCLATH/pagesel fix: ***
    recalc += 2

    if (n) != recalc
      error "DELAY_CYCLES: error in recalc, value: "#v(recalc)
      _DELAY_DBG  n, n_rem, cnt_h, cnt_m, cnt_l, cnt_mod, overhead
      exitm
    endif

    ; code gen
;    _DELAY_DBG  n, n_rem, cnt_h, cnt_m, cnt_l, cnt_mod, ovehead
    if (n) >= DELAY_THRESH_L
                call    _delay_setup
      if (n) >= DELAY_THRESH_H
                movlw   cnt_h
                movwf   _delay_cnt_h
      endif
      if (n) >= DELAY_THRESH_M
                movlw   cnt_m
                movwf   _delay_cnt_m
      endif
                movlw   cnt_l
                call    _delay_mod_#v(cnt_mod)
    endif
    ; *** temp: PCLATH/pagesel fix: ***
    if (n) > 3
                pagesel $
    endif

    ENDM

DELAY_PAGE      CODE
_DELAY_PAGE:

; delay `W` seconds, 0 implies 256
delay_w_s:
        DELAY_US_ADJ  1000,5
        sublw     1
        btfsc     STATUS,Z
        return
        goto    delay_w_us

; delay `W` microseconds, 0 implies 256
delay_w_us:
        DELAY_US_ADJ  1000,5
        sublw     1
        btfsc     STATUS,Z
        return
        goto    delay_w_us

; delay `W` milliseconds, 0 implies 256
delay_w_ms:
        DELAY_US_ADJ  1000,5
        sublw     1
        btfsc     STATUS,Z
        return
        goto    delay_w_ms


;   * IDEA: 'status-loop' (macro?) allowing up to 8 iterations
;           use STATUS,{Z,C,DC} as count bits
;           affords longer 2 instr delays
;           tricky timing calcs
;
;   * IDEA: dynamic delay routine
;           would have lower bound of [call + ...checks... + ret]
;           maybe 8,16,24 bit variants, passed ptr to 1,2,3 byte delay value
;           tricky timing calcs

;---------------------------------------
; delay loop setup:                         ; 12 cycles
;   NOTE: always ensure DELAY_SETUP_CYCLES definition is accurate at top of file
_delay_setup:
                movwf   _delay_shr          ; 1     save W
                swapf   STATUS,W            ; 1     save STATUS

                DELAY_BANKSEL               ; 2     setup RP bits for delay var
                movwf   _delay_tmp          ; 1     store saved W (nib-swapped)

                clrf    _delay_cnt_h        ; 1
                clrf    _delay_cnt_m        ; 1

                swapf   _delay_shr,F        ; 1     prepare saved W for swapf restore
                return                      ; 2 + [2]

;---------------------------------------
; delay loop impl.
;
; - provides cycle accurate delay with a max. call depth of 1
; - uses 1 shared and 3 banked ram locations, W and STATUS lrft untouched
;
; cycle count:
;             _delay_count_h * 2^20     : outer iteration
;   +         _delay_count_m * 2^12     : mid iteration
;   + ((W + 255) % 256) + 1) *  2^4     : inner iteration
;   +              <mod_# entry>        : n % 16 adjustment
;
; the above, whilst correct  is a simplification of how the loop iterations sum
; to the total cycle count, in reality there are Wadd tobit annoying but unavaoidable, there is no way to make W
; consistent with mid and high params due to construction of the loop,
; macro calculation needs to be:
;
;   example:    n == 8192
;       we need h: 0, m: 2, W: 1
;
;   cnt_h == n / 2^20
;   cnt_m == (n % 2^20) / 2^12
;   cnt_l == (n % 2^12) / 2^4
;   W     == (cnt_l + 1) % 256
;
_delay_mod_16:  movwf   _delay_cnt_l        ;           1
_delay_mod_15:  movwf   _delay_cnt_l        ;           1
_delay_mod_14:  movwf   _delay_cnt_l        ;           1
_delay_mod_13:  movwf   _delay_cnt_l        ;           1
_delay_loop:                                ; ___
_delay_mod_12:  movwf   _delay_cnt_l        ;  ^        1
_delay_mod_11:  movwf   _delay_cnt_l        ;  |        1
_delay_mod_10:  movwf   _delay_cnt_l        ;  |        1
_delay_mod_9:   movwf   _delay_cnt_l        ;  |        1
_delay_mod_8:   movwf   _delay_cnt_l        ;  |        1
_delay_mod_7:   movwf   _delay_cnt_l        ;  |        1
_delay_mod_6:   movwf   _delay_cnt_l        ;  |        1
_delay_mod_5:   movwf   _delay_cnt_l        ;  | 16     1
_delay_mod_4:   movwf   _delay_cnt_l        ;  |        1
_delay_mod_3:   movwf   _delay_cnt_l        ;  |        1
_delay_mod_2:   movwf   _delay_cnt_l        ;  |        1
_delay_mod_1:   movwf   _delay_cnt_l        ;  |   ___  1
_delay_mod_0:   movwf   _delay_cnt_l        ;  |    ^   1
                decfsz  _delay_cnt_l,W      ;  |    |   1 (2)
                goto    _delay_loop         ; _v_   |   2
                                            ;       |
                movf    _delay_cnt_m,W      ;       |   1
                btfss   STATUS,Z            ;       |   1 (2)
                goto    _delay_m_iter       ;       |   2
                                            ;    16 |
                movf    _delay_cnt_h,W      ;       |   1
                btfss   STATUS,Z            ;       |   1 (2)
                goto    _delay_h_iter       ;       |   2
                                            ;       |
                ; exit: restore regs        ;       |
                swapf   _delay_tmp,W        ;       |   1
                movwf   STATUS              ;       |   1
                swapf   _delay_shr,W        ;       |   1
                return                      ;      _v_  2 + [2]
                                            ;
_delay_m_iter:  decf    _delay_cnt_m,F      ;           1
                movlw   0                   ;           1
                goto    _delay_mod_5        ;           2
                                            ;
_delay_h_iter:  decf    _delay_cnt_m,F      ;           1
                decf    _delay_cnt_h,F      ;           1
                movlw   0                   ;           1
                goto    _delay_mod_1        ;           2

;---------------------------------------
; single instr delay 4..=16 cycles
_delay_16:      nop                     ; 1
_delay_15:      nop                     ; 1
_delay_14:      nop                     ; 1
_delay_13:      nop                     ; 1
_delay_12:      nop                     ; 1
_delay_11:      nop                     ; 1
_delay_10:      nop                     ; 1
_delay_9:       nop                     ; 1
_delay_8:       nop                     ; 1
_delay_7:       nop                     ; 1
_delay_6:       nop                     ; 1
_delay_5:       nop                     ; 1
_delay_4:       return                  ; 2 [2]

; ------------------------------------------------
; DELAY_CYCLES tests

;_DELAY_TEST_GEN         MACRO   l, h
;    if (l) > (h)
;        error "_DELAY_TEST_GEN: l must be lower than h"
;    endif
;
;    local i = l
;    while i <= (h)
;_delay_test_#v(i):
;        DELAY_CYCLES    #v(i)
;        i += 1
;    endw
;
;    ENDM
;
;    ; small delay tests
;    _DELAY_TEST_GEN     0,                  64
;
;    ; mid delay boundary tests
;    _DELAY_TEST_GEN     (4096 - 32),        (4096 + 32)
;
;    ; high delay boundary tests
;    _DELAY_TEST_GEN     (0x100000 - 32),    (0x100000 + 32)

#endif ; __DELAY_INC__
; timing analysis:
;_delay_mod      h: 1    m: 1    w: 0    0
;    "           h: 1    m: 1    w: 255  16
;    "           h: 1    m: 1    w: 254  32
;    "           h: 1    m: 1    w: 253  48
;...
;    "           h: 1    m: 1    w: 1    4080
;    (m_iter)
;    "           h: 1    m: 0    w: 0    4096
;    "           h: 1    m: 0    w: 255  4112
;...
;    "           h: 1    m: 0    w: 1    8076
;    (h_iter)
;    "           h: 0    m: 255  w: 0    8192
;    "           h: 0    m: 255  w: 255  8208
;...
;    "           h: 0    m: 255    w: 1   12272
;    (m_iter)
;    "           h: 0    m: 254    w: 0   12288
;...
;    "           h: 0    m: 254    w: 1   12272
;    (m_iter)
;    "           h: 0    m: 253    w: 0   12288
;...


