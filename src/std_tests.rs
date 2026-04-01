#[cfg(test)]
mod tests {
    use crate::parser::Parser;
    use crate::sema::Sema;
    use std::fs;

    fn check_file(path: &str) {
        let content = fs::read_to_string(path).expect(&format!("Could not read {}", path));
        let mut parser = Parser::new(&content);
        let prog = parser.parse_program();
        let mut sema = Sema::new();
        sema.check_program(&prog);
    }

    #[test]
    fn test_prelude_sema() {
        check_file("std/prelude.oco");
    }

    #[test]
    fn test_string_sema() {
        check_file("std/string.oco");
    }

    #[test]
    fn test_list_sema() {
        check_file("std/list.oco");
    }

    #[test]
    fn test_vector_sema() {
        check_file("std/vector.oco");
    }

    #[test]
    fn test_map_sema() {
        check_file("std/map.oco");
    }

    #[test]
    fn test_io_sema() {
        // io.oco has some top-level allocations that might need sema setup
        check_file("std/io.oco");
    }
}
