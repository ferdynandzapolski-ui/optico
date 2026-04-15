use goir_refmodel::*;

#[test]
fn test_basic_alloc_load_store() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(4);
    interp.gstore(p, &[1, 2, 3, 4], None).expect("store failed");
    let val = interp.gload(p, 4, None).expect("load failed");
    assert_eq!(val, vec![1, 2, 3, 4]);
}

#[test]
fn test_oob_load() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(4);
    let p_oob = interp.ggep(p, 4);
    let res = interp.gload(p_oob, 1, Some("site1".into()));
    assert!(matches!(res, Err(Trap::Bounds(_, Some(s))) if s == "site1"));
}

#[test]
fn test_oob_store() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(4);
    let p_oob = interp.ggep(p, 4);
    let res = interp.gstore(p_oob, &[1], Some("site2".into()));
    assert!(matches!(res, Err(Trap::Bounds(_, Some(s))) if s == "site2"));
}

#[test]
fn test_uaf_load() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(4);
    interp.gfree(p, None).expect("free failed");
    let res = interp.gload(p, 1, None);
    assert!(matches!(res, Err(Trap::Life(_, _))));
}

#[test]
fn test_double_free() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(4);
    interp.gfree(p, None).expect("first free failed");
    let res = interp.gfree(p, None);
    assert!(matches!(res, Err(Trap::Life(_, _))));
}

#[test]
fn test_memcpy_safe() {
    let mut interp = Interpreter::new();
    let p1 = interp.galloc(4);
    let p2 = interp.galloc(4);
    interp.gstore(p1, &[10, 20, 30, 40], None).unwrap();
    interp.gmemcpy(p2, p1, 4, None).unwrap();
    assert_eq!(interp.gload(p2, 4, None).unwrap(), vec![10, 20, 30, 40]);
}

#[test]
fn test_memcpy_oob() {
    let mut interp = Interpreter::new();
    let p1 = interp.galloc(4);
    let p2 = interp.galloc(2);
    interp.gstore(p1, &[1, 2, 3, 4], None).unwrap();
    let res = interp.gmemcpy(p2, p1, 4, None);
    assert!(matches!(res, Err(Trap::Bounds(_, _))));
}

#[test]
fn test_ptr_store_load() {
    let mut interp = Interpreter::new();
    let p_storage = interp.galloc(8);
    let p_val = interp.galloc(4);
    interp.mem.store_ptr(p_storage, p_val, None).unwrap();
    let p_loaded = interp.mem.load_ptr(p_storage, None).unwrap();
    assert_eq!(p_loaded.addr, p_val.addr);
    assert_eq!(p_loaded.grade, p_val.grade);
}

#[test]
fn test_pnvi_ae_roundtrip() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(4);
    let addr = interp.gptrtoint(p);
    let p2 = interp.ginttoptr(addr);
    assert_eq!(p2.grade.alloc_id, p.grade.alloc_id);
    interp.gload(p2, 1, None).expect("Roundtrip load failed");
}

#[test]
fn test_pnvi_ae_no_exposure() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(4);
    let addr = p.addr; // Manually get addr without gptrtoint exposure
    let p2 = interp.ginttoptr(addr);
    assert!(p2.grade.is_top());
}

#[test]
fn test_ggep_interior() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(16);
    let p_int = interp.ggep(p, 4);
    interp.gstore(p_int, &[42], None).unwrap();
    assert_eq!(interp.gload(p_int, 1, None).unwrap(), vec![42]);
}

#[test]
fn test_free_non_base() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(16);
    let p_int = interp.ggep(p, 4);
    let res = interp.gfree(p_int, None);
    assert!(matches!(res, Err(Trap::Bounds(_, _))));
}

#[test]
fn test_perms_read_only() {
    let mut interp = Interpreter::new();
    let mut p = interp.galloc(4);
    p.grade.perms = Grade::PERM_R;
    interp.gload(p, 1, None).unwrap();
    let res = interp.gstore(p, &[1], None);
    assert!(matches!(res, Err(Trap::Perms(_, _))));
}

#[test]
fn test_perms_write_only() {
    let mut interp = Interpreter::new();
    let mut p = interp.galloc(4);
    p.grade.perms = Grade::PERM_W;
    interp.gstore(p, &[1], None).unwrap();
    let res = interp.gload(p, 1, None);
    assert!(matches!(res, Err(Trap::Perms(_, _))));
}

#[test]
fn test_memcpy_shadow() {
    let mut interp = Interpreter::new();
    let src = interp.galloc(8);
    let dst = interp.galloc(8);
    let ptr_to_copy = interp.galloc(4);
    interp.mem.store_ptr(src, ptr_to_copy, None).unwrap();
    interp.gmemcpy(dst, src, 8, None).unwrap();
    let ptr_loaded = interp.mem.load_ptr(dst, None).unwrap();
    assert_eq!(ptr_loaded, ptr_to_copy);
}

#[test]
fn test_taint_shadow() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(8);
    let ptr_val = interp.galloc(4);
    interp.mem.store_ptr(p, ptr_val, None).unwrap();
    interp.gstore(p, &[0], None).unwrap(); // Overwrite first byte
    let ptr_loaded = interp.mem.load_ptr(p, None).unwrap();
    assert!(ptr_loaded.grade.is_top());
}

#[test]
fn test_one_past_end_ggep() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(4);
    let p_end = interp.ggep(p, 4);
    // GEP to one-past-end is allowed in C, but load is not.
    let res = interp.gload(p_end, 1, None);
    assert!(matches!(res, Err(Trap::Bounds(_, _))));
}

#[test]
fn test_negative_ggep_oob() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(4);
    let p_neg = interp.ggep(p, -1);
    let res = interp.gload(p_neg, 1, None);
    assert!(matches!(res, Err(Trap::Bounds(_, _))));
}

#[test]
fn test_zero_size_alloc() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(0);
    let res = interp.gload(p, 1, None);
    assert!(matches!(res, Err(Trap::Bounds(_, _))));
    interp.gfree(p, None).expect("Free zero-size alloc failed");
}

#[test]
fn test_large_alloc() {
    let mut interp = Interpreter::new();
    let size = 1024 * 1024;
    let p = interp.galloc(size);
    interp.gstore(interp.ggep(p, (size - 1) as i64), &[0xFF], None).unwrap();
    assert_eq!(interp.gload(interp.ggep(p, (size - 1) as i64), 1, None).unwrap(), vec![0xFF]);
}

#[test]
fn test_memcpy_unaligned_shadow() {
    let mut interp = Interpreter::new();
    let src = interp.galloc(16);
    let dst = interp.galloc(16);
    let p_val = interp.galloc(4);

    // Store pointer at offset 4 (unaligned if base is 8-byte aligned)
    let src_p = interp.ggep(src, 4);
    let dst_p = interp.ggep(dst, 4);
    interp.mem.store_ptr(src_p, p_val, None).unwrap();

    // Copy the whole range
    interp.gmemcpy(dst, src, 16, None).unwrap();

    // Verify pointer was copied at the correct relative offset
    let p_loaded = interp.mem.load_ptr(dst_p, None).unwrap();
    assert_eq!(p_loaded, p_val);
}

#[test]
fn test_policy_switch() {
    let mut interp = Interpreter::new();
    let p = interp.galloc(4);
    let addr = p.addr;

    // Default policy PnviAe returns TOP if not exposed
    assert!(interp.ginttoptr(addr).grade.is_top());

    // Switch to None (conservative) - always returns TOP
    interp.prov_policy = ProvPolicy::None;
    interp.exposed.clear();
    interp.gptrtoint(p); // Even if "exposed"
    assert!(interp.ginttoptr(addr).grade.is_top());

    // Switch back to PnviAe
    interp.prov_policy = ProvPolicy::PnviAe;
    interp.exposed.clear();
    interp.gptrtoint(p); // Expose it
    let p2 = interp.ginttoptr(addr);
    assert_eq!(p2.grade.alloc_id, p.grade.alloc_id);
}
