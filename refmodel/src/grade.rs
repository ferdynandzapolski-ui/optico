use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum BoundsKind {
    Unknown = 0,
    Object = 1,
    SubObject = 2,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub struct Grade {
    pub base: u64,
    pub end: u64,
    pub alloc_id: u32,
    pub epoch: u32,
    pub perms: u32,
    pub prov_tag: u32,
    pub alias_tok: u64,
    pub flags: u32,
}

impl Grade {
    pub const PERM_R: u32 = 1;
    pub const PERM_W: u32 = 2;
    pub const PERM_F: u32 = 4;
    pub const PERM_X: u32 = 8;

    pub fn top() -> Self {
        Grade {
            base: 0,
            end: !0,
            alloc_id: 0,
            epoch: 0,
            perms: 0xF,
            prov_tag: 0,
            alias_tok: 0,
            flags: 0,
        }
    }

    pub fn is_top(&self) -> bool {
        self.base == 0 && self.end == !0 && self.perms == 0xF
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub struct GPtr {
    pub addr: u64,
    pub grade: Grade,
}

impl GPtr {
    pub fn new(addr: u64, grade: Grade) -> Self {
        GPtr { addr, grade }
    }

    pub fn null() -> Self {
        GPtr::new(0, Grade::top())
    }
}
