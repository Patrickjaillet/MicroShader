mod aggressive;
mod budget;
mod callgraph;
mod deflate;
mod expr;
mod gif;
mod golfer;
mod inline;
mod iq;
mod lexer;
mod loop_golf;
mod macro_cse;
mod neyret;
mod search;
mod swizzle;
mod twigl;
mod vocab;

#[cfg(feature = "capi")]
mod capi;

pub use aggressive::AggressiveStats;
pub use budget::{
    estimate_budget, estimate_deflate_bytes, estimate_twigl_geekest_budget, presets,
    BudgetPreset, BudgetResult,
};
pub use gif::{encode_gif, GifFrame, Rgb};
pub use golfer::{golf, golf_with_options, golf_with_protected_names, AggressiveOptions, GolfResult, GolfStats};
pub use search::{golf_harder, golf_harder_deep, AppliedChange, SearchObjective, SearchOutcome};
pub use swizzle::SwizzleAlphabet;
pub use neyret::{
    neyret_hash_snippet, neyret_hash_snippets, raymarch_loop_idioms, rotation_constant_catalogue,
    suggest_rotation_matrix_constants, NeyretHashSnippet, RaymarchLoopIdiom, RotationConstant,
    RotationSuggestion,
};
pub use iq::{
    iq_hash_snippets, iq_palette_presets, iq_sdf_snippets, iq_tonemap_snippets, IqPalettePreset,
    IqSnippet, IQ_PALETTE_FUNCTION,
};
pub use twigl::{
    rewrite_twigl_shader, rewrite_twigl_shader_mrt, rewrite_twigl_uniforms, twigl_es300_header,
    resolve_rename_collisions, twigl_export_uniform_names, twigl_snippet, twigl_snippets,
    unrewrite_twigl_shader, TwiglMode, TwiglSnippet,
};
