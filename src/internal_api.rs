use crate::{
    interface::{
        gearlance_compile, gearlance_compile_input, gearlance_compiled_program, gearlance_execute,
        gearlance_execute_input, gearlance_execute_result, gearlance_free_program, MAXTAPE,
        MINTAPE,
    },
    Error, MatchDetailedStatistics, MatchResult, RoundResult,
};
use std::{
    ffi::CStr,
    fmt::{Debug, Formatter},
};

pub struct GearlanceCompiledProgram(*mut gearlance_compiled_program);
impl Drop for GearlanceCompiledProgram {
    fn drop(&mut self) {
        unsafe {
            gearlance_free_program(self.0);
        }
    }
}
unsafe impl Sync for GearlanceCompiledProgram {}
unsafe impl Send for GearlanceCompiledProgram {}
impl Debug for GearlanceCompiledProgram {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "[compiled program at 0x{:x}]", self.0 as usize)
    }
}
impl PartialEq for GearlanceCompiledProgram {
    fn eq(&self, other: &Self) -> bool {
        self.0 as usize == other.0 as usize
    }
}
impl Eq for GearlanceCompiledProgram {}

pub fn compile_program(input: &[u8]) -> Result<GearlanceCompiledProgram, Error> {
    unsafe {
        let input = gearlance_compile_input { data: input.as_ptr() as *mut _, length: input.len() };
        let result = gearlance_compile(input);
        if result.error_encountered {
            Err(Error::ParseFailed(CStr::from_ptr(result.err_msg).to_str().unwrap()))
        } else {
            Ok(GearlanceCompiledProgram(result.program))
        }
    }
}

pub const MATCH_COUNT: usize = (MAXTAPE - MINTAPE + 1) as usize;

pub fn execute_match(
    program_a: &GearlanceCompiledProgram,
    program_b: &GearlanceCompiledProgram,
    extra_stats: bool,
) -> MatchResult {
    let input = gearlance_execute_input {
        program_left: program_a.0,
        program_right: program_b.0,
        track_stats: extra_stats,
    };
    let mut result = gearlance_execute_result {
        scores: [[0; (MAXTAPE + 1) as usize]; 2],
        cycles: 0,
        tape_max: [[0; MAXTAPE as usize]; 2],
        heat_position: [[0; MAXTAPE as usize]; 2],
    };
    unsafe {
        gearlance_execute(input, &mut result);
    }

    // transcribe scores
    let mut scores = MatchResult {
        results: [[RoundResult::Tie; MATCH_COUNT]; 2],
        total_cycles: result.cycles as u32,
        detailed_statistics: None,
    };
    for configuration in 0..2 {
        for i in 0..MATCH_COUNT {
            scores.results[configuration][i] =
                match result.scores[configuration][MINTAPE as usize + i] {
                    -1 => RoundResult::RightWins,
                    0 => RoundResult::Tie,
                    1 => RoundResult::LeftWins,
                    _ => unreachable!("Invalid score received?"),
                };
        }
    }
    if extra_stats {
        scores.detailed_statistics = Some(MatchDetailedStatistics {
            tape_max: result.tape_max,
            heat_position: result.heat_position,
        });
    }

    scores
}
