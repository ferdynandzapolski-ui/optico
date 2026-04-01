use std::collections::HashMap;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Durability {
    Volatile,
    Normal,
    Durable,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Fingerprint(pub [u8; 16]); // 128-bit hash

pub struct StorageBackend {
    pub disk_simulation: Vec<u8>, // Mock "Memory-Mapped" Disk
    pub index: HashMap<Fingerprint, (usize, usize)>, // Fingerprint -> (offset, length)
    pub lru_cache: Vec<Fingerprint>,
    pub capacity: usize,
}

impl StorageBackend {
    pub fn new(capacity: usize) -> Self {
        Self {
            disk_simulation: Vec::new(),
            index: HashMap::new(),
            lru_cache: Vec::new(),
            capacity,
        }
    }

    pub fn store(&mut self, data: Vec<u8>) -> Fingerprint {
        // 128-bit hash using a more robust algorithm (mocking BLAKE3/CRC)
        let mut hash = [0u8; 16];
        for (i, &b) in data.iter().enumerate() {
            let offset = i % 16;
            hash[offset] = hash[offset].wrapping_add(b).wrapping_mul(31);
        }
        let fp = Fingerprint(hash);

        if !self.index.contains_key(&fp) {
            let offset = self.disk_simulation.len();
            let len = data.len();
            self.disk_simulation.extend_from_slice(&data);
            self.index.insert(fp, (offset, len));
        }

        self.promote(fp);
        fp
    }

    pub fn fetch(&mut self, fp: Fingerprint) -> Option<&[u8]> {
        if let Some(&(offset, len)) = self.index.get(&fp) {
            self.promote(fp);
            // Simulate Zero-Copy access by returning a slice of the "disk"
            Some(&self.disk_simulation[offset..offset+len])
        } else {
            None
        }
    }

    fn promote(&mut self, fp: Fingerprint) {
        if let Some(pos) = self.lru_cache.iter().position(|&x| x == fp) {
            self.lru_cache.remove(pos);
        }
        self.lru_cache.push(fp);
        if self.lru_cache.len() > self.capacity {
            self.lru_cache.remove(0);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ast::*;
    use crate::sema::*;
    use crate::parser::Parser;

    #[test]
    fn test_cas_lru_paging() {
        let mut storage = StorageBackend::new(2);
        let fp1 = storage.store(vec![1]);
        let fp2 = storage.store(vec![2]);
        let fp3 = storage.store(vec![3]);

        assert_eq!(storage.lru_cache.len(), 2);
        assert!(!storage.lru_cache.contains(&fp1));
        assert!(storage.lru_cache.contains(&fp2));
        assert!(storage.lru_cache.contains(&fp3));
    }

    #[test]
    fn test_incremental_early_cutoff() {
        let mut sema = Sema::new();
        let e1 = Expr::Var("a".to_string());
        let e2 = Expr::Var("b".to_string());
        let expr = Expr::Compose(Box::new(e1), Box::new(e2));

        let mut env = std::collections::HashMap::new();
        env.insert("a".to_string(), Type::Int);
        env.insert("b".to_string(), Type::Int);
        let mut consumed = std::collections::HashSet::new();
        let mut locals = std::collections::HashSet::new();

        // First run: should populate cache
        sema.check_expr(&expr, &mut env, &mut consumed, &mut locals);
        assert_eq!(sema.query_cache.len(), 1);
        let fp_first = sema.query_cache.values().next().unwrap().1;

        // Second run with same expression: should trigger early cutoff
        let ty_second = sema.check_expr(&expr, &mut env, &mut consumed, &mut locals);
        let fp_second = sema.query_cache.values().next().unwrap().1;
        assert_eq!(fp_first, fp_second);
        assert_eq!(ty_second, Type::Int);
    }

    #[test]
    fn test_parser_durability_prefixes() {
        let input = "
            resource Durable DB[Open] db = alloc<DB, Durable>(0);
            co Normal int* p = db;
        ";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        assert_eq!(prog.decls.len(), 2);

        if let Decl::Resource { ty, .. } = &prog.decls[0] {
            if let Type::Resource(_, _, _, dur) = ty {
                assert_eq!(dur, &Some(Durability::Durable));
            } else { panic!("Expected Resource type"); }
        } else { panic!("Expected Resource declaration"); }

        if let Decl::Global(_, ty, _) = &prog.decls[1] {
            if let Type::Co(_, dur, _) = ty {
                assert_eq!(dur, &Some(Durability::Normal));
            } else { panic!("Expected Co type"); }
        } else { panic!("Expected Global declaration"); }
    }

    #[test]
    fn test_ssfg_serialization_roundtrip() {
        let mut states = HashMap::new();
        states.insert("f1".to_string(), "Open".to_string());
        states.insert("f2".to_string(), "Closed".to_string());

        let mut durability = HashMap::new();
        durability.insert("f1".to_string(), Durability::Durable);
        durability.insert("f2".to_string(), Durability::Volatile);

        let checkpoint = SSFGCheckpoint {
            resource_states: states,
            resource_durability: durability,
        };

        let serialized = checkpoint.serialize();
        let deserialized = SSFGCheckpoint::deserialize(&serialized);

        assert_eq!(checkpoint.resource_states, deserialized.resource_states);
        assert_eq!(checkpoint.resource_durability, deserialized.resource_durability);
    }

    #[test]
    fn test_ssfg_persistence() {
        let mut sema = Sema::new();
        sema.resource_states.insert("f".to_string(), "Open".to_string());
        sema.resource_durability.insert("f".to_string(), Durability::Durable);

        let checkpoint = sema.checkpoint_ssfg();

        // Restart compiler
        let mut new_sema = Sema::new();
        new_sema.restore_ssfg(checkpoint);

        assert_eq!(new_sema.resource_states.get("f").unwrap(), "Open");
        assert_eq!(*new_sema.resource_durability.get("f").unwrap(), Durability::Durable);
    }

    #[test]
    fn test_durability_aware_paging() {
        let mut sema = Sema::new();
        let prog = Program {
            decls: vec![
                Decl::Resource {
                    name: "db".to_string(),
                    ty: Type::Resource(vec![], Some("Open".to_string()), None, Some(Durability::Durable)),
                    val: Expr::Alloc(Type::Int, vec![Expr::ConstInt(0)], Some(Durability::Durable)),
                },
                Decl::Func {
                    name: "query".to_string(),
                    params: vec![],
                    ret_type: Type::Int,
                    body: Expr::Var("db".to_string()),
                }
            ],
        };

        sema.check_program(&prog);

        // After check_program, "db" should have been "fetched" (promoted in LRU)
        assert_eq!(sema.storage.lru_cache.len(), 1);
    }
}
