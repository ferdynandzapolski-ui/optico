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
