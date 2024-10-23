use anyhow::{Error, Result};
use clap::Parser;
use oxygenlance::{RoundResult, Warrior, MATCH_COUNT};
use std::{fmt::Write, path::PathBuf};

/// A simple reimplementation of gearlance.
#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    /// The list of BF Joust warriors to run.
    warriors: Vec<PathBuf>,
}

fn main() -> Result<()> {
    let args = Args::parse();

    if args.warriors.len() <= 1 {
        return Err(Error::msg("Must have one or more warrior!"));
    }

    let mut warriors = Vec::new();
    for warrior in args.warriors {
        eprintln!("Compiling {}...", warrior.display());
        let compiled = Warrior::compile(std::fs::read(&warrior)?)?;
        warriors.push((warrior, compiled));
    }

    for (_, warrior_a) in &warriors {
        for (_, warrior_b) in &warriors {
            let result = warrior_a.run_match(warrior_b);
            let mut line = String::new();
            for configuration in 0..2 {
                for i in 0..MATCH_COUNT {
                    line.push(match result.results[configuration][i] {
                        RoundResult::LeftWins => '<',
                        RoundResult::RightWins => '>',
                        RoundResult::Tie => 'X',
                    });
                }
                line.push(' ');
            }
            write!(line, "{}", result.score())?;
            println!("{line}");
        }
    }

    Ok(())
}
