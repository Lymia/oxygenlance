use oxygenlance::Warrior;

fn test_str(s: &str) {
    let warrior = Warrior::compile(s).unwrap();
    warrior.run_match(&warrior);
}

#[test]
fn test_bad_loops() {
    test_str("((((.)*0)*10000)*10000)*10000");
    test_str("(({.})*-1)*10000");
    test_str("((((.{.}.)*0)*10000)*10000)*10000");
}
