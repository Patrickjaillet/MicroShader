mod aggressive;
mod budget;
mod callgraph;
mod expr;
mod golfer;
mod inline;
mod lexer;
mod vocab;

#[cfg(feature = "capi")]
mod capi;

pub use aggressive::AggressiveStats;
pub use budget::{estimate_budget, estimate_deflate_bytes, presets, BudgetPreset, BudgetResult};
pub use golfer::{golf, golf_with_options, golf_with_protected_names, AggressiveOptions, GolfResult, GolfStats};
