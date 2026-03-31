use app::parser::Parser;
use app::sema::Sema;
use app::cir::CIRLowerer;
use app::codegen::CodeGenerator;

fn main() {
    let example_a_sim = "
        rec optic int* list_sum(optic ListNode* node) {
            node->data | (next list_sum(node->next))
        }
    ";

    // Wait, node->data is Access(Var(node), data)
    // next is a prefix operator, but I'm calling it in a block.

    let example_c = "
        void main() {
            resource File* f = alloc<File>(0);
            co char* first = f;
            first := 65;
        }
    ";

    let mut parser = Parser::new(example_c);
    let prog = parser.parse_program();

    let mut sema = Sema::new();
    sema.check_program(&prog);

    let cir = CIRLowerer::lower_program(&prog);
    let code = CodeGenerator::generate(&cir);

    println!("Example C CIR:\n{}", code);
}
