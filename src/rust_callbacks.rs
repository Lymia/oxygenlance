use std::ffi::{c_char, CStr};

#[no_mangle]
pub unsafe extern "C-unwind" fn oxygenlance_die(str: *const c_char) -> ! {
    let str = CStr::from_ptr(str);
    let r_str = str.to_string_lossy();
    panic!("Fatal error from Chainlance: {r_str}");
}
