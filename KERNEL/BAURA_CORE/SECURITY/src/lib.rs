use std::os::raw::c_int;
use std::sync::Mutex;

pub const CAP_READ: u32 = 1 << 0;
pub const CAP_WRITE: u32 = 1 << 1;
pub const CAP_EXECUTE: u32 = 1 << 2;
pub const CAP_GRANT: u32 = 1 << 3;
const VALID_RIGHTS: u32 = CAP_READ | CAP_WRITE | CAP_EXECUTE | CAP_GRANT;
const MAX_CSPACES: usize = 64;
const MAX_SLOTS: usize = 64;

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Capability { pub object_id: u64, pub rights: u32 }

#[derive(Clone, Copy)]
struct CapabilitySlot { slot: u32, capability: Capability, active: bool }
impl CapabilitySlot { const EMPTY: Self = Self { slot: 0, capability: Capability { object_id: 0, rights: 0 }, active: false }; }

#[derive(Clone, Copy)]
struct CSpace { task_id: u64, slots: [CapabilitySlot; MAX_SLOTS], active: bool }
impl CSpace { const EMPTY: Self = Self { task_id: 0, slots: [CapabilitySlot::EMPTY; MAX_SLOTS], active: false }; }

struct CSpaceTable { spaces: [CSpace; MAX_CSPACES] }
static GLOBAL_CSPACES: Mutex<CSpaceTable> = Mutex::new(CSpaceTable { spaces: [CSpace::EMPTY; MAX_CSPACES] });

fn find_space(spaces: &[CSpace], task_id: u64) -> Option<usize> {
    spaces.iter().position(|space| space.active && space.task_id == task_id)
}

#[no_mangle]
pub extern "C" fn sec_create_cspace(task_id: u64) -> c_int {
    if task_id == 0 { return -5; }
    let mut table = GLOBAL_CSPACES.lock().unwrap();
    if find_space(&table.spaces, task_id).is_some() { return -7; }
    let Some(space) = table.spaces.iter_mut().find(|space| !space.active) else { return -4; };
    *space = CSpace { task_id, slots: [CapabilitySlot::EMPTY; MAX_SLOTS], active: true };
    0
}

#[no_mangle]
pub extern "C" fn sec_destroy_cspace(task_id: u64) -> c_int {
    let mut table = GLOBAL_CSPACES.lock().unwrap();
    let Some(index) = find_space(&table.spaces, task_id) else { return -1; };
    table.spaces[index] = CSpace::EMPTY;
    0
}

#[no_mangle]
pub extern "C" fn sec_mint_capability(task_id: u64, slot: u32, object_id: u64, rights: u32) -> c_int {
    if object_id == 0 || rights == 0 || rights & !VALID_RIGHTS != 0 { return -5; }
    let mut table = GLOBAL_CSPACES.lock().unwrap();
    let Some(space_index) = find_space(&table.spaces, task_id) else { return -1; };
    if let Some(existing) = table.spaces[space_index].slots.iter().position(|entry| entry.active && entry.slot == slot) {
        table.spaces[space_index].slots[existing].capability = Capability { object_id, rights };
        return 0;
    }
    let Some(empty) = table.spaces[space_index].slots.iter().position(|entry| !entry.active) else { return -4; };
    table.spaces[space_index].slots[empty] = CapabilitySlot { slot, capability: Capability { object_id, rights }, active: true };
    0
}

#[no_mangle]
pub extern "C" fn sec_validate_capability(task_id: u64, slot: u32, required_right: u32) -> c_int {
    if required_right == 0 || required_right & !VALID_RIGHTS != 0 { return 0; }
    let table = GLOBAL_CSPACES.lock().unwrap();
    let Some(space_index) = find_space(&table.spaces, task_id) else { return 0; };
    match table.spaces[space_index].slots.iter().find(|entry| entry.active && entry.slot == slot) {
        Some(entry) if entry.capability.rights & required_right == required_right => 1,
        _ => 0,
    }
}

#[no_mangle]
pub extern "C" fn sec_revoke_capability(task_id: u64, slot: u32) -> c_int {
    let mut table = GLOBAL_CSPACES.lock().unwrap();
    let Some(space_index) = find_space(&table.spaces, task_id) else { return -1; };
    let Some(entry) = table.spaces[space_index].slots.iter_mut().find(|entry| entry.active && entry.slot == slot) else { return -2; };
    *entry = CapabilitySlot::EMPTY;
    0
}

#[no_mangle]
pub extern "C" fn sec_delegate_capability(src_task: u64, src_slot: u32, dst_task: u64, dst_slot: u32, requested_rights: u32) -> c_int {
    if requested_rights == 0 || requested_rights & !VALID_RIGHTS != 0 { return -5; }
    let mut table = GLOBAL_CSPACES.lock().unwrap();
    let Some(src_index) = find_space(&table.spaces, src_task) else { return -1; };
    let Some(source) = table.spaces[src_index].slots.iter().find(|entry| entry.active && entry.slot == src_slot).map(|entry| entry.capability) else { return -2; };
    if source.rights & CAP_GRANT == 0 { return -3; }
    if requested_rights & !source.rights != 0 { return -6; }
    let Some(dst_index) = find_space(&table.spaces, dst_task) else { return -1; };
    if table.spaces[dst_index].slots.iter().any(|entry| entry.active && entry.slot == dst_slot) { return -7; }
    let Some(destination) = table.spaces[dst_index].slots.iter_mut().find(|entry| !entry.active) else { return -4; };
    *destination = CapabilitySlot { slot: dst_slot, capability: Capability { object_id: source.object_id, rights: requested_rights }, active: true };
    0
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn delegation_only_attenuates_and_revocation_works() {
        assert_eq!(sec_create_cspace(9001), 0);
        assert_eq!(sec_create_cspace(9002), 0);
        assert_eq!(sec_mint_capability(9001, 1, 42, CAP_READ | CAP_WRITE | CAP_GRANT), 0);
        assert_eq!(sec_delegate_capability(9001, 1, 9002, 2, CAP_READ), 0);
        assert_eq!(sec_validate_capability(9002, 2, CAP_READ), 1);
        assert_eq!(sec_validate_capability(9002, 2, CAP_WRITE), 0);
        assert_eq!(sec_delegate_capability(9001, 1, 9002, 3, CAP_EXECUTE), -6);
        assert_eq!(sec_revoke_capability(9002, 2), 0);
        assert_eq!(sec_validate_capability(9002, 2, CAP_READ), 0);
        assert_eq!(sec_destroy_cspace(9001), 0);
        assert_eq!(sec_destroy_cspace(9002), 0);
    }
}
