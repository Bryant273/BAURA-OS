use std::ffi::CStr;
use std::os::raw::{c_char, c_int};

const MAX_PATH: usize = 256;
const OK: c_int = 0;
const INVALID_ARGUMENT: c_int = -1;
const INVALID_PATH: c_int = -2;
const VOLUME_NOT_FOUND: c_int = -3;
const PATH_TOO_LONG: c_int = -4;

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct BauraPathInfo {
    pub volume_id: u32,
    pub path_len: u32,
    pub path: [c_char; MAX_PATH],
}

fn volume_id(name: &str) -> Option<u32> {
    match name {
        "C" => Some(1),
        "D" => Some(2),
        "USB" => Some(3),
        _ => None,
    }
}

fn resolve_path(input: &str) -> Result<BauraPathInfo, c_int> {
    let remainder = input.strip_prefix("PC/").ok_or(INVALID_PATH)?;
    let (volume, suffix) = remainder.split_once(':').ok_or(INVALID_PATH)?;
    if volume.is_empty() || volume.len() > 7
        || !volume.bytes().all(|b| b.is_ascii_alphanumeric() || b == b'_') {
        return Err(INVALID_PATH);
    }
    let volume_id = volume_id(volume).ok_or(VOLUME_NOT_FOUND)?;
    if !suffix.starts_with('/') || suffix.contains('\\') { return Err(INVALID_PATH); }

    let mut normalized = String::new();
    for component in suffix[1..].split('/') {
        if component.is_empty() || component == "." { continue; }
        if component == ".." || component.chars().any(|c| c.is_control() || c == ':') {
            return Err(INVALID_PATH);
        }
        let separator_len = if normalized.is_empty() { 0 } else { 1 };
        if normalized.len() + separator_len + component.len() >= MAX_PATH {
            return Err(PATH_TOO_LONG);
        }
        if separator_len != 0 { normalized.push('/'); }
        normalized.push_str(component);
    }

    let mut path = [0 as c_char; MAX_PATH];
    for (destination, source) in path.iter_mut().zip(normalized.bytes()) {
        *destination = source as c_char;
    }
    Ok(BauraPathInfo { volume_id, path_len: normalized.len() as u32, path })
}

#[no_mangle]
pub unsafe extern "C" fn baura_vfs_resolve_path_rust(
    baura_path: *const c_char, info: *mut BauraPathInfo,
) -> c_int {
    if baura_path.is_null() || info.is_null() { return INVALID_ARGUMENT; }
    let input = unsafe { CStr::from_ptr(baura_path) };
    let input = match input.to_str() { Ok(value) => value, Err(_) => return INVALID_PATH };
    let resolved = match resolve_path(input) { Ok(value) => value, Err(error) => return error };
    unsafe { info.write(resolved) };
    OK
}

#[cfg(test)]
mod tests {
    use super::{resolve_path, BauraPathInfo, INVALID_PATH, PATH_TOO_LONG, VOLUME_NOT_FOUND};
    use std::ffi::CStr;

    fn path_string(info: &BauraPathInfo) -> &str {
        unsafe { CStr::from_ptr(info.path.as_ptr()) }.to_str().unwrap()
    }

    #[test]
    fn resolves_known_volume_roots() {
        for (path, volume) in [("PC/C:/", 1), ("PC/D:/", 2), ("PC/USB:/", 3)] {
            let info = resolve_path(path).unwrap();
            assert_eq!(info.volume_id, volume);
            assert_eq!(info.path_len, 0);
            assert_eq!(path_string(&info), "");
        }
    }

    #[test]
    fn normalizes_separators_and_dot_segments() {
        let info = resolve_path("PC/C://Users/./Ahmed//Documents/").unwrap();
        assert_eq!(path_string(&info), "Users/Ahmed/Documents");
        assert_eq!(info.path_len as usize, "Users/Ahmed/Documents".len());
    }

    #[test]
    fn rejects_unknown_volumes_and_traversal() {
        assert_eq!(resolve_path("PC/Z:/file"), Err(VOLUME_NOT_FOUND));
        assert_eq!(resolve_path("PC/C:/Users/../secret"), Err(INVALID_PATH));
        assert_eq!(resolve_path("PC/C:\\Users\\Ahmed"), Err(INVALID_PATH));
    }

    #[test]
    fn enforces_path_capacity() {
        let input = format!("PC/C:/{}", "a".repeat(256));
        assert_eq!(resolve_path(&input), Err(PATH_TOO_LONG));
    }

    #[test]
    fn rejects_noncanonical_paths() {
        assert_eq!(resolve_path("C:/Users/Ahmed"), Err(INVALID_PATH));
        assert_eq!(resolve_path("PC/C:Users/Ahmed"), Err(INVALID_PATH));
    }
}
