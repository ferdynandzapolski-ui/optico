use crate::grade::{Grade, GPtr};
use std::collections::HashMap;

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Trap {
    Bounds(String, Option<String>),
    Life(String, Option<String>),
    Perms(String, Option<String>),
    Prov(String, Option<String>),
    Alias(String, Option<String>),
}

pub type Result<T> = std::result::Result<T, Trap>;

pub struct Allocation {
    pub base: u64,
    pub size: u64,
    pub bytes: Vec<u8>,
    pub alloc_id: u32,
    pub epoch: u32,
    pub alive: bool,
}

pub struct Memory {
    pub heap: HashMap<u32, Allocation>, // alloc_id -> Allocation
    pub shadow: HashMap<u64, Grade>,    // addr -> Grade (for pointers stored in memory)
    pub next_alloc_id: u32,
    pub next_addr: u64,
}

impl Memory {
    pub fn new() -> Self {
        Memory {
            heap: HashMap::new(),
            shadow: HashMap::new(),
            next_alloc_id: 1,
            next_addr: 0x1000, // Start heap at 4KB
        }
    }

    pub fn alloc(&mut self, size: u64) -> GPtr {
        let alloc_id = self.next_alloc_id;
        self.next_alloc_id += 1;

        let base = self.next_addr;
        self.next_addr += size + 16; // Simple bump allocator with padding

        let allocation = Allocation {
            base,
            size,
            bytes: vec![0; size as usize],
            alloc_id,
            epoch: 1,
            alive: true,
        };

        self.heap.insert(alloc_id, allocation);

        let grade = Grade {
            base,
            end: base + size,
            alloc_id,
            epoch: 1,
            perms: Grade::PERM_R | Grade::PERM_W | Grade::PERM_F,
            prov_tag: 0,
            alias_tok: 0,
            flags: 1, // GO_BOUNDS_KIND_OBJECT
        };

        GPtr::new(base, grade)
    }

    pub fn free(&mut self, ptr: GPtr, site: Option<String>) -> Result<()> {
        let g = ptr.grade;

        // Check perms
        if g.perms & Grade::PERM_F == 0 {
            return Err(Trap::Perms("No free permission".into(), site));
        }

        // Check if p is base
        if ptr.addr != g.base {
            return Err(Trap::Bounds("Free of non-base address".into(), site));
        }

        // Check lifetime
        let alloc = self.heap.get_mut(&g.alloc_id).ok_or_else(|| Trap::Life("Invalid alloc_id".into(), site.clone()))?;
        if !alloc.alive {
            return Err(Trap::Life("Double free".into(), site));
        }
        if alloc.epoch != g.epoch {
            return Err(Trap::Life("Epoch mismatch".into(), site));
        }

        alloc.alive = false;
        Ok(())
    }

    pub fn load_byte(&self, ptr: GPtr, site: Option<String>) -> Result<u8> {
        let g = ptr.grade;
        let addr = ptr.addr;

        // Spatial check
        if addr < g.base || addr >= g.end {
            return Err(Trap::Bounds(format!("OOB load at {:x} for grade [{:x}, {:x})", addr, g.base, g.end), site));
        }

        // Temporal check
        let alloc = self.heap.get(&g.alloc_id).ok_or_else(|| Trap::Life("Invalid alloc_id".into(), site.clone()))?;
        if !alloc.alive {
            return Err(Trap::Life("Use-after-free (load)".into(), site));
        }
        if alloc.epoch != g.epoch {
            return Err(Trap::Life("Epoch mismatch (load)".into(), site));
        }

        // Access check
        if g.perms & Grade::PERM_R == 0 {
            return Err(Trap::Perms("No read permission".into(), site));
        }

        let offset = (addr - alloc.base) as usize;
        Ok(alloc.bytes[offset])
    }

    pub fn store_byte(&mut self, ptr: GPtr, val: u8, site: Option<String>) -> Result<()> {
        let g = ptr.grade;
        let addr = ptr.addr;

        // Spatial check
        if addr < g.base || addr >= g.end {
            return Err(Trap::Bounds(format!("OOB store at {:x} for grade [{:x}, {:x})", addr, g.base, g.end), site));
        }

        // Temporal check
        let alloc = self.heap.get(&g.alloc_id).ok_or_else(|| Trap::Life("Invalid alloc_id".into(), site.clone()))?;
        if !alloc.alive {
            return Err(Trap::Life("Use-after-free (store)".into(), site));
        }
        if alloc.epoch != g.epoch {
            return Err(Trap::Life("Epoch mismatch (store)".into(), site));
        }

        // Access check
        if g.perms & Grade::PERM_W == 0 {
            return Err(Trap::Perms("No write permission".into(), site));
        }

        let base = alloc.base;
        let offset = (addr - base) as usize;
        let alloc_mut = self.heap.get_mut(&g.alloc_id).unwrap();
        alloc_mut.bytes[offset] = val;

        // Writing a byte taints shadow if it was a pointer
        self.shadow.remove(&addr);

        Ok(())
    }

    pub fn store_ptr(&mut self, ptr: GPtr, val: GPtr, site: Option<String>) -> Result<()> {
        // Pointers are 8 bytes in this model
        for i in 0..8 {
            let byte = ((val.addr >> (i * 8)) & 0xFF) as u8;
            self.store_byte(GPtr::new(ptr.addr + i, ptr.grade), byte, site.clone())?;
        }
        self.shadow.insert(ptr.addr, val.grade);
        Ok(())
    }

    pub fn load_ptr(&self, ptr: GPtr, site: Option<String>) -> Result<GPtr> {
        let mut addr: u64 = 0;
        for i in 0..8 {
            let byte = self.load_byte(GPtr::new(ptr.addr + i, ptr.grade), site.clone())?;
            addr |= (byte as u64) << (i * 8);
        }
        let grade = self.shadow.get(&ptr.addr).cloned().unwrap_or(Grade::top());
        Ok(GPtr::new(addr, grade))
    }
}
