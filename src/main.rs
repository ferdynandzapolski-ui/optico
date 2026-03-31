use app::parser::Parser;
use app::sema::Sema;
use app::cir::CIRLowerer;
use app::codegen::CodeGenerator;

fn main() {
    let example_std = "
        struct Point { int x; int y; }
        resource Point console = alloc<Point>(1, 2);

        void main() {
            co Point* p = console;
            int val = p->x;
        }
    ";

    let mut parser = Parser::new(example_std);
    let prog = parser.parse_program();

    let mut sema = Sema::new();
    sema.check_program(&prog);

    let cir = CIRLowerer::lower_program(&prog);
    let code = CodeGenerator::generate(&cir);

    println!("Example STD CIR:\n{}", code);
}
