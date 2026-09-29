
use std::collections::HashMap;
use std::sync::Mutex;
use std::os::raw::{c_int, c_uint};

#[derive(Debug, Clone, Copy)]
pub enum DriverStatus {
    Running,
    Crashed,
    Restarting,
    Stopped,
}

pub struct DriverContainer {
    pub task_id: u64,
    pub address_space_id: u64,
    pub status: DriverStatus,
    pub restart_count: u32,
    pub driver_type: String,
}

lazy_static::lazy_static! {
    static ref DRIVER_REGISTRY: Mutex<HashMap<u32, DriverContainer>> = Mutex::new(HashMap::new());
}

#[no_mangle]
pub extern "C" fn iso_spawn_driver(driver_id: u32, driver_type: *const c_char) -> c_int {
    let mut registry = DRIVER_REGISTRY.lock().unwrap();

    // 1. Call L4 to create a totally isolated address space
    let as_id = l4_create_address_space();
    let tid = l4_create_task(as_id);

    let type_str = unsafe { std::ffi::CStr::from_ptr(driver_type).to_string_lossy().into_owned() };

    registry.insert(driver_id, DriverContainer {
        task_id: tid,
        address_space_id: as_id,
        status: DriverStatus::Running,
        restart_count: 0,
        driver_type: type_str,
    });

    0 // SUCCESS
}

#[no_mangle]
pub extern "C" fn iso_monitor_drivers() -> c_int {
    let mut registry = DRIVER_REGISTRY.lock().unwrap();

    for (id, driver) in registry.iter_mut() {
        // Check if the L4 task is still alive
        if !l4_is_task_alive(driver.task_id) {
            println!("[Watchdog] Driver {} crashed! Attempting restart...", driver.driver_type);
            driver.status = DriverStatus::Restarting;

            // Restart Logic:
            // 1. Clean up old address space
            l4_destroy_task(driver.task_id);

            // 2. Re-spawn in a fresh isolation zone
            let new_as = l4_create_address_space();
            driver.task_id = l4_create_task(new_as);
            driver.address_space_id = new_as;
            driver.status = DriverStatus::Running;
            driver.restart_count += 1;

            println!("[Watchdog] Driver {} recovered. Restart count: {}", driver.driver_type, driver.restart_count);
        }
    }
    0
}

// Mock L4 functions for the Rust logic (Actuals are in C/Asm)
fn l4_create_address_space() -> u64 { 0xDEADBEEF }
fn l4_create_task(_as: u64) -> u64 { 0xCAFEBABE }
fn l4_is_task_alive(_tid: u64) -> bool { true }
fn l4_destroy_task(_tid: u64) {}
