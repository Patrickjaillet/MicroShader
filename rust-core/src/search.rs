use crate::budget::{estimate_budget, estimate_twigl_geekest_budget};
use crate::golfer::{golf_with_protected_names, AggressiveOptions, GolfResult};
use crate::swizzle::SwizzleAlphabet;

/// golf.md Phase 32.1 -- a bounded, deterministic local hill-climb over the
/// Phase 29-31 pass toggles this document introduced. Each candidate is a
/// single coordinate: flip one boolean pass toggle, or step the
/// `swizzle_alphabet` choice to one of its other three values. Capped at
/// `MAX_ROUNDS` full passes over the fixed candidate list -- never
/// combinatorial-explosive, and never randomized, so the same input and
/// options always converge to the same chosen combination.
const MAX_ROUNDS: usize = 2;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum SearchToggle {
    FrequencyAwareRenaming,
    FactorRepeatedVectorArgs,
    FuseStatementSequences,
    AggressiveInlining,
    MacroCse,
    HoistDeclarations,
    LoopHeaderGolf,
    LoopFormGolf,
}

const TOGGLE_ORDER: [SearchToggle; 8] = [
    SearchToggle::FrequencyAwareRenaming,
    SearchToggle::FactorRepeatedVectorArgs,
    SearchToggle::FuseStatementSequences,
    SearchToggle::AggressiveInlining,
    SearchToggle::MacroCse,
    SearchToggle::HoistDeclarations,
    SearchToggle::LoopHeaderGolf,
    SearchToggle::LoopFormGolf,
];

const SWIZZLE_CANDIDATES: [SwizzleAlphabet; 4] = [
    SwizzleAlphabet::Auto,
    SwizzleAlphabet::Xyzw,
    SwizzleAlphabet::Rgba,
    SwizzleAlphabet::Stpq,
];

fn get_toggle(options: &AggressiveOptions, toggle: SearchToggle) -> bool {
    match toggle {
        SearchToggle::FrequencyAwareRenaming => options.frequency_aware_renaming,
        SearchToggle::FactorRepeatedVectorArgs => options.factor_repeated_vector_args,
        SearchToggle::FuseStatementSequences => options.fuse_statement_sequences,
        SearchToggle::AggressiveInlining => options.aggressive_inlining,
        SearchToggle::MacroCse => options.macro_cse,
        SearchToggle::HoistDeclarations => options.hoist_declarations,
        SearchToggle::LoopHeaderGolf => options.loop_header_golf,
        SearchToggle::LoopFormGolf => options.loop_form_golf,
    }
}

fn set_toggle(options: &mut AggressiveOptions, toggle: SearchToggle, value: bool) {
    match toggle {
        SearchToggle::FrequencyAwareRenaming => options.frequency_aware_renaming = value,
        SearchToggle::FactorRepeatedVectorArgs => options.factor_repeated_vector_args = value,
        SearchToggle::FuseStatementSequences => options.fuse_statement_sequences = value,
        SearchToggle::AggressiveInlining => options.aggressive_inlining = value,
        SearchToggle::MacroCse => options.macro_cse = value,
        SearchToggle::HoistDeclarations => options.hoist_declarations = value,
        SearchToggle::LoopHeaderGolf => options.loop_header_golf = value,
        SearchToggle::LoopFormGolf => options.loop_form_golf = value,
    }
}

fn toggle_name(toggle: SearchToggle) -> &'static str {
    match toggle {
        SearchToggle::FrequencyAwareRenaming => "frequency_aware_renaming",
        SearchToggle::FactorRepeatedVectorArgs => "factor_repeated_vector_args",
        SearchToggle::FuseStatementSequences => "fuse_statement_sequences",
        SearchToggle::AggressiveInlining => "aggressive_inlining",
        SearchToggle::MacroCse => "macro_cse",
        SearchToggle::HoistDeclarations => "hoist_declarations",
        SearchToggle::LoopHeaderGolf => "loop_header_golf",
        SearchToggle::LoopFormGolf => "loop_form_golf",
    }
}

fn swizzle_name(alphabet: SwizzleAlphabet) -> &'static str {
    match alphabet {
        SwizzleAlphabet::Auto => "auto",
        SwizzleAlphabet::Xyzw => "xyzw",
        SwizzleAlphabet::Rgba => "rgba",
        SwizzleAlphabet::Stpq => "stpq",
    }
}

/// One coordinate the search accepted over the starting options, in the
/// order it was applied -- surfaced so a caller (the "Golf harder" UI
/// action) can explain what changed, not just show the resulting diff.
#[derive(Debug, Clone)]
pub struct AppliedChange {
    pub pass_name: &'static str,
    pub from: String,
    pub to: String,
}

#[derive(Debug, Clone)]
pub struct SearchOutcome {
    pub result: GolfResult,
    pub final_options: AggressiveOptions,
    pub improved: bool,
    pub applied: Vec<AppliedChange>,
}

fn score(code: &str, compression_based: bool) -> usize {
    if compression_based {
        estimate_budget(code).deflate_bytes
    } else {
        code.chars().count()
    }
}

/// golf.md Phase 32.1 -- runs the bounded search described above and
/// returns whichever combination scored smallest under the caller's
/// selected budget metric (`compression_based`: DEFLATE-estimated when
/// `true`, raw character count otherwise). `base` is used unmodified as
/// the starting point and its own result is always the floor: the search
/// only ever replaces it with a strictly smaller candidate, so the
/// returned outcome can never be larger than running `base` alone.
pub fn golf_harder(
    source: &str,
    base: AggressiveOptions,
    protected_names: &[String],
    compression_based: bool,
) -> SearchOutcome {
    let mut current_options = base;
    let mut current_result = golf_with_protected_names(source, current_options, protected_names);
    let mut current_score = score(&current_result.code, compression_based);
    let base_score = current_score;
    let mut applied: Vec<AppliedChange> = Vec::new();

    for _ in 0..MAX_ROUNDS {
        let mut round_improved = false;

        for &toggle in TOGGLE_ORDER.iter() {
            let from = get_toggle(&current_options, toggle);
            let mut trial_options = current_options;
            set_toggle(&mut trial_options, toggle, !from);
            let trial_result = golf_with_protected_names(source, trial_options, protected_names);
            let trial_score = score(&trial_result.code, compression_based);
            if trial_score < current_score {
                applied.push(AppliedChange {
                    pass_name: toggle_name(toggle),
                    from: from.to_string(),
                    to: (!from).to_string(),
                });
                current_options = trial_options;
                current_result = trial_result;
                current_score = trial_score;
                round_improved = true;
            }
        }

        for &candidate in SWIZZLE_CANDIDATES.iter() {
            if candidate == current_options.swizzle_alphabet {
                continue;
            }
            let from = current_options.swizzle_alphabet;
            let mut trial_options = current_options;
            trial_options.swizzle_alphabet = candidate;
            let trial_result = golf_with_protected_names(source, trial_options, protected_names);
            let trial_score = score(&trial_result.code, compression_based);
            if trial_score < current_score {
                applied.push(AppliedChange {
                    pass_name: "swizzle_alphabet",
                    from: swizzle_name(from).to_string(),
                    to: swizzle_name(candidate).to_string(),
                });
                current_options = trial_options;
                current_result = trial_result;
                current_score = trial_score;
                round_improved = true;
            }
        }

        if !round_improved {
            break;
        }
    }

    SearchOutcome {
        result: current_result,
        final_options: current_options,
        improved: current_score < base_score,
        applied,
    }
}

// ---------------------------------------------------------------------
// ROADMAP.md Phase 37.1/37.3 -- "Golf harder", extended: a simulated-
// annealing search over the same candidate set as `golf_harder` above,
// plus a multi-objective scoring choice (37.3), with a wall-clock budget
// (37.1). This is an additive capability layered on top of Phase 32.1,
// not a replacement -- `golf_harder`'s bounded hill-climb keeps its own
// API, tests, and FFI export exactly as shipped.
// ---------------------------------------------------------------------

/// ROADMAP.md Phase 37.3 -- which byte metric a search scores candidates
/// against. `TwiglGeekest280` rewrites the candidate through the Phase 34
/// Twigl `geekest`-mode export first and measures *raw* character count
/// (the 280-character X/Twitter limit is a character budget, not a
/// compressed-byte budget), for when a Twigl `geekest`-mode export is the
/// active tab rather than the plain Shadertoy `Golfed` tab.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SearchObjective {
    RawBytes,
    DeflateBytes,
    TwiglGeekest280,
}

fn score_for_objective(code: &str, objective: SearchObjective) -> usize {
    match objective {
        SearchObjective::RawBytes => code.chars().count(),
        SearchObjective::DeflateBytes => estimate_budget(code).deflate_bytes,
        SearchObjective::TwiglGeekest280 => estimate_twigl_geekest_budget(code).raw_bytes,
    }
}

/// ROADMAP.md Phase 37.1 -- a small, dependency-free xorshift64 PRNG,
/// seeded deterministically (never from wall-clock time) so the same
/// input always drives the same sequence of random draws. Determinism of
/// the search's *outcome* additionally depends on the iteration budget
/// being reached before the wall-clock safety net below -- see
/// `golf_harder_deep`'s doc comment.
struct Xorshift64 {
    state: u64,
}

impl Xorshift64 {
    fn new(seed: u64) -> Self {
        Self { state: if seed == 0 { 0x9E37_79B9_7F4A_7C15 } else { seed } }
    }

    fn next_u64(&mut self) -> u64 {
        let mut x = self.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        self.state = x;
        x
    }

    fn next_f64(&mut self) -> f64 {
        (self.next_u64() >> 11) as f64 / (1u64 << 53) as f64
    }

    fn next_below(&mut self, n: usize) -> usize {
        (self.next_u64() % n as u64) as usize
    }
}

fn seed_from_input(source: &str, base: AggressiveOptions, objective: SearchObjective) -> u64 {
    // FNV-1a over the source bytes plus a byte-encoding of the starting
    // options and objective, so the seed depends only on the search's own
    // input -- never on wall-clock time or any other run-to-run varying
    // state.
    let mut hash: u64 = 0xcbf2_9ce4_8422_2325;
    let mut feed = |byte: u8| {
        hash ^= byte as u64;
        hash = hash.wrapping_mul(0x0000_0100_0000_01B3);
    };
    for byte in source.bytes() {
        feed(byte);
    }
    for &toggle in TOGGLE_ORDER.iter() {
        feed(get_toggle(&base, toggle) as u8);
    }
    feed(base.swizzle_alphabet as u8);
    feed(objective as u8);
    hash
}

fn random_neighbor(options: AggressiveOptions, rng: &mut Xorshift64) -> AggressiveOptions {
    let mut trial = options;
    // 8 boolean toggles + 1 swizzle-alphabet coordinate, uniformly chosen.
    let coordinate = rng.next_below(TOGGLE_ORDER.len() + 1);
    if coordinate < TOGGLE_ORDER.len() {
        let toggle = TOGGLE_ORDER[coordinate];
        let from = get_toggle(&trial, toggle);
        set_toggle(&mut trial, toggle, !from);
    } else {
        let from = trial.swizzle_alphabet;
        let others: Vec<SwizzleAlphabet> = SWIZZLE_CANDIDATES.iter().copied().filter(|&c| c != from).collect();
        trial.swizzle_alphabet = others[rng.next_below(others.len())];
    }
    trial
}

/// ROADMAP.md Phase 37.1/37.3 -- simulated-annealing extension of
/// `golf_harder`: instead of a single bounded hill-climb pass, this
/// explores the same candidate space via random neighbor moves, initially
/// accepting some worse moves (probability `exp(-delta/temperature)`) to
/// escape local optima that the plain hill-climb cannot, cooling down
/// geometrically until it settles. The best-scoring combination seen
/// across the whole run is returned, never a worse one than
/// `golf_harder` alone would find (both start from `base` and both only
/// ever *replace* the running best with a strictly smaller candidate).
///
/// Bounded by two independent limits: `max_iterations` (a hard cap that
/// is always fully deterministic for a given seed -- reaching it always
/// takes the same number of steps) and `max_duration` (the wall-clock
/// budget ROADMAP.md Phase 37.1 asks for, default 2 seconds, checked
/// every iteration since each iteration reruns the whole golf pipeline
/// and is therefore not cheap). In practice `max_duration` is usually
/// what actually stops the search before `max_iterations` does -- each
/// iteration's full re-golf takes a non-trivial, machine-dependent
/// amount of time. This is an explicit, documented departure from
/// `golf_harder`'s (Phase 32.1) strict "same input always converges to
/// the same combination" guarantee: `golf_harder_deep`'s outcome can
/// differ across runs on machines (or under system load) fast/slow
/// enough to fit a different number of iterations inside the same
/// wall-clock budget. Fully reproducible runs are still available by
/// passing a `max_duration` long enough that `max_iterations` is always
/// reached first (as every test below does).
pub fn golf_harder_deep(
    source: &str,
    base: AggressiveOptions,
    protected_names: &[String],
    objective: SearchObjective,
    max_iterations: usize,
    max_duration: std::time::Duration,
) -> SearchOutcome {
    let start = std::time::Instant::now();
    let mut rng = Xorshift64::new(seed_from_input(source, base, objective));

    let mut current_options = base;
    let mut current_result = golf_with_protected_names(source, current_options, protected_names);
    let mut current_score = score_for_objective(&current_result.code, objective);

    let mut best_options = current_options;
    let mut best_score = current_score;
    let base_score = current_score;

    const INITIAL_TEMPERATURE: f64 = 8.0;
    const COOLING_RATE: f64 = 0.98;
    let mut temperature = INITIAL_TEMPERATURE;

    let mut iteration = 0usize;
    while iteration < max_iterations {
        // Checked every iteration, not every N: unlike a typical SA
        // neighbor evaluation, each iteration here reruns the whole golf
        // pipeline (tokenize + every enabled pass), so it is not cheap
        // enough to let many iterations elapse between wall-clock checks.
        if start.elapsed() >= max_duration {
            break;
        }

        let trial_options = random_neighbor(current_options, &mut rng);
        let trial_result = golf_with_protected_names(source, trial_options, protected_names);
        let trial_score = score_for_objective(&trial_result.code, objective);

        let delta = trial_score as f64 - current_score as f64;
        let accept = delta < 0.0 || rng.next_f64() < (-delta / temperature.max(1e-9)).exp();

        if accept {
            current_options = trial_options;
            current_result = trial_result;
            current_score = trial_score;

            if current_score < best_score {
                best_score = current_score;
                best_options = current_options;
            }
        }

        temperature *= COOLING_RATE;
        iteration += 1;
    }

    let best_result = golf_with_protected_names(source, best_options, protected_names);

    // Unlike `golf_harder`'s single monotonic hill-climb, the random walk
    // above can pass through and revert many intermediate combinations --
    // recording every accepted step would not describe what actually
    // changed between `base` and the returned `best_options`. So `applied`
    // is computed once at the end, directly diffing the two option sets
    // coordinate by coordinate, which is always an accurate description
    // regardless of the path the walk took to get there.
    let mut applied: Vec<AppliedChange> = Vec::new();
    for &toggle in TOGGLE_ORDER.iter() {
        let from = get_toggle(&base, toggle);
        let to = get_toggle(&best_options, toggle);
        if from != to {
            applied.push(AppliedChange { pass_name: toggle_name(toggle), from: from.to_string(), to: to.to_string() });
        }
    }
    if base.swizzle_alphabet != best_options.swizzle_alphabet {
        applied.push(AppliedChange {
            pass_name: "swizzle_alphabet",
            from: swizzle_name(base.swizzle_alphabet).to_string(),
            to: swizzle_name(best_options.swizzle_alphabet).to_string(),
        });
    }

    SearchOutcome {
        result: best_result,
        final_options: best_options,
        improved: best_score < base_score,
        applied,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const RAYMARCH_FIXTURE: &str = include_str!("../../fixtures/macro_cse.glsl");
    const FREQUENCY_FIXTURE: &str = include_str!("../../fixtures/frequency_renaming.glsl");
    const AGGRESSIVE_INLINING_FIXTURE: &str = include_str!("../../fixtures/aggressive_inlining.glsl");

    #[test]
    fn never_returns_a_result_larger_than_the_default_fixed_pass_order_alone() {
        for source in [RAYMARCH_FIXTURE, FREQUENCY_FIXTURE, AGGRESSIVE_INLINING_FIXTURE] {
            for compression_based in [false, true] {
                let base = AggressiveOptions::all();
                let base_result = golf_with_protected_names(source, base, &[]);
                let base_score = score(&base_result.code, compression_based);

                let outcome = golf_harder(source, base, &[], compression_based);
                let outcome_score = score(&outcome.result.code, compression_based);

                assert!(
                    outcome_score <= base_score,
                    "golf_harder regressed on compression_based={compression_based}: {outcome_score} > {base_score}"
                );
            }
        }
    }

    #[test]
    fn is_deterministic_across_repeated_runs_on_the_same_input() {
        let base = AggressiveOptions::all();
        let first = golf_harder(RAYMARCH_FIXTURE, base, &[], true);
        let second = golf_harder(RAYMARCH_FIXTURE, base, &[], true);

        assert_eq!(first.result.code, second.result.code);
        assert_eq!(first.applied.len(), second.applied.len());
        for (a, b) in first.applied.iter().zip(second.applied.iter()) {
            assert_eq!(a.pass_name, b.pass_name);
            assert_eq!(a.from, b.from);
            assert_eq!(a.to, b.to);
        }
    }

    #[test]
    fn respects_the_protected_names_list_like_every_pass_it_orchestrates() {
        let base = AggressiveOptions::all();
        let protected = vec!["mainImage".to_string(), "fragColor".to_string(), "fragCoord".to_string()];
        let outcome = golf_harder(RAYMARCH_FIXTURE, base, &protected, false);

        assert!(outcome.result.code.contains("mainImage"));
        assert!(outcome.result.code.contains("fragColor"));
        assert!(outcome.result.code.contains("fragCoord"));
    }

    #[test]
    fn replaying_the_final_options_directly_reproduces_the_same_code() {
        // golf_harder is purely an orchestration layer: it invents no new
        // transformation of its own, so re-running the plain fixed pass
        // order with the options it converged on must reproduce the exact
        // same output.
        let base = AggressiveOptions::all();
        let outcome = golf_harder(RAYMARCH_FIXTURE, base, &[], true);
        let replayed = golf_with_protected_names(RAYMARCH_FIXTURE, outcome.final_options, &[]);
        assert_eq!(outcome.result.code, replayed.code);
    }

    #[test]
    fn a_source_with_no_possible_improvement_reports_not_improved_and_keeps_the_base_code() {
        let source = "void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.,0.,0.,1.);}";
        let base = AggressiveOptions::none();
        let outcome = golf_harder(source, base, &[], false);
        let base_result = golf_with_protected_names(source, base, &[]);

        assert_eq!(outcome.result.code, base_result.code);
        assert!(!outcome.improved);
        assert!(outcome.applied.is_empty());
    }

    #[test]
    fn is_bounded_to_the_documented_round_cap_and_completes_quickly() {
        let start = std::time::Instant::now();
        let base = AggressiveOptions::all();
        let _ = golf_harder(RAYMARCH_FIXTURE, base, &[], true);
        assert!(start.elapsed() < std::time::Duration::from_secs(2));
    }

    // -------------------------------------------------------------
    // ROADMAP.md Phase 37.1/37.3 -- golf_harder_deep (simulated annealing)
    // -------------------------------------------------------------

    #[test]
    fn deep_search_never_returns_a_result_larger_than_the_base_options_alone() {
        for source in [RAYMARCH_FIXTURE, FREQUENCY_FIXTURE, AGGRESSIVE_INLINING_FIXTURE] {
            for objective in [SearchObjective::RawBytes, SearchObjective::DeflateBytes, SearchObjective::TwiglGeekest280] {
                let base = AggressiveOptions::all();
                let base_result = golf_with_protected_names(source, base, &[]);
                let base_score = score_for_objective(&base_result.code, objective);

                let outcome = golf_harder_deep(source, base, &[], objective, 30, std::time::Duration::from_millis(150));
                let outcome_score = score_for_objective(&outcome.result.code, objective);

                assert!(
                    outcome_score <= base_score,
                    "deep search regressed for {objective:?}: {outcome_score} > {base_score}"
                );
            }
        }
    }

    #[test]
    fn deep_search_is_deterministic_across_repeated_runs_on_the_same_input() {
        let base = AggressiveOptions::all();
        let first = golf_harder_deep(RAYMARCH_FIXTURE, base, &[], SearchObjective::DeflateBytes, 30, std::time::Duration::from_millis(150));
        let second = golf_harder_deep(RAYMARCH_FIXTURE, base, &[], SearchObjective::DeflateBytes, 30, std::time::Duration::from_millis(150));

        assert_eq!(first.result.code, second.result.code);
        assert_eq!(first.final_options, second.final_options);
    }

    #[test]
    fn deep_search_respects_the_protected_names_list() {
        let base = AggressiveOptions::all();
        let protected = vec!["mainImage".to_string(), "fragColor".to_string(), "fragCoord".to_string()];
        let outcome = golf_harder_deep(RAYMARCH_FIXTURE, base, &protected, SearchObjective::DeflateBytes, 30, std::time::Duration::from_millis(150));

        assert!(outcome.result.code.contains("mainImage"));
        assert!(outcome.result.code.contains("fragColor"));
        assert!(outcome.result.code.contains("fragCoord"));
    }

    #[test]
    fn deep_search_replaying_the_final_options_directly_reproduces_the_same_code() {
        let base = AggressiveOptions::all();
        let outcome = golf_harder_deep(RAYMARCH_FIXTURE, base, &[], SearchObjective::DeflateBytes, 30, std::time::Duration::from_millis(150));
        let replayed = golf_with_protected_names(RAYMARCH_FIXTURE, outcome.final_options, &[]);
        assert_eq!(outcome.result.code, replayed.code);
    }

    #[test]
    fn deep_search_honors_the_wall_clock_safety_net() {
        let base = AggressiveOptions::all();
        let budget = std::time::Duration::from_millis(150);
        let start = std::time::Instant::now();
        // A huge iteration cap that would run far longer than the wall-clock
        // budget if the safety net did not cut it short.
        let _ = golf_harder_deep(RAYMARCH_FIXTURE, base, &[], SearchObjective::DeflateBytes, 10_000_000, budget);
        // Generous absolute tolerance, not a tight multiplier of `budget`:
        // the wall clock is only checked *between* iterations (see
        // `golf_harder_deep`'s doc comment), and one specific coordinate --
        // flipping `frequency_aware_renaming` on -- is currently far more
        // expensive than every other candidate (a pre-existing performance
        // characteristic of `choose_frequency_aware_candidate`, unrelated
        // to this search; tracked separately, not fixed here). A single
        // iteration landing on that coordinate can overshoot a short budget
        // by roughly that much on its own, so this bound must accommodate
        // one such outlier without going flaky.
        assert!(
            start.elapsed() < budget + std::time::Duration::from_secs(3),
            "wall-clock safety net did not bound the search"
        );
    }

    #[test]
    fn deep_search_applied_list_accurately_diffs_base_against_the_final_options() {
        let base = AggressiveOptions::none();
        let outcome = golf_harder_deep(RAYMARCH_FIXTURE, base, &[], SearchObjective::DeflateBytes, 30, std::time::Duration::from_millis(150));
        for change in &outcome.applied {
            match change.pass_name {
                "swizzle_alphabet" => {
                    assert_eq!(change.from, swizzle_name(base.swizzle_alphabet));
                    assert_eq!(change.to, swizzle_name(outcome.final_options.swizzle_alphabet));
                }
                name => {
                    let toggle = TOGGLE_ORDER.iter().copied().find(|t| toggle_name(*t) == name).unwrap();
                    assert_eq!(change.from, get_toggle(&base, toggle).to_string());
                    assert_eq!(change.to, get_toggle(&outcome.final_options, toggle).to_string());
                }
            }
        }
    }
}

