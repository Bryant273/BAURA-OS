use std::ffi::{c_char, c_void, CStr};
use std::mem::size_of;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Mutex, MutexGuard};

pub const BAURA_DRIVER_ISO_ABI_VERSION: u32 = 1;
pub const BAURA_DRIVER_ISO_MAX_DRIVERS: usize = 64;
pub const BAURA_DRIVER_ISO_MAX_TYPE_BYTES: usize = 127;

pub const BAURA_DRIVER_ISO_OK: i32 = 0;
pub const BAURA_DRIVER_ISO_ERR_ARGUMENT: i32 = -1;
pub const BAURA_DRIVER_ISO_ERR_UNAVAILABLE: i32 = -2;
pub const BAURA_DRIVER_ISO_ERR_CAPACITY: i32 = -3;
pub const BAURA_DRIVER_ISO_ERR_NOT_FOUND: i32 = -4;
pub const BAURA_DRIVER_ISO_ERR_BACKEND: i32 = -5;
pub const BAURA_DRIVER_ISO_ERR_ALREADY_EXISTS: i32 = -6;
pub const BAURA_DRIVER_ISO_ERR_LIMIT: i32 = -7;
pub const BAURA_DRIVER_ISO_ERR_INVALID_STATE: i32 = -8;

const DRIVER_RUNNING: u32 = 1;
const DRIVER_CRASHED: u32 = 2;
const DRIVER_STOPPING: u32 = 3;
const DRIVER_RESTARTING: u32 = 4;

pub type SpawnDriver =
    unsafe extern "C" fn(*mut c_void, u32, *const c_char, *mut u64, *mut u64) -> i32;
pub type IsTaskAlive = unsafe extern "C" fn(*mut c_void, u64, *mut u8) -> i32;
pub type TerminateTask = unsafe extern "C" fn(*mut c_void, u64, u64) -> i32;

#[repr(C)]
pub struct BauraDriverIsolationBackend {
    pub abi_version: u32,
    pub struct_size: u32,
    pub context: *mut c_void,
    pub spawn_driver: Option<SpawnDriver>,
    pub is_task_alive: Option<IsTaskAlive>,
    pub terminate_task: Option<TerminateTask>,
}

#[derive(Clone, Copy)]
struct PlatformBackend {
    context: usize,
    spawn_driver: SpawnDriver,
    is_task_alive: IsTaskAlive,
    terminate_task: TerminateTask,
}

struct DriverContainer {
    driver_id: u32,
    task_id: u64,
    address_space_id: u64,
    status: u32,
    restart_count: u32,
    driver_type: String,
}

static PLATFORM_BACKEND: Mutex<Option<PlatformBackend>> = Mutex::new(None);
static DRIVER_REGISTRY: Mutex<Vec<DriverContainer>> = Mutex::new(Vec::new());
static PENDING_SPAWNS: AtomicUsize = AtomicUsize::new(0);

struct PendingSpawn;

impl Drop for PendingSpawn {
    fn drop(&mut self) {
        PENDING_SPAWNS.fetch_sub(1, Ordering::Release);
    }
}

fn lock_recover<T>(mutex: &Mutex<T>) -> MutexGuard<'_, T> {
    mutex.lock().unwrap_or_else(std::sync::PoisonError::into_inner)
}

fn current_backend() -> Result<PlatformBackend, i32> {
    lock_recover(&PLATFORM_BACKEND)
        .as_ref()
        .copied()
        .ok_or(BAURA_DRIVER_ISO_ERR_UNAVAILABLE)
}

fn terminate_backend_task(
    backend: PlatformBackend,
    task_id: u64,
    address_space_id: u64,
) {
    if task_id != 0 {
        unsafe {
            (backend.terminate_task)(
                backend.context as *mut c_void,
                task_id,
                address_space_id,
            );
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn iso_set_platform_backend(
    backend: *const BauraDriverIsolationBackend,
) -> i32 {
    let mut installed = lock_recover(&PLATFORM_BACKEND);
    let drivers = lock_recover(&DRIVER_REGISTRY);
    if !drivers.is_empty() || PENDING_SPAWNS.load(Ordering::Acquire) != 0 {
        return BAURA_DRIVER_ISO_ERR_INVALID_STATE;
    }
    if backend.is_null() {
        *installed = None;
        return BAURA_DRIVER_ISO_OK;
    }

    let supplied = &*backend;
    if supplied.abi_version != BAURA_DRIVER_ISO_ABI_VERSION
        || supplied.struct_size < size_of::<BauraDriverIsolationBackend>() as u32
    {
        return BAURA_DRIVER_ISO_ERR_ARGUMENT;
    }
    let (Some(spawn_driver), Some(is_task_alive), Some(terminate_task)) = (
        supplied.spawn_driver,
        supplied.is_task_alive,
        supplied.terminate_task,
    ) else {
        return BAURA_DRIVER_ISO_ERR_ARGUMENT;
    };

    *installed = Some(PlatformBackend {
        context: supplied.context as usize,
        spawn_driver,
        is_task_alive,
        terminate_task,
    });
    BAURA_DRIVER_ISO_OK
}

#[no_mangle]
pub unsafe extern "C" fn iso_spawn_driver(
    driver_id: u32,
    driver_type: *const c_char,
) -> i32 {
    if driver_id == 0 || driver_type.is_null() {
        return BAURA_DRIVER_ISO_ERR_ARGUMENT;
    }
    let name = CStr::from_ptr(driver_type);
    if name.to_bytes().is_empty() {
        return BAURA_DRIVER_ISO_ERR_ARGUMENT;
    }
    if name.to_bytes().len() > BAURA_DRIVER_ISO_MAX_TYPE_BYTES {
        return BAURA_DRIVER_ISO_ERR_LIMIT;
    }
    let Ok(driver_type) = name.to_str() else {
        return BAURA_DRIVER_ISO_ERR_ARGUMENT;
    };

    let backend = {
        let installed = lock_recover(&PLATFORM_BACKEND);
        let drivers = lock_recover(&DRIVER_REGISTRY);
        let Some(backend) = installed.as_ref().copied() else {
            return BAURA_DRIVER_ISO_ERR_UNAVAILABLE;
        };
        if drivers.iter().any(|driver| driver.driver_id == driver_id) {
            return BAURA_DRIVER_ISO_ERR_ALREADY_EXISTS;
        }
        if drivers.len() >= BAURA_DRIVER_ISO_MAX_DRIVERS {
            return BAURA_DRIVER_ISO_ERR_CAPACITY;
        }
        PENDING_SPAWNS.fetch_add(1, Ordering::AcqRel);
        backend
    };
    let _pending_spawn = PendingSpawn;

    let mut task_id = 0;
    let mut address_space_id = 0;
    let spawn_status = (backend.spawn_driver)(
        backend.context as *mut c_void,
        driver_id,
        name.as_ptr(),
        &mut task_id,
        &mut address_space_id,
    );
    if spawn_status != 0 || task_id == 0 || address_space_id == 0 {
        terminate_backend_task(backend, task_id, address_space_id);
        return BAURA_DRIVER_ISO_ERR_BACKEND;
    }

    let mut drivers = lock_recover(&DRIVER_REGISTRY);
    if drivers.iter().any(|driver| driver.driver_id == driver_id) {
        drop(drivers);
        terminate_backend_task(backend, task_id, address_space_id);
        return BAURA_DRIVER_ISO_ERR_ALREADY_EXISTS;
    }
    if drivers.len() >= BAURA_DRIVER_ISO_MAX_DRIVERS {
        drop(drivers);
        terminate_backend_task(backend, task_id, address_space_id);
        return BAURA_DRIVER_ISO_ERR_CAPACITY;
    }
    drivers.push(DriverContainer {
        driver_id,
        task_id,
        address_space_id,
        status: DRIVER_RUNNING,
        restart_count: 0,
        driver_type: driver_type.to_owned(),
    });
    BAURA_DRIVER_ISO_OK
}

#[no_mangle]
pub extern "C" fn iso_monitor_drivers() -> i32 {
    let backend = match current_backend() {
        Ok(backend) => backend,
        Err(error) => return error,
    };
    let snapshot: Vec<(u32, u64, u64)> = {
        let drivers = lock_recover(&DRIVER_REGISTRY);
        drivers
            .iter()
            .filter(|driver| driver.status == DRIVER_RUNNING)
            .map(|driver| {
                (
                    driver.driver_id,
                    driver.task_id,
                    driver.address_space_id,
                )
            })
            .collect()
    };

    let mut first_error = BAURA_DRIVER_ISO_OK;
    for (driver_id, task_id, address_space_id) in snapshot {
        let claimed = {
            let mut drivers = lock_recover(&DRIVER_REGISTRY);
            if let Some(driver) = drivers.iter_mut().find(|driver| {
                driver.driver_id == driver_id
                    && driver.task_id == task_id
                    && driver.status == DRIVER_RUNNING
            }) {
                driver.status = DRIVER_RESTARTING;
                true
            } else {
                false
            }
        };
        if !claimed {
            continue;
        }

        let mut alive = 0;
        let query_status = unsafe {
            (backend.is_task_alive)(
                backend.context as *mut c_void,
                task_id,
                &mut alive,
            )
        };
        if query_status != 0 {
            if let Some(driver) = lock_recover(&DRIVER_REGISTRY)
                .iter_mut()
                .find(|driver| driver.driver_id == driver_id && driver.task_id == task_id)
            {
                driver.status = DRIVER_RUNNING;
            }
            if first_error == BAURA_DRIVER_ISO_OK {
                first_error = BAURA_DRIVER_ISO_ERR_BACKEND;
            }
            continue;
        }
        if alive > 1 {
            if let Some(driver) = lock_recover(&DRIVER_REGISTRY)
                .iter_mut()
                .find(|driver| driver.driver_id == driver_id && driver.task_id == task_id)
            {
                driver.status = DRIVER_RUNNING;
            }
            if first_error == BAURA_DRIVER_ISO_OK {
                first_error = BAURA_DRIVER_ISO_ERR_BACKEND;
            }
            continue;
        }
        if alive != 0 {
            if let Some(driver) = lock_recover(&DRIVER_REGISTRY)
                .iter_mut()
                .find(|driver| driver.driver_id == driver_id && driver.task_id == task_id)
            {
                driver.status = DRIVER_RUNNING;
            }
            continue;
        }

        let terminate_status = unsafe {
            (backend.terminate_task)(
                backend.context as *mut c_void,
                task_id,
                address_space_id,
            )
        };
        let mut new_task_id = 0;
        let mut new_address_space_id = 0;
        let spawn_status = if terminate_status == 0 {
            let driver_type = {
                let drivers = lock_recover(&DRIVER_REGISTRY);
                drivers
                    .iter()
                    .find(|driver| driver.driver_id == driver_id)
                    .map(|driver| driver.driver_type.clone())
            };
            if let Some(driver_type) = driver_type {
                match std::ffi::CString::new(driver_type) {
                    Ok(driver_type) => unsafe {
                        (backend.spawn_driver)(
                            backend.context as *mut c_void,
                            driver_id,
                            driver_type.as_ptr(),
                            &mut new_task_id,
                            &mut new_address_space_id,
                        )
                    },
                    Err(_) => BAURA_DRIVER_ISO_ERR_ARGUMENT,
                }
            } else {
                BAURA_DRIVER_ISO_ERR_NOT_FOUND
            }
        } else {
            terminate_status
        };

        let mut cleanup_new_task = false;
        let updated = {
            let mut drivers = lock_recover(&DRIVER_REGISTRY);
            if let Some(driver) = drivers
                .iter_mut()
                .find(|driver| {
                    driver.driver_id == driver_id
                        && driver.task_id == task_id
                        && driver.status == DRIVER_RESTARTING
                })
            {
                if spawn_status == 0 && new_task_id != 0 && new_address_space_id != 0 {
                    driver.task_id = new_task_id;
                    driver.address_space_id = new_address_space_id;
                    driver.status = DRIVER_RUNNING;
                    driver.restart_count = driver.restart_count.saturating_add(1);
                } else {
                    driver.status = DRIVER_CRASHED;
                    cleanup_new_task = new_task_id != 0;
                }
                true
            } else {
                false
            }
        };

        if !updated {
            cleanup_new_task = new_task_id != 0;
            if first_error == BAURA_DRIVER_ISO_OK {
                first_error = BAURA_DRIVER_ISO_ERR_INVALID_STATE;
            }
        } else if (spawn_status != 0 || new_task_id == 0 || new_address_space_id == 0)
            && first_error == BAURA_DRIVER_ISO_OK
        {
            first_error = BAURA_DRIVER_ISO_ERR_BACKEND;
        }
        if cleanup_new_task {
            terminate_backend_task(backend, new_task_id, new_address_space_id);
        }
    }
    first_error
}

#[no_mangle]
pub extern "C" fn iso_stop_driver(driver_id: u32) -> i32 {
    if driver_id == 0 {
        return BAURA_DRIVER_ISO_ERR_ARGUMENT;
    }
    let backend = match current_backend() {
        Ok(backend) => backend,
        Err(error) => return error,
    };
    let (task_id, address_space_id, previous_status) = {
        let mut drivers = lock_recover(&DRIVER_REGISTRY);
        let Some(driver) = drivers
            .iter_mut()
            .find(|driver| driver.driver_id == driver_id)
        else {
            return BAURA_DRIVER_ISO_ERR_NOT_FOUND;
        };
        if driver.status == DRIVER_STOPPING || driver.status == DRIVER_RESTARTING {
            return BAURA_DRIVER_ISO_ERR_INVALID_STATE;
        }
        let previous_status = driver.status;
        driver.status = DRIVER_STOPPING;
        (driver.task_id, driver.address_space_id, previous_status)
    };

    let status = unsafe {
        (backend.terminate_task)(
            backend.context as *mut c_void,
            task_id,
            address_space_id,
        )
    };
    if status != 0 {
        let mut drivers = lock_recover(&DRIVER_REGISTRY);
        if let Some(driver) = drivers.iter_mut().find(|driver| driver.driver_id == driver_id) {
            driver.status = previous_status;
        }
        return BAURA_DRIVER_ISO_ERR_BACKEND;
    }
    lock_recover(&DRIVER_REGISTRY).retain(|driver| driver.driver_id != driver_id);
    BAURA_DRIVER_ISO_OK
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicU64, Ordering};

    static TEST_LOCK: Mutex<()> = Mutex::new(());
    static NEXT_TASK_ID: AtomicU64 = AtomicU64::new(100);
    static SPAWN_CALLS: AtomicU64 = AtomicU64::new(0);
    static TERMINATE_CALLS: AtomicU64 = AtomicU64::new(0);

    unsafe extern "C" fn mock_spawn(
        _context: *mut c_void,
        _driver_id: u32,
        _driver_type: *const c_char,
        task_id: *mut u64,
        address_space_id: *mut u64,
    ) -> i32 {
        SPAWN_CALLS.fetch_add(1, Ordering::SeqCst);
        let next = NEXT_TASK_ID.fetch_add(1, Ordering::SeqCst);
        *task_id = next;
        *address_space_id = next + 1000;
        0
    }

    unsafe extern "C" fn mock_alive(
        _context: *mut c_void,
        _task_id: u64,
        alive: *mut u8,
    ) -> i32 {
        *alive = 0;
        0
    }

    unsafe extern "C" fn mock_terminate(
        _context: *mut c_void,
        _task_id: u64,
        _address_space_id: u64,
    ) -> i32 {
        TERMINATE_CALLS.fetch_add(1, Ordering::SeqCst);
        0
    }

    #[test]
    fn isolation_calls_require_an_installed_platform_backend() {
        let _test_guard = lock_recover(&TEST_LOCK);
        lock_recover(&DRIVER_REGISTRY).clear();
        assert_eq!(
            unsafe { iso_set_platform_backend(std::ptr::null()) },
            BAURA_DRIVER_ISO_OK
        );
        let driver_type = std::ffi::CString::new("storage").unwrap();
        assert_eq!(
            unsafe { iso_spawn_driver(1, driver_type.as_ptr()) },
            BAURA_DRIVER_ISO_ERR_UNAVAILABLE
        );
        assert_eq!(
            iso_monitor_drivers(),
            BAURA_DRIVER_ISO_ERR_UNAVAILABLE
        );
    }

    #[test]
    fn backend_spawn_monitor_restart_and_stop_use_real_callbacks() {
        let _test_guard = lock_recover(&TEST_LOCK);
        lock_recover(&DRIVER_REGISTRY).clear();
        SPAWN_CALLS.store(0, Ordering::SeqCst);
        TERMINATE_CALLS.store(0, Ordering::SeqCst);

        let backend = BauraDriverIsolationBackend {
            abi_version: BAURA_DRIVER_ISO_ABI_VERSION,
            struct_size: size_of::<BauraDriverIsolationBackend>() as u32,
            context: std::ptr::null_mut(),
            spawn_driver: Some(mock_spawn),
            is_task_alive: Some(mock_alive),
            terminate_task: Some(mock_terminate),
        };
        assert_eq!(
            unsafe { iso_set_platform_backend(&backend) },
            BAURA_DRIVER_ISO_OK
        );
        let driver_type = std::ffi::CString::new("network").unwrap();
        assert_eq!(
            unsafe { iso_spawn_driver(27, driver_type.as_ptr()) },
            BAURA_DRIVER_ISO_OK
        );
        assert_eq!(
            unsafe { iso_spawn_driver(27, driver_type.as_ptr()) },
            BAURA_DRIVER_ISO_ERR_ALREADY_EXISTS
        );
        assert_eq!(iso_monitor_drivers(), BAURA_DRIVER_ISO_OK);
        assert_eq!(SPAWN_CALLS.load(Ordering::SeqCst), 2);
        assert_eq!(TERMINATE_CALLS.load(Ordering::SeqCst), 1);
        assert_eq!(iso_stop_driver(27), BAURA_DRIVER_ISO_OK);
        assert_eq!(TERMINATE_CALLS.load(Ordering::SeqCst), 2);
        assert_eq!(
            iso_stop_driver(27),
            BAURA_DRIVER_ISO_ERR_NOT_FOUND
        );
        assert_eq!(
            unsafe { iso_set_platform_backend(std::ptr::null()) },
            BAURA_DRIVER_ISO_OK
        );

        lock_recover(&DRIVER_REGISTRY).clear();
        assert_eq!(
            unsafe { iso_set_platform_backend(std::ptr::null()) },
            BAURA_DRIVER_ISO_OK
        );
    }
}
