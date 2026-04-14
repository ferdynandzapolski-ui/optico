struct ByteBuf {
  Int capacity;
  Int size;
  Pointer(Char, "Heap", None) data;
}
struct TlsState {
  Int version;
  Int handshake_complete;
  Int session_id;
}
struct HttpState {
  Int method;
  Int status;
  Int keep_alive;
}
struct HttpConnection {
  Named("ByteBuf") in_buffer;
  Named("ByteBuf") out_buffer;
  Named("TlsState") tls;
  Named("HttpState") http;
}
block {
  load conn
}
block {
  load conn
}
block {
  block {
  }
  block {
  }
  block {
  }
  block {
  }
  get (
    load pipeline  )
}
