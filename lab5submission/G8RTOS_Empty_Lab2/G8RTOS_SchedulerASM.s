; G8RTOS_SchedulerASM.s
; Holds all ASM functions needed for the scheduler
; Note: If you have an h file, do not have a C file and an S file of the same name

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler, StartCriticalSection, EndCriticalSection ; added crit section dependencies

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file 
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc
	; Implement this
	LDR R0, RunningPtr
	LDR R1, [R0]
	LDR SP, [R1];, #8]
	POP {R4 - R11}
	POP {R0 - R3}
	POP {R12}
	ADD SP, SP, #4 ;POP {LR}	; replace with ADD?
	POP {LR}
	ADD SP, SP, #4
	;MOV LR, PC
	CPSIE I		; enable interrupts
	BX LR		; branch to first thread
	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler:
	
	.asmfunc
	;Implement this
	MOV R3, LR		;save EXC_RETURN value
	BL StartCriticalSection
	MOV R2, R0

	PUSH {R4 - R11}

	LDR R0, RunningPtr
	LDR R1, [R0]
	STR SP, [R1];, #8]

	BL G8RTOS_Scheduler

	LDR R0, RunningPtr
	LDR R1, [R0]
	LDR SP, [R1];, #8]

	POP {R4 - R11}

	MOV R0, R2
	BL EndCriticalSection

	MOV LR, R3
	BX LR

	.endasmfunc
	
	; end of the asm file
	.align
	.end
