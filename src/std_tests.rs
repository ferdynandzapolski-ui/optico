#[cfg(test)]
mod tests {
    use crate::parser::Parser;
    use crate::sema::Sema;
    use std::fs;

    fn check_file(path: &str) {
        let mut full_content = fs::read_to_string("std/prelude.oco").unwrap();
        if path != "std/prelude.oco" {
            full_content.push_str(&fs::read_to_string(path).expect(&format!("Could not read {}", path)));
        }
        let mut parser = Parser::new(&full_content);
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
    fn test_ptr_vector_sema() {
        check_file("std/ptr_vector.oco");
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

    #[test]
    fn test_ir_node_sema() {
        let mut full_content = fs::read_to_string("std/prelude.oco").unwrap();
        full_content.push_str(&fs::read_to_string("std/string.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/ptr_vector.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/ir/symbol.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/ir/type.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/ir/node.oco").unwrap());
        let mut parser = Parser::new(&full_content);
        let prog = parser.parse_program();
        let mut sema = Sema::new();
        sema.check_program(&prog);
    }

    #[test]
    fn test_ir_symbol_sema() {
        let mut full_content = fs::read_to_string("std/prelude.oco").unwrap();
        full_content.push_str(&fs::read_to_string("std/ptr_vector.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/ir/symbol.oco").unwrap());
        let mut parser = Parser::new(&full_content);
        let prog = parser.parse_program();
        let mut sema = Sema::new();
        sema.check_program(&prog);
    }

    #[test]
    fn test_ir_type_sema() {
        let mut full_content = fs::read_to_string("std/prelude.oco").unwrap();
        full_content.push_str(&fs::read_to_string("std/ptr_vector.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/ir/type.oco").unwrap());
        let mut parser = Parser::new(&full_content);
        let prog = parser.parse_program();
        let mut sema = Sema::new();
        sema.check_program(&prog);
    }

    #[test]
    fn test_compiler_lexer_sema() {
        let mut full_content = fs::read_to_string("std/prelude.oco").unwrap();
        full_content.push_str(&fs::read_to_string("std/string.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/compiler/lexer.oco").unwrap());
        let mut parser = Parser::new(&full_content);
        let prog = parser.parse_program();
        let mut sema = Sema::new();
        sema.check_program(&prog);
    }

    #[test]
    fn test_compiler_parser_sema() {
        // Need to include ir nodes for parser
        let mut full_content = fs::read_to_string("std/prelude.oco").unwrap();
        full_content.push_str(&fs::read_to_string("std/string.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/ptr_vector.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/ir/symbol.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/ir/type.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/ir/node.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/compiler/lexer.oco").unwrap());
        full_content.push_str(&fs::read_to_string("std/compiler/parser.oco").unwrap());
        let mut parser = Parser::new(&full_content);
        let prog = parser.parse_program();
        let mut sema = Sema::new();
        sema.check_program(&prog);
    }
}
