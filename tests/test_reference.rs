use oxygenlance::{MatchResult, RoundResult, Warrior, MATCH_COUNT};
use rayon::prelude::*;
use std::{fmt::Write, path::PathBuf};

const INDEX: &str = include_str!("warriors.idx");
const REFERENCE: &str = include_str!("reference.data");

fn load_warriors() -> Vec<Warrior> {
    let base_path = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("tests");

    let names: Vec<_> = INDEX.split("\n").filter(|x| !x.is_empty()).collect();
    names
        .into_par_iter()
        .map(|warrior| {
            let warrior_file = base_path.join(warrior);
            Warrior::compile(std::fs::read(warrior_file).unwrap()).unwrap()
        })
        .collect()
}

fn push_result_to_string(str: &mut String, result: MatchResult) {
    for configuration in 0..2 {
        for i in 0..MATCH_COUNT {
            str.push(match result.results[configuration][i] {
                RoundResult::LeftWins => '<',
                RoundResult::RightWins => '>',
                RoundResult::Tie => 'X',
            });
        }
        str.push(' ');
    }
    write!(str, "{}", result.score()).unwrap();
    str.push('\n');
}

#[test]
fn single_threaded_test() {
    let warriors = load_warriors();
    let mut str = String::new();
    for warrior_a in &warriors {
        for warrior_b in &warriors {
            let match_result = warrior_a.run_match(warrior_b);
            push_result_to_string(&mut str, match_result);
        }
    }
    assert_eq!(str, REFERENCE);
}

#[test]
fn multi_threaded_test() {
    let warriors = load_warriors();

    let mut matchups = Vec::new();
    for warrior_a in &warriors {
        for warrior_b in &warriors {
            matchups.push((warrior_a, warrior_b));
        }
    }
    let results: Vec<_> = matchups
        .into_par_iter()
        .map(|(a, b)| a.run_match(b))
        .collect();

    let mut str = String::new();
    for result in results {
        push_result_to_string(&mut str, result);
    }

    assert_eq!(str, REFERENCE);
}

#[test]
fn detailed_test() {
    let warriors = load_warriors();
    let mut str = String::new();
    for warrior_a in &warriors {
        for warrior_b in &warriors {
            let match_result = warrior_a.run_match_detailed(warrior_b);
            push_result_to_string(&mut str, match_result);
        }
    }
    assert_eq!(str, REFERENCE);
}
