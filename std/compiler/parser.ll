struct Unit {
}
struct Pair {
  Int fst;
  Int snd;
}
struct Fingerprint {
  Int hi;
  Int lo;
}
struct OptionInt {
  Int tag;
  Int value;
}
struct OptionChar {
  Int tag;
  Char value;
}
struct OptionBool {
  Int tag;
  Bool value;
}
struct ResultInt {
  Int tag;
  Int value;
}
struct ResultVoid {
  Int tag;
}
struct String {
  Int len;
  Pointer(Char, "Heap", None) data;
}
struct Token {
  Int tag;
  Named("String") lexeme;
  Int line;
}
struct ASTNode {
  Int tag;
  Int value;
  RecOptic(Optic(Named("ASTNode"), None, None, None), None, None, None) left;
  RecOptic(Optic(Named("ASTNode"), None, None, None), None, None, None) right;
}
block {
  load o
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  load c
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
    block {
    }
    block {
    }
    block {
    }
    call string_copy_rec (
      load new_data,
      block {
      },
      load new_len,
      block {
      },
      block {
      },
    )
    alloc<Named("String"), None> (
      load new_len,
      load new_data,
    )
  }
}
block {
  block {
    block {
    }
    block {
    }
    call string_copy_rec (
      load new_data,
      block {
      },
      block {
      },
      block {
      },
      block {
      },
    )
    put (
      block {
      }    ,
      load c    )
    alloc<Named("String"), None> (
      load new_len,
      load new_data,
    )
  }
}
block {
  block {
    block {
    }
    block {
    }
    call string_copy_rec (
      load new_data,
      block {
      },
      block {
      },
      block {
      },
      block {
      },
    )
    call string_copy_rec (
      load new_data,
      block {
      },
      block {
      },
      block {
      },
      block {
      },
    )
    alloc<Named("String"), None> (
      load new_len,
      load new_data,
    )
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  call string_compare_rec (
    block {
    },
    block {
    },
    block {
    },
    block {
    },
    block {
    },
  )
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
call malloc_ptr (
)
call free_ptr (
)
struct PtrVector {
  Int size;
  Int capacity;
  Pointer(Pointer(Named("Unit"), "Heap", None), "Heap", None) data;
}
block {
  alloc<Named("PtrVector"), None> (
    block {
    },
    load cap,
    call malloc_ptr (
      block {
      },
    ),
  )
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  put (
    block {
    }  ,
    load val  )
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  put (
    block {
    }  ,
    block {
    }  )
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
call malloc_int (
)
call free_int (
)
struct Vector {
  Int size;
  Int capacity;
  Pointer(Int, "Heap", None) data;
}
block {
  alloc<Named("Vector"), None> (
    block {
    },
    load cap,
    call malloc_int (
      block {
      },
    ),
  )
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  put (
    block {
    }  ,
    load val  )
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  put (
    block {
    }  ,
    block {
    }  )
}
block {
  call vector_contains_rec (
    load v,
    load val,
    block {
    },
  )
}
block {
  block {
  }
}
struct CAS {
  Named("PtrVector") entries;
}
struct CASEntry {
  Named("Fingerprint") fingerprint;
  Pointer(Named("Unit"), "Heap", None) data;
}
block {
  block {
    block {
    }
    block {
    }
    block {
    }
    alloc<Named("Fingerprint"), None> (
      load hi,
      load lo,
    )
  }
}
block {
  alloc<Named("CAS"), None> (
    call ptr_vector_new (
      block {
      },
    ),
  )
}
block {
  call ptr_vector_push (
    block {
    },
    alloc<Named("CASEntry"), None> (
      load fp,
      load data,
    ),
  )
}
block {
  call cas_fetch_rec (
    block {
    },
    load fp,
    block {
    },
  )
}
block {
  block {
  }
}
struct Node {
  Int id;
  Int kind;
  Named("String") lexeme;
  Named("PtrVector") children;
  Optic(Optic(Named("Node"), None, None, None), None, None, None) parent;
  Optic(Optic(Named("Node"), None, None, None), None, None, None) next;
  Named("String") source_file;
  Int line_number;
  Named("Fingerprint") fingerprint;
  Int durability;
}
protocol NodeSession {
  state Parsed {
    Optic(Optic(Named("Symbol"), None, None, None), None, None, None) resolve;
  }
  state Resolved {
    Optic(Optic(Named("TypeIR"), None, None, None), None, None, None) typecheck;
  }
  state Typed {
    Optic(Optic(Named("IR"), None, None, None), None, None, None) lower;
  }
  state Lowered {
  }
}
block {
  alloc<Named("Fingerprint"), None> (
    block {
    },
    block {
    },
  )
}
block {
  alloc<Named("Symbol"), None> (
    block {
    },
    block {
    },
    load n,
    call node_empty_fingerprint (
    ),
  )
}
block {
  alloc<Named("TypeIR"), None> (
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    ),
    block {
    },
    call ptr_vector_new (
      block {
      },
    ),
    call node_empty_fingerprint (
    ),
  )
}
block {
  alloc<Named("IR"), None> (
    block {
    },
    block {
    },
    call ptr_vector_new (
      block {
      },
    ),
    call node_empty_fingerprint (
    ),
  )
}
struct Symbol {
  Named("String") name;
  Int scope_id;
  Pointer(Named("Unit"), "Heap", None) node_ref;
  Named("Fingerprint") fingerprint;
}
struct TypeIR {
  Named("String") kind;
  Int size;
  Named("PtrVector") params;
  Named("Fingerprint") fingerprint;
}
struct IR {
  Int opcode;
  Int value;
  Named("PtrVector") inputs;
  Named("Fingerprint") fingerprint;
}
struct Lexer {
  Named("String") input;
  Int pos;
  Int line;
  Named("String") source_file;
}
block {
  block {
  }
}
block {
  block {
    block {
    }
    block {
    }
    block {
    }
    load c
  }
}
block {
  block {
    block {
    }
    block {
    }
  }
}
block {
  block {
    block {
    }
    block {
    }
  }
}
block {
  block {
    block {
    }
    block {
    }
  }
}
block {
  block {
    block {
    }
    block {
    }
  }
}
block {
  block {
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    alloc<Named("Token"), None> (
      block {
      },
      load s,
      load line,
    )
  }
}
block {
  block {
    block {
    }
    block {
    }
  }
}
block {
  block {
    call lexer_skip_whitespace (
      load l,
    )
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    block {
    }
    alloc<Named("Token"), None> (
      block {
      },
      call string_slice (
        block {
        },
        load start_pos,
        block {
        },
      ),
      block {
      },
    )
  }
}
struct Parser {
  Named("Lexer") lexer;
  Named("Token") current;
}
block {
  put (
    block {
    }  ,
    call lexer_next_token (
      block {
      },
    )  )
}
block {
  block {
  }
}
block {
  alloc<Named("Node"), None> (
    block {
    },
    load kind,
    load lexeme,
    load children,
    block {
    },
    block {
    },
    block {
    },
    block {
    },
    call node_empty_fingerprint (
    ),
    block {
    },
  )
}
block {
  block {
    block {
    }
    block {
    }
    call parser_advance (
      load p,
    )
    block {
    }
    block {
    }
    load ty
  }
}
block {
  block {
    block {
    }
  }
}
block {
  block {
  }
}
block {
  block {
    block {
    }
    block {
    }
  }
}
block {
  block {
  }
}
block {
  block {
    block {
    }
    block {
    }
  }
}
block {
  block {
  }
}
block {
  block {
    call parser_advance (
      load p,
    )
    block {
    }
    call parser_advance (
      load p,
    )
    call parser_expect (
      load p,
      block {
      },
    )
    block {
    }
    call parser_parse_struct_fields (
      load p,
      load fields,
    )
    call parser_expect (
      load p,
      block {
      },
    )
    call parser_make_node (
      block {
      },
      load name,
      load fields,
      load p,
    )
  }
}
block {
  block {
  }
}
block {
  block {
  }
}
block {
  block {
    call parser_advance (
      load p,
    )
    block {
    }
    call parser_advance (
      load p,
    )
    call parser_expect (
      load p,
      block {
      },
    )
    block {
    }
    call parser_parse_protocol_states (
      load p,
      load states,
    )
    call parser_expect (
      load p,
      block {
      },
    )
    call parser_make_node (
      block {
      },
      load name,
      load states,
      load p,
    )
  }
}
block {
  block {
  }
}
block {
  block {
    block {
    }
  }
}
