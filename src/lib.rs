//! oxygenlance is a simple crate to allow the execution of [BF Joust] matches as a Rust library.
//!
//! It is a simple wrapper over [gearlance]'s internals, modified to be thread-safe and integrate
//! better into Rust code.
//!
//! [BF Joust]: http://esolangs.org/wiki/BF_Joust
//! [gearlance]: https://github.com/fis/chainlance

#[allow(unused)]
#[allow(non_camel_case_types)]
#[rustfmt::skip]
mod interface;
mod internal_api;
mod rust_callbacks;

mod api {
    use crate::{interface, internal_api, internal_api::GearlanceCompiledProgram};
    use std::sync::Arc;
    use thiserror::Error;

    /// The minimum tape size that a round may have.
    pub const MINTAPE: usize = interface::MINTAPE as usize;

    /// The maximum tape size that a round may have.
    pub const MAXTAPE: usize = interface::MAXTAPE as usize;

    /// The number of matches that occur in a single round on a single configuration.
    pub const MATCH_COUNT: usize = internal_api::MATCH_COUNT;

    /// The error type for this crate.
    #[derive(Error, Debug)]
    #[non_exhaustive]
    pub enum Error {
        #[error("parse failed: {0}")]
        ParseFailed(&'static str),
    }

    /// A compiled BF Joust warrior.
    #[derive(Clone, Debug)]
    pub struct Warrior {
        compiled: Arc<GearlanceCompiledProgram>,
    }
    impl Warrior {
        /// Compile a BF Joust warrior from its source code.
        pub fn compile(source: &str) -> Result<Warrior, Error> {
            Ok(Warrior { compiled: Arc::new(internal_api::compile_program(source)?) })
        }

        /// Runs a match between two warriors.
        ///
        /// This warrior is set as the left warrior, and the passed warrior is set as the right
        /// warrior.
        pub fn run_match(&self, right: &Warrior) -> MatchResult {
            internal_api::execute_match(&self.compiled, &right.compiled, false)
        }

        /// Runs a match between two warriors, gathering detailed statistics during the execution.
        ///
        /// This is slower than [`run_match`](`Warrior::run_match`), so it should only be used if
        /// you need to gather the detailed statistics for some reason.
        ///
        /// This warrior is set as the left warrior, and the passed warrior is set as the right
        /// warrior.
        pub fn run_match_detailed(&self, right: &Warrior) -> MatchResult {
            internal_api::execute_match(&self.compiled, &right.compiled, false)
        }
    }

    /// The result of a single round between two BF Joust warriors.
    #[derive(Copy, Clone, Debug, Ord, PartialOrd, Eq, PartialEq, Hash)]
    pub enum RoundResult {
        /// The left warrior wins.
        LeftWins,

        /// The right warrior wins.
        RightWins,

        /// The round is tied.
        Tie,
    }
    impl RoundResult {
        /// Returns the score that this result would get for the left warrior.
        pub fn score(&self) -> i32 {
            match self {
                RoundResult::LeftWins => 1,
                RoundResult::RightWins => -1,
                RoundResult::Tie => 0,
            }
        }

        /// Reverses the perspective of the right and left warriors.
        pub fn reverse(&self) -> RoundResult {
            match self {
                RoundResult::LeftWins => RoundResult::RightWins,
                RoundResult::RightWins => RoundResult::LeftWins,
                RoundResult::Tie => RoundResult::Tie,
            }
        }
    }

    /// The result of a match between two BF Joust warriors.
    #[derive(Copy, Clone, Debug, Hash)]
    #[non_exhaustive]
    pub struct MatchResult {
        /// The results of all matches between the warriors.
        ///
        /// The first array is for the normal polarity configuration, and the second array is for
        /// the inverted polarity configuration.
        pub results: [[RoundResult; MATCH_COUNT]; 2],

        /// The total number of cycles executed across all matches.
        pub total_cycles: u32,

        /// Detailed statistics for the match.
        ///
        /// Only included if this was created using [`Warrior::run_match_detailed`].
        pub detailed_statistics: Option<MatchDetailedStatistics>,
    }
    impl MatchResult {
        /// Returns the score that this result would get for the left warrior.
        pub fn score(&self) -> i32 {
            self.results
                .iter()
                .map(|x| x.iter().map(|x| x.score()).sum::<i32>())
                .sum()
        }

        /// Reverses the perspective of the right and left warriors.
        ///
        /// This removes the detailed statistics, as those cannot be trivially reversed without
        /// rerunning the match.
        pub fn reverse(&self) -> MatchResult {
            let mut results = [[RoundResult::Tie; MATCH_COUNT]; 2];
            for configuration in 0..2 {
                for i in 0..MATCH_COUNT {
                    results[configuration][i] = self.results[configuration][i].reverse();
                }
            }
            MatchResult {
                results,
                total_cycles: self.total_cycles,
                detailed_statistics: self.detailed_statistics,
            }
        }
    }

    /// Detailed statistics for a match between two BF Joust warriors.
    ///
    /// This is useful for behavior visualizations and similar programs.
    #[derive(Copy, Clone, Debug, Hash)]
    #[non_exhaustive]
    pub struct MatchDetailedStatistics {
        /// The maximum absolute distance from 0 that each cell reaches on average when the program
        /// leaves a cell. This helps show its decoy layout.
        ///
        /// The first array is the left warrior, and the second array is the right warrior.
        pub tape_max: [[u8; MAXTAPE]; 2],

        /// The number of cycles each program spends on each cell.
        ///
        /// The first array is the left warrior, and the second array is the right warrior.
        pub heat_position: [[u32; MAXTAPE]; 2],
    }
}

pub use api::*;
