fn main() {
    cc::Build::new()
        .file("src/gearlance.c")
        .file("src/parser.c")
        .file("src/rust_callbacks.c")
        .compile("oxygenlance_gearlance");
}