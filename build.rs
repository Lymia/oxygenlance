fn main() {
    println!("cargo:rerun-if-changed=src");
    cc::Build::new()
        .file("src/gearlance.c")
        .file("src/parser.c")
        .file("src/rust_callbacks.c")
        .flag("--std=c2x")
        .compile("oxygenlance_gearlance");
}