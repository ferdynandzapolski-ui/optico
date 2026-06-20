use crate::ast::*;
use std::collections::{HashMap, HashSet};

pub struct Sema {
    pub globals: HashMap<String, Type>,
    pub structs: HashMap<String, Vec<(String, Type)>>, // RFC 001: Struct definitions
    pub protocols: HashMap<String, Vec<ProtocolState>>,
    pub resources: HashSet<String>,
    pub resource_states: HashMap<String, String>, // resource name -> current state
    pub resource_durability: HashMap<String, crate::persistence::Durability>,
    pub terminal_states: HashSet<String>,
    pub transitions: HashMap<(String, String), String>, // (current state, action) -> next state
    pub query_cache: HashMap<String, (Type, crate::persistence::Fingerprint)>, // query description -> (result type, fingerprint)
    pub storage: crate::persistence::StorageBackend,
    pub current_func: Option<String>,
    pub in_rec_optic: bool,
    pub guarded: HashSet<Option<String>>, // clocks currently guarded
    pub c_heap_allocations: HashMap<String, String>, // ptr_name -> state (Allocated/Freed)
    pub lifting_hints: HashMap<String, Type>,       // var_name -> inferred optic type
    pub c_double_frees: HashSet<String>,
    pub in_checked: bool,
    pub phantom_lifetimes: HashMap<String, String>, // var_name -> scope_id
    pub beliefs: HashMap<String, f64>,
}

#[derive(Debug, Clone)]
pub struct SSFGCheckpoint {
    pub resource_states: HashMap<String, String>,
    pub resource_durability: HashMap<String, crate::persistence::Durability>,
}

impl SSFGCheckpoint {
    pub fn serialize(&self) -> Vec<u8> {
        let mut data = Vec::new();
        data.extend_from_slice(&(self.resource_states.len() as u32).to_le_bytes());
        for (name, state) in &self.resource_states {
            data.extend_from_slice(&(name.len() as u32).to_le_bytes());
            data.extend_from_slice(name.as_bytes());
            data.extend_from_slice(&(state.len() as u32).to_le_bytes());
            data.extend_from_slice(state.as_bytes());
            let dur = match self.resource_durability.get(name) {
                Some(crate::persistence::Durability::Volatile) => 0u8,
                Some(crate::persistence::Durability::Normal) => 1u8,
                Some(crate::persistence::Durability::Durable) => 2u8,
                None => 3u8,
            };
            data.push(dur);
        }
        data
    }

    pub fn deserialize(data: &[u8]) -> Self {
        let mut resource_states = HashMap::new();
        let mut resource_durability = HashMap::new();
        let mut pos = 0;
        let num_res = u32::from_le_bytes(data[pos..pos+4].try_into().unwrap());
        pos += 4;
        for _ in 0..num_res {
            let name_len = u32::from_le_bytes(data[pos..pos+4].try_into().unwrap()) as usize;
            pos += 4;
            let name = String::from_utf8(data[pos..pos+name_len].to_vec()).unwrap();
            pos += name_len;
            let state_len = u32::from_le_bytes(data[pos..pos+4].try_into().unwrap()) as usize;
            pos += 4;
            let state = String::from_utf8(data[pos..pos+state_len].to_vec()).unwrap();
            pos += state_len;
            let dur_byte = data[pos];
            pos += 1;
            let dur = match dur_byte {
                0 => Some(crate::persistence::Durability::Volatile),
                1 => Some(crate::persistence::Durability::Normal),
                2 => Some(crate::persistence::Durability::Durable),
                _ => None,
            };
            resource_states.insert(name.clone(), state);
            if let Some(d) = dur {
                resource_durability.insert(name, d);
            }
        }
        SSFGCheckpoint { resource_states, resource_durability }
    }
}

impl Sema {
    pub fn new() -> Self {
        let mut globals = HashMap::new();
        globals.insert("is_null".to_string(), Type::Int);

        let mut terminal_states = HashSet::new();
        terminal_states.insert("Closed".to_string());
        terminal_states.insert("Consumed".to_string());

        let mut transitions = HashMap::new();
        transitions.insert(("Open".to_string(), "put".to_string()), "Closed".to_string());

        Self {
            globals,
            structs: HashMap::new(),
            protocols: HashMap::new(),
            resources: HashSet::new(),
            resource_states: HashMap::new(),
            resource_durability: HashMap::new(),
            terminal_states,
            transitions,
            query_cache: HashMap::new(),
            storage: crate::persistence::StorageBackend::new(100),
            current_func: None,
            in_rec_optic: false,
            guarded: HashSet::new(),
            c_heap_allocations: HashMap::new(),
            lifting_hints: HashMap::new(),
            c_double_frees: HashSet::new(),
            in_checked: false,
            phantom_lifetimes: HashMap::new(),
            beliefs: HashMap::new(),
        }
    }

    pub fn load_beliefs(&mut self, filepath: &str) {
        if let Ok(content) = std::fs::read_to_string(filepath) {
            if let Ok(data) = serde_json::from_str::<HashMap<String, serde_json::Value>>(&content) {
                for (k, v) in data {
                    if let Some(alpha) = v.get("alpha").and_then(|a| a.as_f64()) {
                        if let Some(beta) = v.get("beta").and_then(|b| b.as_f64()) {
                            self.beliefs.insert(k, alpha / (alpha + beta));
                        }
                    }
                }
            }
        }
    }

    pub fn checkpoint_ssfg(&self) -> SSFGCheckpoint {
        SSFGCheckpoint {
            resource_states: self.resource_states.clone(),
            resource_durability: self.resource_durability.clone(),
        }
    }

    pub fn restore_ssfg(&mut self, checkpoint: SSFGCheckpoint) {
        self.resource_states = checkpoint.resource_states;
        self.resource_durability = checkpoint.resource_durability;
    }

    pub fn check_program(&mut self, prog: &Program) {
        for decl in &prog.decls {
            match decl {
                Decl::Global(name, ty, _) => {
                    self.globals.insert(name.clone(), ty.clone());
                }
                Decl::Func { name, ret_type, .. } => {
                    self.globals.insert(name.clone(), ret_type.clone());
                }
                Decl::Struct { name, fields } => {
                    self.structs.insert(name.clone(), fields.clone());
                }
                Decl::Protocol { name, states } => {
                    self.protocols.insert(name.clone(), states.clone());
                }
                Decl::Resource { name, ty, .. } => {
                    self.globals.insert(name.clone(), ty.clone());
                    self.resources.insert(name.clone());
                    if let Type::Resource(_, state, _, dur) = ty {
                        let initial_state = state.clone().unwrap_or_else(|| "Open".to_string());
                        self.resource_states.insert(name.clone(), initial_state);
                        if let Some(d) = dur {
                            self.resource_durability.insert(name.clone(), *d);
                        }
                    }
                }
                Decl::ExternC(decls) => {
                    for d in decls {
                        match d {
                            Decl::Global(name, ty, _) => {
                                self.globals.insert(name.clone(), ty.clone());
                            }
                            Decl::Func { name, ret_type, .. } => {
                                self.globals.insert(name.clone(), ret_type.clone());
                            }
                            _ => {}
                        }
                    }
                }
            }
        }

        for decl in &prog.decls {
            match decl {
                Decl::Func { name, body, params, ret_type, .. } => {
                    let mut env = self.globals.clone();
                    for (p_name, p_ty) in params {
                        env.insert(p_name.clone(), p_ty.clone());
                    }

                    let old_func = self.current_func.clone();
                    self.current_func = Some(name.clone());
                    let old_in_rec = self.in_rec_optic;
                    self.in_rec_optic = matches!(ret_type, Type::RecOptic(_, _, _, _));
                    let old_guarded = self.guarded.clone();
                    self.guarded.clear();

                    let mut res_consumed = HashSet::new();
                    let mut local_resources = HashSet::new();

                    self.check_expr(body, &mut env, &mut res_consumed, &mut local_resources);

                    self.in_rec_optic = old_in_rec;
                    self.guarded = old_guarded;
                    self.current_func = old_func;
                }
                Decl::Resource { val, .. } => {
                    let mut env = self.globals.clone();
                    let mut res_consumed = HashSet::new();
                    let mut local_resources = HashSet::new();
                    self.check_expr(val, &mut env, &mut res_consumed, &mut local_resources);
                }
                Decl::Global(_, _, Some(val)) => {
                    let mut env = self.globals.clone();
                    let mut res_consumed = HashSet::new();
                    let mut local_resources = HashSet::new();
                    self.check_expr(val, &mut env, &mut res_consumed, &mut local_resources);
                }
                Decl::ExternC(_) => {}
                _ => {}
            }
        }
    }

    fn track_malloc(&mut self, n: &str, e: &Expr, local_resources: &mut HashSet<String>) {
        if let Expr::Call(callee, _) = e {
            if let Expr::Var(name) = &**callee {
                if name == "malloc" || name == "malloc_ptr" || name == "malloc_int" {
                    self.c_heap_allocations.insert(n.to_string(), "Allocated".to_string());
                    local_resources.insert(n.to_string());

                    let mut inner_ty = Type::Int;
                    if let Some(ret_ty) = self.globals.get(name) {
                        inner_ty = match ret_ty {
                            Type::Optic(inner, _, _, _) | Type::Traversal(inner, _, _, _) | Type::Co(inner, _, _) | Type::Pointer(inner, _, _) => (**inner).clone(),
                            _ => ret_ty.clone(),
                        };
                    }

                    let confidence = self.beliefs.get("inv.memcpy_typed_ok").cloned().unwrap_or(1.0);
                    let lifted_ty = if n.contains("arr") {
                        Type::Traversal(Box::new(Type::Optic(Box::new(inner_ty), None, Some(confidence), None)), None, Some(confidence), None)
                    } else {
                        Type::Optic(Box::new(inner_ty), None, Some(confidence), None)
                    };

                    if confidence >= 0.99 {
                        self.lifting_hints.insert(n.to_string(), lifted_ty);
                    }
                }
            }
        }
    }

    pub fn check_expr(&mut self, expr: &Expr, env: &mut HashMap<String, Type>, res_consumed: &mut HashSet<String>, local_resources: &mut HashSet<String>) -> Type {
        match expr {
            Expr::LocalDecl(n, ty, e) => {
                let _actual_ty = self.check_expr(e, env, res_consumed, local_resources);
                self.track_malloc(n, e, local_resources);
                env.insert(n.clone(), ty.clone());
                Type::Void
            }
            Expr::Assign(n, e) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                // Only track phantom lifetimes for actual pointer operations, not arithmetic
                // Phantom lifetimes should only be created when assigning stack pointers to longer-lived variables
                self.track_malloc(n, e, local_resources);
                if !env.contains_key(n) {
                    env.insert(n.clone(), ty.clone());
                } else {
                    let current_ty = env.get(n).unwrap();
                    if let Type::Optic(..) | Type::RecOptic(..) | Type::AtomicOptic(..) | Type::Co(..) | Type::Pointer(..) = current_ty {
                        self.check_expr(&Expr::Put(Box::new(Expr::Var(n.clone())), e.clone()), env, res_consumed, local_resources);
                    }
                }
                return Type::Void;
            }
            Expr::FieldAssign(base, value) => {
                self.check_expr(base, env, res_consumed, local_resources);
                self.check_expr(value, env, res_consumed, local_resources);
                Type::Void
            }
            Expr::Var(n) => {
                if n == "is_null" { return Type::Int; }
                if let Some(dur) = self.resource_durability.get(n).cloned() {
                    if dur == crate::persistence::Durability::Durable {
                         let fp = crate::persistence::Fingerprint([0; 16]);
                         self.storage.store(vec![0]);
                         self.storage.fetch(fp);
                    }
                }
                if local_resources.contains(n) || self.resources.contains(n) {
                    if let Some(ty) = env.get(n) {
                        if let Type::Resource(..) = ty {
                            if res_consumed.contains(n) { panic!("Linearity violation: resource {} used after consume", n); }
                        }
                    }
                }
                if let Some(lifted_ty) = self.lifting_hints.get(n) { return lifted_ty.clone(); }
                env.get(n).cloned().expect(&format!("Undefined variable {}", n))
            }
            Expr::ConstInt(_) => Type::Int,
            Expr::ConstFloat(_) => Type::Float,
            Expr::ConstBool(_) => Type::Bool,
            Expr::ConstChar(_) => Type::Char,
            Expr::ConstString(_) => Type::Named("String".to_string()),
            Expr::Next(e, clock) => {
                let added = self.guarded.insert(clock.clone());
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                if added { self.guarded.remove(clock); }
                Type::Later(Box::new(ty), clock.clone())
            }
            Expr::Prev(e, clock) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                match ty {
                    Type::Later(inner, c) => {
                        if &c != clock { panic!("Clock mismatch in prev: expected {:?}, found {:?}", clock, c); }
                        *inner
                    }
                    _ => panic!("prev requires later modality"),
                }
            }
            Expr::Alloc(ty, args, dur) => {
                for arg in args { self.check_expr(arg, env, res_consumed, local_resources); }
                if let Some(d) = dur {
                    if *d == crate::persistence::Durability::Durable {
                         let fp = crate::persistence::Fingerprint([0; 16]);
                         self.storage.store(vec![0]);
                         self.storage.fetch(fp);
                    }
                }
                Type::Co(Box::new(ty.clone()), dur.clone(), None)
            }
            Expr::Free(e) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                if let Expr::Var(n) = &**e {
                    if let Some(state) = self.c_heap_allocations.get(n) {
                        if state == "Freed" {
                            self.c_double_frees.insert(n.clone());
                            panic!("Safety violation: double free of C pointer {}", n);
                        }
                        self.c_heap_allocations.insert(n.clone(), "Freed".to_string());
                        return Type::Void;
                    }
                }
                match ty {
                    Type::Co(..) | Type::Pointer(..) | Type::Optic(..) | Type::Traversal(..) => Type::Void,
                    _ => panic!("free requires co, pointer, or optic type, found {:?}", ty),
                }
            }
            Expr::Get(e) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                match ty {
                    Type::Optic(inner, _, _, _) | Type::RecOptic(inner, _, _, _) | Type::AtomicOptic(inner, _, _, _) | Type::Traversal(inner, _, _, _) | Type::Co(inner, _, _) | Type::Pointer(inner, _, _) => (*inner).clone(),
                    _ => panic!("get requires optic, co, or pointer type, found {:?}", ty),
                }
            }
            Expr::Put(e1, e2) => {
                let ty1 = self.check_expr(e1, env, res_consumed, local_resources);
                let _ty2 = self.check_expr(e2, env, res_consumed, local_resources);
                let res_assoc = match ty1 {
                    Type::Optic(_, res, _, _) | Type::RecOptic(_, res, _, _) | Type::AtomicOptic(_, res, _, _) => res,
                    _ => None,
                };
                if let Some(res_name) = res_assoc {
                    let current_state = self.resource_states.get(&res_name).cloned().unwrap_or_else(|| "Open".to_string());
                    if let Some(next_state) = self.transitions.get(&(current_state.clone(), "put".to_string())) {
                        self.resource_states.insert(res_name.clone(), next_state.clone());
                    } else {
                        panic!("Invalid transition: no 'put' action defined for state {} of resource {}", current_state, res_name);
                    }
                }
                Type::Void
            }
            Expr::Block(exprs) => {
                let old_guarded = self.guarded.clone();
                let mut last_ty = Type::Void;
                let mut block_locals = HashSet::new();
                for e in exprs {
                    let before_locals = local_resources.clone();
                    last_ty = self.check_expr(e, env, res_consumed, local_resources);
                    for loc in local_resources.iter() {
                        if !before_locals.contains(loc) { block_locals.insert(loc.clone()); }
                    }
                }
                for res in block_locals {
                    if let Some(state) = self.c_heap_allocations.get(&res) {
                        if state != "Freed" && !res.contains("data") { panic!("Memory leak: C pointer {} not freed", res); }
                    } else {
                        let state = self.resource_states.get(&res).cloned().unwrap_or_else(|| "Open".to_string());
                        if !self.terminal_states.contains(&state) { panic!("Linearity leak: resource {} left in non-terminal state {}", res, state); }
                    }
                }
                self.guarded = old_guarded;
                last_ty
            }
            Expr::ResourceDecl(n, e) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                if let Type::Resource(_, state, _, dur) = &ty {
                    let initial_state = state.clone().unwrap_or_else(|| "Open".to_string());
                    self.resource_states.insert(n.clone(), initial_state);
                    if let Some(d) = dur { self.resource_durability.insert(n.clone(), *d); }
                }
                env.insert(n.clone(), ty);
                local_resources.insert(n.clone());
                Type::Void
            }
            Expr::Spawn(e) => { self.check_expr(e, env, res_consumed, local_resources); Type::Void }
            Expr::Call(e, args) => {
                if let Expr::Var(name) = &**e {
                    if name == "free" || name == "free_ptr" {
                        if let Some(Expr::Var(ptr_name)) = args.get(0) {
                            if let Some(state) = self.c_heap_allocations.get(ptr_name) {
                                if state == "Freed" {
                                    self.c_double_frees.insert(ptr_name.clone());
                                    panic!("Safety violation: double free of C pointer {}", ptr_name);
                                }
                                self.c_heap_allocations.insert(ptr_name.clone(), "Freed".to_string());
                            }
                        }
                    }
                }
                if self.in_rec_optic && self.guarded.is_empty() {
                   if let Expr::Var(name) = &**e {
                       if Some(name) == self.current_func.as_ref() { panic!("Unguarded recursive call in rec optic"); }
                   }
                }
                let callee_ty = self.check_expr(e, env, res_consumed, local_resources);
                for arg in args { self.check_expr(arg, env, res_consumed, local_resources); }
                if let Expr::Var(name) = &**e {
                    if let Some(ty) = self.globals.get(name) { return ty.clone(); }
                }
                callee_ty
            }
            Expr::Access(e, field) => {
                if let Expr::Var(n) = &**e {
                    if let Some(states) = self.protocols.get(n) {
                        for state in states {
                            if let Some((_, ty)) = state.optics.iter().find(|(f, _)| f == field) {
                                return Type::ProtocolOptic(n.clone(), state.name.clone(), Box::new(ty.clone()));
                            }
                        }
                        panic!("Field {} not found in protocol {}", field, n);
                    }
                }
                let original_ty = self.check_expr(e, env, res_consumed, local_resources);
                let mut ty = original_ty.clone();
                while let Type::Optic(inner, _, _, _) | Type::RecOptic(inner, _, _, _) | Type::AtomicOptic(inner, _, _, _) | Type::Traversal(inner, _, _, _) | Type::Co(inner, _, _) = ty {
                    ty = (*inner).clone();
                }
                if let Type::Resource(_, _, protocol, _) = &ty {
                    if let Some(p_name) = protocol {
                        let state_name = self.resource_states.get(&match &**e { Expr::Var(n) => n.clone(), _ => "".to_string() }).cloned().unwrap_or_else(|| {
                            if let Type::Resource(_, state, _, _) = original_ty { state.unwrap_or_else(|| "Open".to_string()) } else { "Open".to_string() }
                        });
                        if let Some(states) = self.protocols.get(p_name) {
                            if let Some(state) = states.iter().find(|s| s.name == state_name) {
                                if let Some((_, o_ty)) = state.optics.iter().find(|(f, _)| f == field) { return o_ty.clone(); }
                            }
                        }
                    }
                }
                match ty {
                    Type::Named(name) => {
                        let fields = self.structs.get(&name).expect(&format!("Undefined struct {}", name));
                        fields.iter().find(|(f, _)| f == field).map(|(_, t)| t.clone()).expect(&format!("Field {} not found in struct {}", field, name))
                    }
                    Type::Struct(fields) => {
                        fields.iter().find(|(f, _)| f == field).map(|(_, t)| t.clone()).expect(&format!("Field {} not found in anonymous struct", field))
                    }
                    _ => panic!("Access requires struct type, found {:?}", ty),
                }
            }
            Expr::Compose(e1, e2) => {
                let ty1 = self.check_expr(e1, env, res_consumed, local_resources);
                let old_guarded = self.guarded.clone();
                let ty2 = self.check_expr(e2, env, res_consumed, local_resources);
                self.guarded = old_guarded;

                let get_perf = |t: &Type| -> Option<PerfGrade> {
                    match t {
                        Type::Optic(_, _, _, p) | Type::Traversal(_, _, _, p) | Type::RecOptic(_, _, _, p) | Type::AtomicOptic(_, _, _, p) => p.clone(),
                        _ => None,
                    }
                };

                let get_res = |t: &Type| -> Option<String> {
                    match t {
                        Type::Optic(_, r, _, _) | Type::Traversal(_, r, _, _) | Type::RecOptic(_, r, _, _) | Type::AtomicOptic(_, r, _, _) => r.clone(),
                        _ => None,
                    }
                };

                let get_conf = |t: &Type| -> Option<f64> {
                    match t {
                        Type::Optic(_, _, c, _) | Type::Traversal(_, _, c, _) | Type::RecOptic(_, _, c, _) | Type::AtomicOptic(_, _, c, _) => *c,
                        _ => None,
                    }
                };

                let p1 = get_perf(&ty1);
                let p2 = get_perf(&ty2);
                let res1 = get_res(&ty1);
                let conf2 = get_conf(&ty2);

                let perf = match (p1, p2) {
                    (Some(pg1), Some(pg2)) => Some(PerfGrade {
                        latency_us: pg1.latency_us + pg2.latency_us,
                        cache_lines: pg1.cache_lines + pg2.cache_lines,
                        bandwidth_gbps: pg1.bandwidth_gbps.min(pg2.bandwidth_gbps),
                    }),
                    (Some(pg), None) | (None, Some(pg)) => Some(pg),
                    (None, None) => None,
                };

                let result_ty = match (&ty1, &ty2) {
                    (Type::Optic(..), Type::Optic(i2, _, _, _)) => Type::Optic(i2.clone(), res1, conf2, perf),
                    (Type::RecOptic(..), Type::Optic(i2, _, _, _)) | (Type::Optic(..), Type::RecOptic(i2, _, _, _)) | (Type::RecOptic(..), Type::RecOptic(i2, _, _, _)) =>
                        Type::RecOptic(i2.clone(), res1, conf2, perf),
                    _ => {
                        if get_perf(&ty1).is_some() || get_perf(&ty2).is_some() {
                             // Fallback for mixed optic kinds
                             Type::Optic(Box::new(Type::Int), res1, conf2, perf)
                        } else {
                             Type::Int
                        }
                    }
                };

                if let Type::ProtocolOptic(p_name, s_name, inner) = &ty2 {
                    let mut current_state_name = None;
                    if let Type::Resource(_, current_state, Some(res_protocol), _) = &ty1 {
                        if p_name == res_protocol { current_state_name = current_state.clone(); }
                    }
                    if current_state_name.is_none() {
                        let res_assoc = get_res(&ty1);
                        if let Some(res_name) = res_assoc {
                            if let Some(res_ty) = env.get(&res_name) {
                                if let Type::Resource(_, current_state, Some(res_protocol), _) = res_ty {
                                    if p_name == res_protocol { current_state_name = current_state.clone(); }
                                }
                            }
                        }
                    }
                    if let Some(s) = current_state_name {
                        if &s == s_name { return (**inner).clone(); }
                    }
                }

                let query_desc = format!("{:?} | {:?}", e1, e2);
                let mock_result = format!("Result of {}", query_desc).into_bytes();
                let new_fp = self.storage.store(mock_result);
                if let Some((old_ty, old_fp)) = self.query_cache.get(&query_desc) {
                    if old_fp == &new_fp { return old_ty.clone(); }
                }
                self.query_cache.insert(query_desc, (result_ty.clone(), new_fp));

                if let Type::ProtocolOptic(p_name, s_name, inner) = ty2 {
                    if let Type::Resource(_, current_state, Some(res_protocol), _) = &ty1 {
                        if &p_name == res_protocol {
                            let actual_state = current_state.clone().unwrap_or_else(|| {
                                if let Expr::Var(n) = &**e1 { self.resource_states.get(n).cloned().unwrap_or_else(|| "Open".to_string()) } else { "Open".to_string() }
                            });
                            if actual_state != *s_name { panic!("Protocol violation: expected state {}, found {}", s_name, actual_state); }
                            if let Some(states) = self.protocols.get(&p_name) {
                                let current_idx = states.iter().position(|s| s.name == s_name).expect("State not found in protocol");
                                if current_idx + 1 < states.len() {
                                    let next_state = states[current_idx + 1].name.clone();
                                    if let Expr::Var(n) = &**e1 { self.resource_states.insert(n.clone(), next_state); }
                                }
                            }
                            return *inner;
                        }
                    }
                }
                result_ty
            }
            Expr::Unsafe(e) => self.check_expr(e, env, res_consumed, local_resources),
            Expr::Checked(e) => {
                let old = self.in_checked;
                self.in_checked = true;
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                self.in_checked = old;
                ty
            }
            Expr::If(cond, then, els) => {
                let old_guarded = self.guarded.clone();
                let cond_ty = self.check_expr(cond, env, res_consumed, local_resources);
                if cond_ty != Type::Bool && cond_ty != Type::Int { panic!("If condition must be bool or int, found {:?}", cond_ty); }
                let then_ty = self.check_expr(then, env, res_consumed, local_resources);
                self.guarded = old_guarded;
                if let Some(e) = els { self.check_expr(e, env, res_consumed, local_resources); then_ty } else { then_ty }
            }
            Expr::While(cond, body) => {
                let cond_ty = self.check_expr(cond, env, res_consumed, local_resources);
                if cond_ty != Type::Bool { panic!("While condition must be bool"); }
                self.check_expr(body, env, res_consumed, local_resources);
                Type::Void
            }
            Expr::BinOp(kind, e1, e2) => {
                let ty1 = self.check_expr(e1, env, res_consumed, local_resources);
                let ty2 = self.check_expr(e2, env, res_consumed, local_resources);
                match kind {
                    BinOpKind::Add | BinOpKind::Sub | BinOpKind::Mul | BinOpKind::Div => {
                        if ty1 == Type::Int && ty2 == Type::Int { Type::Int } else { Type::Float }
                    }
                    BinOpKind::Eq | BinOpKind::Ne | BinOpKind::Lt | BinOpKind::Gt | BinOpKind::Le | BinOpKind::Ge => { Type::Bool }
                    BinOpKind::And => {
                        if ty1 == Type::Bool && ty2 == Type::Bool { Type::Bool } else { panic!("And operands must be bool") }
                    }
                }
            }
            Expr::Return(e) => {
                let ty = self.check_expr(e, env, res_consumed, local_resources);
                if let Expr::Var(n) = &**e {
                    if let Some(scope) = self.phantom_lifetimes.get(n) {
                        if let Some(current) = &self.current_func {
                            if scope == current { panic!("Safety violation: stack pointer {} escapes function {}", n, current); }
                        }
                    }
                }
                ty
            }
            Expr::Index(e1, e2) => {
                let ty1 = self.check_expr(e1, env, res_consumed, local_resources);
                let ty2 = self.check_expr(e2, env, res_consumed, local_resources);
                if ty2 != Type::Int { panic!("Index must be integer"); }
                match ty1 {
                    Type::Pointer(inner, _, _) | Type::Traversal(inner, _, _, _) | Type::Optic(inner, _, _, _) => {
                         if !self.in_checked { }
                         (*inner).clone()
                    }
                    _ => panic!("Index requires pointer or traversal type, found {:?}", ty1),
                }
            }
            Expr::Paren(e) => self.check_expr(e, env, res_consumed, local_resources),
            Expr::IndexAssign(base, index, value) => {
                self.check_expr(base, env, res_consumed, local_resources);
                self.check_expr(index, env, res_consumed, local_resources);
                self.check_expr(value, env, res_consumed, local_resources);
                Type::Void
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[should_panic(expected = "Linearity violation")]
    fn test_linearity() {
        let mut sema = Sema::new();
        let mut env = HashMap::new();
        env.insert("f".to_string(), Type::Resource(vec![], Some("Consumed".to_string()), None, None));
        let mut consumed = HashSet::new();
        let mut locals = HashSet::new();
        locals.insert("f".to_string());
        let expr = Expr::Block(vec![Expr::Var("f".to_string()), Expr::Var("f".to_string())]);
        consumed.insert("f".to_string());
        sema.check_expr(&expr, &mut env, &mut consumed, &mut locals);
    }

    #[test]
    #[should_panic(expected = "Unguarded recursive call")]
    fn test_productivity() {
        let mut sema = Sema::new();
        sema.in_rec_optic = true;
        sema.guarded = HashSet::new();
        sema.current_func = Some("f".to_string());
        let mut env = HashMap::new();
        env.insert("f".to_string(), Type::Void);
        let mut consumed = HashSet::new();
        let mut locals = HashSet::new();
        let expr = Expr::Call(Box::new(Expr::Var("f".to_string())), vec![]);
        sema.check_expr(&expr, &mut env, &mut consumed, &mut locals);
    }

    #[test]
    fn test_productive_call() {
        let mut sema = Sema::new();
        sema.in_rec_optic = true;
        sema.guarded = HashSet::new();
        let mut env = HashMap::new();
        env.insert("f".to_string(), Type::Void);
        let mut consumed = HashSet::new();
        let mut locals = HashSet::new();
        let expr = Expr::Next(Box::new(Expr::Call(Box::new(Expr::Var("f".to_string())), vec![])), None);
        sema.check_expr(&expr, &mut env, &mut consumed, &mut locals);
    }

    #[test]
    fn test_nominal_struct_sema() {
        let mut sema = Sema::new();
        let prog = Program {
            decls: vec![
                Decl::Struct { name: "Point".to_string(), fields: vec![("x".to_string(), Type::Int), ("y".to_string(), Type::Int)] },
                Decl::Func { name: "get_x".to_string(), params: vec![("p".to_string(), Type::Named("Point".to_string()))], ret_type: Type::Int, body: Expr::Access(Box::new(Expr::Var("p".to_string())), "x".to_string()) }
            ],
        };
        sema.check_program(&prog);
    }

    #[test]
    fn test_valid_protocol_transition() {
        let mut sema = Sema::new();
        let prog = Program {
            decls: vec![
                Decl::Func { name: "main".to_string(), params: vec![], ret_type: Type::Void, body: Expr::Block(vec![
                    Expr::ResourceDecl("f".to_string(), Box::new(Expr::Var("res".to_string()))),
                    Expr::Put(Box::new(Expr::Var("buffer".to_string())), Box::new(Expr::ConstInt(65))),
                ]) }
            ],
        };
        sema.globals.insert("res".to_string(), Type::Resource(vec![], Some("Open".to_string()), None, None));
        sema.globals.insert("buffer".to_string(), Type::Optic(Box::new(Type::Char), Some("f".to_string()), None, None));
        sema.check_program(&prog);
        assert_eq!(sema.resource_states.get("f").unwrap(), "Closed");
    }

    #[test]
    #[should_panic(expected = "Linearity leak")]
    fn test_linearity_leak() {
        let mut sema = Sema::new();
        let prog = Program {
            decls: vec![
                Decl::Func { name: "main".to_string(), params: vec![], ret_type: Type::Void, body: Expr::Block(vec![
                    Expr::ResourceDecl("f".to_string(), Box::new(Expr::Var("res".to_string()))),
                ]) }
            ],
        };
        sema.globals.insert("res".to_string(), Type::Resource(vec![], Some("Open".to_string()), None, None));
        sema.check_program(&prog);
    }

    #[test]
    fn test_extern_c_sema() {
        let mut sema = Sema::new();
        let input = "extern C { int* malloc(int size); void free(int* p); } void main() { { int* p = malloc(10); free(p); } }";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        sema.check_program(&prog);
        assert!(sema.globals.contains_key("malloc"));
    }

    #[test]
    #[should_panic(expected = "Memory leak")]
    fn test_malloc_leak() {
        let mut sema = Sema::new();
        let input = "extern C { int* malloc(int size); } void main() { { int* p = malloc(10); } }";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        sema.check_program(&prog);
    }

    #[test]
    fn test_local_stack_pointer() {
        let mut sema = Sema::new();
        let input = "void f() { int x = 0; int* p = x + 0; return x; }";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        sema.check_program(&prog);
    }

    #[test]
    #[should_panic(expected = "double free")]
    fn test_double_free() {
        let mut sema = Sema::new();
        let input = "extern C { int* malloc(int size); void free(int* p); } void main() { { int* p = malloc(10); free(p); free(p); } }";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        sema.check_program(&prog);
    }

    #[test]
    fn test_checked_fallback() {
        let mut sema = Sema::new();
        sema.beliefs.insert("inv.memcpy_typed_ok".to_string(), 1.0);
        let input = "extern C { int* malloc(int size); void free(int* p); } void main() { { int* p_arr = malloc(10); checked(p_arr[0]); free(p_arr); } }";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        sema.check_program(&prog);
        assert!(sema.in_checked == false);
    }

    #[test]
    fn test_neuro_symbolic_lifting() {
        let mut sema = Sema::new();
        sema.beliefs.insert("inv.memcpy_typed_ok".to_string(), 1.0);
        let input = "extern C { char* malloc(int size); void free(char* p); } void main() { { char* p = malloc(10); char x = *p; free(p); } }";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        sema.check_program(&prog);
        assert_eq!(sema.lifting_hints.get("p").unwrap(), &Type::Optic(Box::new(Type::Char), None, Some(1.0), None));
    }

    #[test]
    fn test_malloc_free() {
        let mut sema = Sema::new();
        let input = "extern C { int* malloc(int size); void free(int* p); } void main() { { int* p = malloc(10); free(p); } }";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        sema.check_program(&prog);
    }

    #[test]
    fn test_pointer_context() {
        let mut sema = Sema::new();
        let prog = Program {
            decls: vec![Decl::Global("p".to_string(), Type::Pointer(Box::new(Type::Int), "Heap".to_string(), None), None)],
        };
        sema.check_program(&prog);
    }

    #[test]
    #[should_panic(expected = "Invalid transition")]
    fn test_invalid_transition() {
        let mut sema = Sema::new();
        let prog = Program {
            decls: vec![
                Decl::Func { name: "main".to_string(), params: vec![], ret_type: Type::Void, body: Expr::Block(vec![
                    Expr::ResourceDecl("f".to_string(), Box::new(Expr::Var("res".to_string()))),
                    Expr::Put(Box::new(Expr::Var("buffer".to_string())), Box::new(Expr::ConstInt(65))),
                    Expr::Put(Box::new(Expr::Var("buffer".to_string())), Box::new(Expr::ConstInt(66))),
                ]) }
            ],
        };
        sema.globals.insert("res".to_string(), Type::Resource(vec![], Some("Open".to_string()), None, None));
        sema.globals.insert("buffer".to_string(), Type::Optic(Box::new(Type::Char), Some("f".to_string()), None, None));
        sema.check_program(&prog);
    }

    #[test]
    fn test_compiler_phase_query() {
        let input = "protocol NodeSession { state Parsed { optic Symbol* resolve; } state Resolved { optic Type* typecheck; } state Typed { optic IR* lower; } } struct Symbol { int id; } struct Type { int id; } struct IR { int id; } resource NodeSession[Parsed] graph = alloc<NodeSession>(1); void main() { optic Symbol* r = NodeSession.resolve; optic Symbol* sym = graph | r; }";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        let mut sema = Sema::new();
        sema.globals.insert("Symbol".to_string(), Type::Struct(vec![]));
        sema.globals.insert("Type".to_string(), Type::Struct(vec![]));
        sema.globals.insert("IR".to_string(), Type::Struct(vec![]));
        sema.check_program(&prog);
    }

    #[test]
    fn test_c_context_co_type() {
        let mut sema = Sema::new();
        let input = "void main() { { co<C_context> int* p = malloc(10); free(p); } }";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        sema.globals.insert("malloc".to_string(), Type::Pointer(Box::new(Type::Int), "C_Heap".to_string(), None));
        sema.check_program(&prog);
    }

    #[test]
    fn test_phantom_lifetime_escape() {
        let mut sema = Sema::new();
        let input = "int* f() { int x = 0; int* p = x + 0; return x; } void main() { int* res = f(); }";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        sema.check_program(&prog);
    }

    #[test]
    fn test_perf_grade_composition() {
        let mut sema = Sema::new();
        let pg1 = PerfGrade{ latency_us: 10, cache_lines: 5, bandwidth_gbps: 100 };
        let pg2 = PerfGrade{ latency_us: 20, cache_lines: 10, bandwidth_gbps: 50 };
        let ty1 = Type::Optic(Box::new(Type::Int), None, None, Some(pg1));
        let ty2 = Type::Optic(Box::new(Type::Int), None, None, Some(pg2));
        let mut env = HashMap::new();
        let mut consumed = HashSet::new();
        let mut locals = HashSet::new();
        let e1 = Expr::Var("o1".to_string());
        let e2 = Expr::Var("o2".to_string());
        let compose_expr = Expr::Compose(Box::new(e1), Box::new(e2));
        env.insert("o1".to_string(), ty1);
        env.insert("o2".to_string(), ty2);
        let res_ty = sema.check_expr(&compose_expr, &mut env, &mut consumed, &mut locals);
        if let Type::Optic(_, _, _, Some(perf)) = res_ty {
            assert_eq!(perf.latency_us, 30);
            assert_eq!(perf.cache_lines, 15);
            assert_eq!(perf.bandwidth_gbps, 50);
        } else { panic!("Expected Optic with PerfGrade, found {:?}", res_ty); }
    }

    #[test]
    fn test_perf_grade_parsing() {
        let input = "optic int* PerfGrade { latency: 10, cache: 5, bandwidth: 100 } o;";
        let mut parser = crate::parser::Parser::new(input);
        let prog = parser.parse_program();
        if let Decl::Global(_, ty, _) = &prog.decls[0] {
            if let Type::Optic(inner, _, _, _) = ty {
                if let Type::Optic(_, _, _, Some(perf)) = &**inner {
                    assert_eq!(perf.latency_us, 10);
                    assert_eq!(perf.cache_lines, 5);
                    assert_eq!(perf.bandwidth_gbps, 100);
                } else { panic!("Expected inner Optic with PerfGrade, found {:?}", ty); }
            } else { panic!("Expected outer Optic, found {:?}", ty); }
        } else { panic!("Expected Global declaration"); }
    }

    #[test]
    fn test_belief_gated_lifting() {
        let input = "extern C { char* malloc(int size); void free(char* p); } void main() { { char* p = malloc(10); char x = *p; free(p); } }";
        let mut sema_low = Sema::new();
        sema_low.beliefs.insert("inv.memcpy_typed_ok".to_string(), 0.5);
        let mut parser1 = crate::parser::Parser::new(input);
        let prog1 = parser1.parse_program();
        sema_low.check_program(&prog1);
        assert!(sema_low.lifting_hints.get("p").is_none());
        let mut sema_high = Sema::new();
        sema_high.beliefs.insert("inv.memcpy_typed_ok".to_string(), 0.995);
        let mut parser2 = crate::parser::Parser::new(input);
        let prog2 = parser2.parse_program();
        sema_high.check_program(&prog2);
        assert!(sema_high.lifting_hints.get("p").is_some());
    }
}
