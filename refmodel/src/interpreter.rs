use crate::grade::{Grade, GPtr};
use crate::mem::{Memory, Result};

pub enum ProvPolicy {
    None,
    PnviAe,
}

pub struct Interpreter {
    pub mem: Memory,
    pub prov_policy: ProvPolicy,
    pub exposed: Vec<u32>, // Exposed alloc_ids
}

impl Interpreter {
    pub fn new() -> Self {
        Interpreter {
            mem: Memory::new(),
            prov_policy: ProvPolicy::PnviAe,
            exposed: Vec::new(),
        }
    }

    pub fn galloc(&mut self, size: u64) -> GPtr {
        self.mem.alloc(size)
    }

    pub fn gfree(&mut self, ptr: GPtr, site: Option<String>) -> Result<()> {
        self.mem.free(ptr, site)
    }

    pub fn gload(&self, ptr: GPtr, size: u64, site: Option<String>) -> Result<Vec<u8>> {
        let mut bytes = Vec::new();
        for i in 0..size {
            bytes.push(self.mem.load_byte(GPtr::new(ptr.addr + i, ptr.grade), site.clone())?);
        }
        Ok(bytes)
    }

    pub fn gstore(&mut self, ptr: GPtr, bytes: &[u8], site: Option<String>) -> Result<()> {
        for (i, &byte) in bytes.iter().enumerate() {
            self.mem.store_byte(GPtr::new(ptr.addr + i as u64, ptr.grade), byte, site.clone())?;
        }
        Ok(())
    }

    pub fn ggep(&self, ptr: GPtr, offset: i64) -> GPtr {
        let mut g = ptr.grade;
        let new_addr = (ptr.addr as i64 + offset) as u64;

        // In GOIR object-mode, bounds don't change on GEP
        // But we might want to flag it as subobject if we had that flag
        g.flags |= 2; // GO_BOUNDS_KIND_SUBOBJECT

        GPtr::new(new_addr, g)
    }

    pub fn gmemcpy(&mut self, dst: GPtr, src: GPtr, len: u64, site: Option<String>) -> Result<()> {
        // Diagnostic tier: byte-wise copy
        for i in 0..len {
            let b = self.mem.load_byte(GPtr::new(src.addr + i, src.grade), site.clone())?;
            self.mem.store_byte(GPtr::new(dst.addr + i, dst.grade), b, site.clone())?;

            // Shadow metadata propagation logic
            // A pointer copy happens if a source address contains a pointer
            // In our model, pointers are 8-byte aligned slots in shadow map.
        }

        // Correct shadow propagation: check all shadow entries that overlap with [src, src+len)
        let mut to_copy = Vec::new();
        for (&addr, &grade) in &self.mem.shadow {
            if addr >= src.addr && addr + 8 <= src.addr + len {
                to_copy.push((addr - src.addr, grade));
            }
        }

        // Taint destination shadow in the range [dst, dst+len)
        // Any pointer in shadow that overlaps with the destination range must be removed.
        let dst_start = dst.addr;
        let dst_end = dst.addr + len;
        self.mem.shadow.retain(|&addr, _| {
            let ptr_start = addr;
            let ptr_end = addr + 8;
            // Keep if no overlap: ptr_end <= dst_start OR ptr_start >= dst_end
            ptr_end <= dst_start || ptr_start >= dst_end
        });

        // Apply copied shadow entries
        for (offset, grade) in to_copy {
            self.mem.shadow.insert(dst.addr + offset, grade);
        }

        Ok(())
    }

    pub fn gptrtoint(&mut self, ptr: GPtr) -> u64 {
        match self.prov_policy {
            ProvPolicy::PnviAe => {
                if !self.exposed.contains(&ptr.grade.alloc_id) {
                    self.exposed.push(ptr.grade.alloc_id);
                }
            }
            ProvPolicy::None => {}
        }
        ptr.addr
    }

    pub fn ginttoptr(&self, addr: u64) -> GPtr {
        match self.prov_policy {
            ProvPolicy::PnviAe => {
                // Find exposed allocation that contains this address
                for (&alloc_id, alloc) in &self.mem.heap {
                    if self.exposed.contains(&alloc_id) && addr >= alloc.base && addr < alloc.base + alloc.size {
                        let grade = Grade {
                            base: alloc.base,
                            end: alloc.base + alloc.size,
                            alloc_id,
                            epoch: alloc.epoch,
                            perms: Grade::PERM_R | Grade::PERM_W | Grade::PERM_F,
                            prov_tag: 0,
                            alias_tok: 0,
                            flags: 1,
                        };
                        return GPtr::new(addr, grade);
                    }
                }
            }
            ProvPolicy::None => {}
        }
        GPtr::new(addr, Grade::top())
    }
}
