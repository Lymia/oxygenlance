use anyhow::{Error, Result};
use clap::Parser;
use oxygenlance::{RoundResult, Warrior, MATCH_COUNT};
use rayon::prelude::*;
use std::{fmt::Write, path::PathBuf};

/// A simple reimplementation of gearlance.
#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    /// Run every matchup, and in a very predictable format. This is used to test oxygenlance.
    #[arg(long)]
    reference_mode: bool,

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
        warriors.push((warrior.file_name().unwrap().to_string_lossy().to_string(), compiled));
    }

    let max_name = warriors.iter().map(|x| x.0.len()).max().unwrap();

    let results: Vec<_> = warriors
        .par_iter()
        .enumerate()
        .flat_map(|(i, (wa_name, wa))| {
            let target = if args.reference_mode {
                &warriors
            } else {
                &warriors[i + 1..]
            };
            target
                .par_iter()
                .map(|(wb_name, wb)| (wa_name.clone(), wb_name.clone(), wa.run_match(wb)))
        })
        .collect();
    for (wa_name, wb_name, result) in results {
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
        if args.reference_mode {
            println!("{line}");
        } else {
            println!("{wa_name:width$   } vs {wb_name:width$}: {line}", width = max_name);
        }
    }

    Ok(())
}
