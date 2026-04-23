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
