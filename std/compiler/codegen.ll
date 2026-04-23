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
  call string_to_int_rec (
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
protocol File {
  state Open {
    Optic(Optic(Int, None, None, None), None, None, None) read;
    Optic(Optic(Int, None, None, None), None, None, None) write;
  }
  state Closed {
  }
}
struct FileObj {
  Int fd;
}
struct Console {
  Int id;
}
resource Named("Console") console = (
  alloc<Named("Console"), None> (
    block {
    },
  ))
block {
}
block {
}
block {
  block {
  }
}
block {
}
block {
}
block {
  alloc<Named("FileObj"), None> (
    block {
    },
  )
}
block {
  put (
    load closer  ,
    block {
    }  )
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
    call parser_advance (
      load p,
    )
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
    call parser_parse_extern_decls (
      load p,
      load decls,
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
      call string_slice (
        block {
        },
        block {
        },
        block {
        },
      ),
      load decls,
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
block {
  block {
  }
}
block {
  block {
    block {
    }
    call parser_parse_program_decls (
      load p,
      load decls,
    )
    alloc<Named("Node"), None> (
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
      load decls,
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
struct CodegenContext {
  Named("PtrVector") functions;
  Named("PtrVector") global_decls;
  Int label_counter;
  Int temp_counter;
  Named("String") current_function;
}
block {
  alloc<Named("CodegenContext"), None> (
    call ptr_vector_new (
      block {
      },
    ),
    call ptr_vector_new (
      block {
      },
    ),
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
  )
}
block {
  block {
    put (
      block {
      }    ,
      block {
      }    )
    call string_concat (
      call string_concat (
        call string_concat (
          call string_concat (
            alloc<Named("String"), None> (
              block {
              },
              alloc<Char, Some(Normal)> (
                block {
                },
              ),
            ),
            alloc<Named("String"), None> (
              block {
              },
              alloc<Char, Some(Normal)> (
                block {
                },
              ),
            ),
          ),
          alloc<Named("String"), None> (
            block {
            },
            alloc<Char, Some(Normal)> (
              block {
              },
            ),
          ),
        ),
        call int_to_string (
          block {
          },
        ),
      ),
      alloc<Named("String"), None> (
        block {
        },
        alloc<Char, Some(Normal)> (
          block {
          },
        ),
      ),
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
    put (
      block {
      }    ,
      block {
      }    )
    call string_concat (
      call string_concat (
        alloc<Named("String"), None> (
          block {
          },
          alloc<Char, Some(Normal)> (
            block {
            },
          ),
        ),
        call int_to_string (
          block {
          },
        ),
      ),
      alloc<Named("String"), None> (
        block {
        },
        alloc<Char, Some(Normal)> (
          block {
          },
        ),
      ),
    )
  }
}
block {
  block {
  }
}
block {
  call string_concat (
    call string_concat (
      call string_concat (
        load ret_type,
        alloc<Named("String"), None> (
          block {
          },
          alloc<Char, Some(Normal)> (
            block {
            },
          ),
        ),
      ),
      load name,
    ),
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    ),
  )
}
block {
  call string_concat (
    call string_concat (
      call string_concat (
        alloc<Named("String"), None> (
          block {
          },
          alloc<Char, Some(Normal)> (
            block {
            },
          ),
        ),
        load ty,
      ),
      alloc<Named("String"), None> (
        block {
        },
        alloc<Char, Some(Normal)> (
          block {
          },
        ),
      ),
    ),
    load name,
  )
}
block {
  call string_concat (
    call string_concat (
      call string_concat (
        call string_concat (
          alloc<Named("String"), None> (
            block {
            },
            alloc<Char, Some(Normal)> (
              block {
              },
            ),
          ),
          load ty,
        ),
        alloc<Named("String"), None> (
          block {
          },
          alloc<Char, Some(Normal)> (
            block {
            },
          ),
        ),
      ),
      load val,
    ),
    call string_concat (
      alloc<Named("String"), None> (
        block {
        },
        alloc<Char, Some(Normal)> (
          block {
          },
        ),
      ),
      load ptr,
    ),
  )
}
block {
  call string_concat (
    call string_concat (
      alloc<Named("String"), None> (
        block {
        },
        alloc<Char, Some(Normal)> (
          block {
          },
        ),
      ),
      call string_concat (
        load ty,
        alloc<Named("String"), None> (
          block {
          },
          alloc<Char, Some(Normal)> (
            block {
            },
          ),
        ),
      ),
    ),
    load name,
  )
}
block {
  call string_concat (
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    ),
    call string_concat (
      load val,
      alloc<Named("String"), None> (
        block {
        },
        alloc<Char, Some(Normal)> (
          block {
          },
        ),
      ),
    ),
  )
}
block {
  alloc<Named("String"), None> (
    block {
    },
    alloc<Char, Some(Normal)> (
      block {
      },
    ),
  )
}
block {
  call string_concat (
    call string_concat (
      call string_concat (
        call string_concat (
          alloc<Named("String"), None> (
            block {
            },
            alloc<Char, Some(Normal)> (
              block {
              },
            ),
          ),
          load ty,
        ),
        alloc<Named("String"), None> (
          block {
          },
          alloc<Char, Some(Normal)> (
            block {
            },
          ),
        ),
      ),
      call string_concat (
        load lhs,
        call string_concat (
          alloc<Named("String"), None> (
            block {
            },
            alloc<Char, Some(Normal)> (
              block {
              },
            ),
          ),
          load rhs,
        ),
      ),
    ),
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    ),
  )
}
block {
  call string_concat (
    call string_concat (
      call string_concat (
        call string_concat (
          call string_concat (
            alloc<Named("String"), None> (
              block {
              },
              alloc<Char, Some(Normal)> (
                block {
                },
              ),
            ),
            alloc<Named("String"), None> (
              block {
              },
              alloc<Char, Some(Normal)> (
                block {
                },
              ),
            ),
          ),
          load pred,
        ),
        alloc<Named("String"), None> (
          block {
          },
          alloc<Char, Some(Normal)> (
            block {
            },
          ),
        ),
      ),
      call string_concat (
        load lhs,
        call string_concat (
          alloc<Named("String"), None> (
            block {
            },
            alloc<Char, Some(Normal)> (
              block {
              },
            ),
          ),
          load rhs,
        ),
      ),
    ),
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    ),
  )
}
block {
  call string_concat (
    call string_concat (
      call string_concat (
        call string_concat (
          call string_concat (
            call string_concat (
              call string_concat (
                call string_concat (
                  call string_concat (
                    alloc<Named("String"), None> (
                      block {
                      },
                      alloc<Char, Some(Normal)> (
                        block {
                        },
                      ),
                    ),
                    load ty,
                  ),
                  alloc<Named("String"), None> (
                    block {
                    },
                    alloc<Char, Some(Normal)> (
                      block {
                      },
                    ),
                  ),
                ),
                load first_label,
              ),
              alloc<Named("String"), None> (
                block {
                },
                alloc<Char, Some(Normal)> (
                  block {
                  },
                ),
              ),
            ),
            load first_val,
          ),
          alloc<Named("String"), None> (
            block {
            },
            alloc<Char, Some(Normal)> (
              block {
              },
            ),
          ),
        ),
        load second_label,
      ),
      alloc<Named("String"), None> (
        block {
        },
        alloc<Char, Some(Normal)> (
          block {
          },
        ),
      ),
    ),
    call string_concat (
      load second_val,
      alloc<Named("String"), None> (
        block {
        },
        alloc<Char, Some(Normal)> (
          block {
          },
        ),
      ),
    ),
  )
}
block {
  block {
    call string_concat (
      call string_concat (
        call string_concat (
          load ret_ty,
          alloc<Named("String"), None> (
            block {
            },
            alloc<Char, Some(Normal)> (
              block {
              },
            ),
          ),
        ),
        load func,
      ),
      alloc<Named("String"), None> (
        block {
        },
        alloc<Char, Some(Normal)> (
          block {
          },
        ),
      ),
    )
  }
}
block {
  call string_concat (
    call string_concat (
      call string_concat (
        alloc<Named("String"), None> (
          block {
          },
          alloc<Char, Some(Normal)> (
            block {
            },
          ),
        ),
        load struct_ty,
      ),
      alloc<Named("String"), None> (
        block {
        },
        alloc<Char, Some(Normal)> (
          block {
          },
        ),
      ),
    ),
    call string_concat (
      load ptr,
      call string_concat (
        alloc<Named("String"), None> (
          block {
          },
          alloc<Char, Some(Normal)> (
            block {
            },
          ),
        ),
        call int_to_string (
          load field_idx,
        ),
      ),
    ),
  )
}
block {
  call string_concat (
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    ),
    load ty,
  )
}
block {
  call string_concat (
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    ),
    load ptr,
  )
}
block {
  call string_concat (
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    ),
    load label,
  )
}
block {
  call string_concat (
    call string_concat (
      call string_concat (
        call string_concat (
          alloc<Named("String"), None> (
            block {
            },
            alloc<Char, Some(Normal)> (
              block {
              },
            ),
          ),
          load cond,
        ),
        alloc<Named("String"), None> (
          block {
          },
          alloc<Char, Some(Normal)> (
            block {
            },
          ),
        ),
      ),
      call string_concat (
        load then_label,
        call string_concat (
          alloc<Named("String"), None> (
            block {
            },
            alloc<Char, Some(Normal)> (
              block {
              },
            ),
          ),
          load else_label,
        ),
      ),
    ),
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    ),
  )
}
block {
  call string_concat (
    load name,
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    ),
  )
}
block {
  alloc<Named("String"), None> (
    block {
    },
    alloc<Char, Some(Normal)> (
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
    block {
    }
    alloc<Named("String"), None> (
      block {
      },
      alloc<Char, Some(Normal)> (
        block {
        },
      ),
    )
  }
}
