fn main() {
    cc::Build::new()
        .file("src/common.c")
        .file("src/gearlance.c")
        .file("src/parser.c")
        .compile("oxygenlance_gearlance");
}