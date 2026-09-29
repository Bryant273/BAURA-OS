
#include <stdint.h>
#include "L4_TCB.h"

// Comprehensive Exception Handler for x86_64
void handle_exception(uint64_t vector, uint64_t error_code, void* stack_frame) {
    switch(vector) {
        case 0: // Division by Zero
            kernel_log("CRITICAL: Division by zero in Task %d", current_task->pid);
            l4_terminate_task(current_task, "DIV_ZERO");
            break;
        case 13: // General Protection Fault (GPF)
            // GPF usually means an app tried to access kernel memory
            kernel_log("SECURITY: GPF detected. Task %d tried to access unauthorized memory. Error: %x",
                       current_task->pid, error_code);
            l4_terminate_task(current_task, "GPF_VIOLATION");
            break;
        case 14: // Page Fault
            // Detailed Page Fault analysis
            uintptr_t fault_addr = read_cr2();
            if (vmm_is_address_valid(current_task->as, fault_addr)) {
                vmm_handle_demand_paging(current_task->as, fault_addr);
            } else {
                kernel_log("SEGFAULT: Task %d accessed invalid address %p", current_task->pid, (void*)fault_addr);
                l4_terminate_task(current_//task, "SEGFAULT");
            }
            break;
        case 18: // Machine Check Exception (MCE)
            // This is a HARDWARE failure (e.g., overheating or RAM error)
            kernel_log("HARDWARE FAILURE: Machine Check Exception. System halting to prevent damage.");
            halt_system_immediate();
            break;
        default:
            kernel_log("Unhandled Exception Vector %d", vector);
            break;
    }
}

// IRQ Nesting Logic: Allows higher priority interrupts to interrupt lower ones
void irq_nested_handler(uint64_t vector) {
    uint64_t flags = read_eflags();
    // Save current flags and enable interrupts for nested processing
    push_flags(flags);
    asm volatile("sti");

    irq_dispatch(vector);

    // Restore flags to original state
    pop_flags();
}
