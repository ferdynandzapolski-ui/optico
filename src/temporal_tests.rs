#[cfg(test)]
mod tests {
    use crate::parser::Parser;
    use crate::sema::Sema;

    #[test]
    fn test_temporal_optic_parsing() {
        let input = "I<k> int* x = next<k> 0; int y = prev<k> x;";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        // x and y are the only two top-level declarations
        assert_eq!(prog.decls.len(), 2);
    }

    #[test]
    fn test_multi_clock_guarded_recursion() {
        // Correctly guarded recursion with specific clock
        let input = "rec optic int* f(optic int* o) { return next<k> f(o); }";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        let mut sema = Sema::new();
        sema.check_program(&prog);
    }

    #[test]
    #[should_panic(expected = "Unguarded recursive call")]
    fn test_multi_clock_unguarded_recursion() {
        // Recursive call is under next<k>, but the check should fail if we call it outside any next
        let input = "rec optic int* f(optic int* o) { return f(o); }";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        let mut sema = Sema::new();
        sema.check_program(&prog);
    }

    #[test]
    #[should_panic(expected = "Clock mismatch")]
    fn test_clock_mismatch() {
        let input = "void main() { I<k1> int* x = next<k1> 0; int y = prev<k2> x; }";
        let mut parser = Parser::new(input);
        let prog = parser.parse_program();
        let mut sema = Sema::new();
        sema.check_program(&prog);
    }
}
