struct Unit {
}
struct Pair {
  Int fst;
  Int snd;
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
  RecOptic(Optic(Named("ASTNode"), None, None), None, None) left;
  RecOptic(Optic(Named("ASTNode"), None, None), None, None) right;
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
struct Token {
  Int tag;
  Named("String") lexeme;
  Int line;
}
struct Lexer {
  Named("String") input;
  Int pos;
  Int line;
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
    alloc<Named("Node"), None> (
      block {
      },
      block {
      },
      load name,
      load fields,
      block {
      },
      block {
      },
      call string_slice (
        block {
        },
        block {
        },
        block {
        },
      ),
      block {
      },
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
    alloc<Named("Node"), None> (
      block {
      },
      block {
      },
      load name,
      load states,
      block {
      },
      block {
      },
      call string_slice (
        block {
        },
        block {
        },
        block {
        },
      ),
      block {
      },
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
